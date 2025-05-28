#include "broker.hpp"
#include "data.hpp"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <iostream>

class OutOfMoneyError : public std::runtime_error {
public:
    OutOfMoneyError() : std::runtime_error("Out of money") {}
};

Broker::Broker(std::shared_ptr<Data> data,
               double cash,
               double spread,
               double commission,
               double margin,
               bool tradeOnClose,
               bool hedging,
               bool exclusiveOrders)
    : _data(data),
      _cash(cash),
      _spread(spread),
      _commission(commission),
      _leverage(1.0 / margin),
      _tradeOnClose(tradeOnClose),
      _hedging(hedging),
      _exclusiveOrders(exclusiveOrders),
      _currentBar(0) 
{
    if (cash <= 0) {
        throw std::invalid_argument("Cash must be positive");
    }
    if (margin <= 0 || margin > 1) {
        throw std::invalid_argument("Margin must be between 0 and 1");
    }
    
    // Initialize equity curve with the size of data
    _equityCurve.resize(data->size(), std::numeric_limits<double>::quiet_NaN());
}

void Broker::next() {
    _currentBar = _data->size() - 1;
    processOrders();
    
    // Log account equity for the equity curve
    double currentEquity = equity();
    _equityCurve[_currentBar] = currentEquity;
    
    // If equity is negative or zero, set all to 0 and stop the simulation
    if (currentEquity <= 0) {
        // Ensure margin available is also <= 0
        if (marginAvailable() > 0) {
            throw std::logic_error("Margin available is positive but equity is not");
        }
        
        // Close all trades
        for (auto& trade : _trades) {
            closeTrade(trade, _data->Close(-1), _currentBar);
        }
        
        _cash = 0;
        std::fill(_equityCurve.begin() + _currentBar, _equityCurve.end(), 0.0);
        throw OutOfMoneyError();
    }
}

Order Broker::newOrder(double size, double limit, double stop, double sl, double tp,
                      const std::string& tag, double slPoints, double tpPoints,
                      std::shared_ptr<Trade> trade) {
    // Validate size
    size = static_cast<double>(std::round(size));
    if (size == 0) {
        throw std::invalid_argument("Order size cannot be zero");
    }
    
    bool isLong = size > 0;
    double adjustedP = adjustedPrice(size);
    
    // Validate stop-loss and take-profit levels
    if (sl != 0.0 || tp != 0.0) {
        if (isLong) {
            if (!((sl <= 0.0 || sl < (limit > 0.0 ? limit : (stop > 0.0 ? stop : adjustedP))) &&
                 (tp <= 0.0 || tp > (limit > 0.0 ? limit : (stop > 0.0 ? stop : adjustedP))))) {
                throw std::invalid_argument("Long orders require: SL < Entry < TP");
            }
        } else {
            if (!((tp <= 0.0 || tp < (limit > 0.0 ? limit : (stop > 0.0 ? stop : adjustedP))) &&
                 (sl <= 0.0 || sl > (limit > 0.0 ? limit : (stop > 0.0 ? stop : adjustedP))))) {
                throw std::invalid_argument("Short orders require: TP < Entry < SL");
            }
        }
    }
    
    Order order(shared_from_this(), size, limit, stop, sl, tp, trade, tag, slPoints, tpPoints);
    
    // If it's not a contingent order (i.e., not tied to an existing trade)
    if (!trade) {
        // If exclusive orders, cancel all non-contingent orders and close all open trades
        if (_exclusiveOrders) {
            // Cancel all non-contingent orders
            auto orderIt = _orders.begin();
            while (orderIt != _orders.end()) {
                if (!orderIt->isContingent()) {
                    orderIt = _orders.erase(orderIt);
                } else {
                    ++orderIt;
                }
            }
            
            // Close all open trades
            for (auto& t : _trades) {
                // Create a close order for each trade
                double closeSize = -t->size();
                Order closeOrder(shared_from_this(), closeSize, 0.0, 0.0, 0.0, 0.0, t, t->tag());
                _orders.push_back(closeOrder);
            }
        }
    }
    
    // Insert the order at the appropriate position
    // Put SL orders at the front so they're processed first
    if (trade && stop > 0.0) {
        _orders.insert(_orders.begin(), order); // SL order goes to the front
    } else {
        _orders.push_back(order); // Other orders at the back
    }
    
    return order;
}

void Broker::cancelOrder(const Order& order) {
    auto it = std::find_if(_orders.begin(), _orders.end(),
        [&order](const Order& o) { return o == order; });
        
    if (it != _orders.end()) {
        _orders.erase(it);
    }
}

double Broker::lastPrice() const {
    return _data->Close(-1);
}

double Broker::equity() const {
    double totalPL = 0.0;
    for (const auto& trade : _trades) {
        totalPL += trade->pl();
    }
    return _cash + totalPL;
}

double Broker::marginAvailable() const {
    double marginUsed = 0.0;
    for (const auto& trade : _trades) {
        marginUsed += std::abs(trade->size()) * trade->entryPrice() / _leverage;
    }
    return std::max(0.0, equity() - marginUsed);
}

Position Broker::position() const {
    return Position(const_cast<Broker*>(this)->shared_from_this());
}

double Broker::adjustedPrice(double size, double price) const {
    if (price == 0.0) {
        price = lastPrice();
    }
    // In long positions, the adjusted price is a fraction higher, and vice versa
    return price * (1.0 + std::copysign(_spread, size));
}

double Broker::calculateCommission(double size, double price) const {
    return std::abs(size) * price * _commission;
}

void Broker::processOrders() {
    double open = _data->Open(-1);
    double high = _data->High(-1);
    double low = _data->Low(-1);
    bool reprocessOrders = false;

    // Process orders
    auto orderIt = _orders.begin();
    while (orderIt != _orders.end()) {
        // Check if the order was already removed
        bool orderRemoved = false;

        // Check if stop condition was hit
        double stopPrice = orderIt->stop();
        if (stopPrice > 0.0) {
            bool isStopHit = (orderIt->isLong() && high >= stopPrice) || 
                            (!orderIt->isLong() && low <= stopPrice);
            if (!isStopHit) {
                ++orderIt;
                continue;
            }
            
            // Stop price was hit, convert to market/limit order
            orderIt->_replace("stopPrice", 0.0);
        }

        // Determine purchase price
        // Check if limit order can be filled
        double price = 0.0;
        if (orderIt->limit() > 0.0) {
            bool isLimitHit = (orderIt->isLong() && low <= orderIt->limit()) || 
                             (!orderIt->isLong() && high >= orderIt->limit());
            
            bool isLimitHitBeforeStop = isLimitHit && stopPrice > 0.0 && 
                ((orderIt->isLong() && orderIt->limit() <= stopPrice) ||
                 (!orderIt->isLong() && orderIt->limit() >= stopPrice));
                 
            if (!isLimitHit || isLimitHitBeforeStop) {
                ++orderIt;
                continue;
            }

            // Calculate fill price
            if (orderIt->isLong()) {
                price = stopPrice > 0.0 ? std::min(stopPrice, orderIt->limit()) : orderIt->limit();
            } else {
                price = stopPrice > 0.0 ? std::max(stopPrice, orderIt->limit()) : orderIt->limit();
            }
        } else {
            // Market-if-touched / market order
            // For contingent orders, always use the next open
            bool isContingentOrder = orderIt->isContingent();
            double prevClose = _data->Close(-2);
            
            if (_tradeOnClose && !isContingentOrder) {
                price = prevClose;
            } else {
                price = open;
            }
            
            if (stopPrice > 0.0) {
                if (orderIt->isLong()) {
                    price = std::max(price, stopPrice);
                } else {
                    price = std::min(price, stopPrice);
                }
            }
        }

        // Determine entry/exit time index
        bool isMarketOrder = orderIt->limit() <= 0.0 && stopPrice <= 0.0;
        size_t timeIndex = _currentBar;
        if (isMarketOrder && _tradeOnClose && !orderIt->isContingent()) {
            timeIndex = _currentBar - 1;
        }

        // If order is a SL/TP order, it should close an existing trade
        if (orderIt->parentTrade()) {
            auto trade = orderIt->parentTrade();
            double prevSize = trade->size();
            
            // Calculate actual size to close
            double closeSize = std::copysign(
                std::min(std::abs(prevSize), std::abs(orderIt->size())), 
                orderIt->size()
            );
            
            // If this trade isn't already closed
            auto tradeIt = std::find(_trades.begin(), _trades.end(), trade);
            if (tradeIt != _trades.end()) {
                reduceTrade(trade, price, closeSize, timeIndex);
                // Check if the trade was fully closed
                if (orderIt->size() == -prevSize) {
                    // Should be removed by now if fully closed
                    if (std::find(_trades.begin(), _trades.end(), trade) != _trades.end()) {
                        throw std::logic_error("Trade should have been closed but wasn't");
                    }
                }
            }
            
            // Remove the order from the list
            orderIt = _orders.erase(orderIt);
            orderRemoved = true;
        } else {
            // This is a standalone trade

            // Adjust price to include commission/spread
            double adjustedP = adjustedPrice(orderIt->size(), price);
            double adjustedPriceWithCommission = adjustedP + calculateCommission(orderIt->size(), price);

            // If order size was specified proportionally
            double size = orderIt->size();
            if (std::abs(size) < 1.0) {
                size = std::copysign(
                    std::floor((marginAvailable() * _leverage * std::abs(size)) / adjustedPriceWithCommission),
                    size
                );
                
                // Not enough cash/margin even for a single unit
                if (size == 0) {
                    std::cerr << "WARNING: Broker canceled the relative-sized order due to insufficient margin." << std::endl;
                    orderIt = _orders.erase(orderIt);
                    orderRemoved = true;
                    continue;
                }
            }
            
            int needSize = static_cast<int>(size);

            if (!_hedging) {
                // Fill position by FIFO closing/reducing existing opposite-facing trades
                auto tradeIt = _trades.begin();
                while (tradeIt != _trades.end() && needSize != 0) {
                    auto& trade = *tradeIt;
                    
                    // Skip trades in the same direction
                    if ((trade->isLong() && orderIt->isLong()) || 
                        (!trade->isLong() && !orderIt->isLong())) {
                        ++tradeIt;
                        continue;
                    }
                    
                    // Order size greater than this opposite-directed existing trade
                    if (std::abs(needSize) >= std::abs(trade->size())) {
                        needSize += trade->size(); // Adjust needed size after closing
                        closeTrade(trade, price, timeIndex);
                        // After closing, reposition the iterator since _trades vector was modified
                        tradeIt = _trades.begin();
                    } else {
                        // The existing trade is larger than the new order
                        reduceTrade(trade, price, needSize, timeIndex);
                        needSize = 0;
                        break;
                    }
                }
            }

            // If we don't have enough liquidity to cover the order, the broker CANCELS it
            if (std::abs(needSize) * adjustedPriceWithCommission > marginAvailable() * _leverage) {
                orderIt = _orders.erase(orderIt);
                orderRemoved = true;
                continue;
            }

            // Open a new trade if there's remaining size
            if (needSize != 0) {
                // Process SL/TP points if they're set
                double slPrice = orderIt->sl();
                double tpPrice = orderIt->tp();
                
                if (orderIt->slPoints() > 0.0) {
                    if (orderIt->isLong()) {
                        slPrice = price - orderIt->slPoints();
                    } else {
                        slPrice = price + orderIt->slPoints();
                    }
                }
                
                if (orderIt->tpPoints() > 0.0) {
                    if (orderIt->isLong()) {
                        tpPrice = price + orderIt->tpPoints();
                    } else {
                        tpPrice = price - orderIt->tpPoints();
                    }
                }
                
                openTrade(price, static_cast<int>(size), slPrice, tpPrice, timeIndex, orderIt->tag(), *orderIt);

                // Check if we need to reprocess for SL/TP orders
                if (slPrice > 0.0 || tpPrice > 0.0) {
                    if (isMarketOrder) {
                        reprocessOrders = true;
                    } 
                    // Handle potential SL/TP hits in the same bar
                    else if (stopPrice > 0.0 && orderIt->limit() <= 0.0 && tpPrice > 0.0 && 
                             ((orderIt->isLong() && tpPrice <= high && (slPrice <= 0.0 || slPrice > low)) ||
                              (!orderIt->isLong() && tpPrice >= low && (slPrice <= 0.0 || slPrice < high)))) {
                        reprocessOrders = true;
                    } 
                    else if ((low <= slPrice && slPrice <= high) || (low <= tpPrice && tpPrice <= high)) {
                        std::cerr << "WARNING: A contingent SL/TP order would execute in the same bar "
                                  << "its parent stop/limit order was turned into a trade. "
                                  << "The affected SL/TP order will be executed on the next matching bar." << std::endl;
                    }
                }
            }

            // Remove the processed order
            orderIt = _orders.erase(orderIt);
            orderRemoved = true;
        }

        // Move to next order if not already removed
        if (!orderRemoved) {
            ++orderIt;
        }
    }

    // If needed, reprocess orders to handle SL/TP orders added to the queue
    if (reprocessOrders) {
        processOrders();
    }
}

void Broker::openTrade(double price, double size, double sl, double tp, 
                      size_t barIndex, const std::string& tag, const Order& order) {
    // Create a new trade
    auto trade = std::make_shared<Trade>(shared_from_this(), size, price, barIndex, tag);
    _trades.push_back(trade);
    
    // Apply commission at trade open
    _cash -= calculateCommission(size, price);
    
    // Create SL/TP (bracket) orders
    if (tp > 0.0) {
        Order tpOrder = newOrder(-size, tp, 0.0, 0.0, 0.0, tag, 0.0, 0.0, trade);
        trade->setTpOrder(tpOrder);
    }
    
    if (sl > 0.0) {
        Order slOrder = newOrder(-size, 0.0, sl, 0.0, 0.0, tag, 0.0, 0.0, trade);
        trade->setSlOrder(slOrder);
    }
}

void Broker::reduceTrade(std::shared_ptr<Trade> trade, double price, double size, size_t barIndex) {
    // Validate that the trade and size are in opposite directions
    if (trade->size() * size >= 0) {
        throw std::invalid_argument("Trade size and reduction size must have opposite signs");
    }
    
    // Validate that we're not trying to reduce by more than the trade size
    if (std::abs(trade->size()) < std::abs(size)) {
        throw std::invalid_argument("Reduction size cannot exceed trade size");
    }
    
    double sizeLeft = trade->size() + size;
    
    // Check if the trade should be completely closed
    if (sizeLeft == 0.0) {
        closeTrade(trade, price, barIndex);
    } else {
        // Reduce existing trade
        trade->setSize(sizeLeft);
        
        // Adjust SL/TP orders if they exist
        if (trade->slOrder()) {
            trade->slOrder()->_replace("size", -trade->size());
        }
        if (trade->tpOrder()) {
            trade->tpOrder()->_replace("size", -trade->size());
        }

        // Create a copy of the trade that will be closed
        auto closedPortion = std::make_shared<Trade>(shared_from_this(), 
                                                  -size, trade->entryPrice(), 
                                                  trade->entryBar(), trade->tag());
        _trades.push_back(closedPortion);
        
        // Close the reduced copy
        closeTrade(closedPortion, price, barIndex);
    }
}

void Broker::closeTrade(std::shared_ptr<Trade> trade, double price, size_t barIndex) {
    // Remove the trade from active trades
    auto tradeIt = std::find(_trades.begin(), _trades.end(), trade);
    if (tradeIt == _trades.end()) {
        throw std::invalid_argument("Attempted to close a trade that is not in the active trades list");
    }
    _trades.erase(tradeIt);
    
    // Remove associated SL/TP orders if they exist
    if (trade->slOrder()) {
        auto orderIt = std::find(_orders.begin(), _orders.end(), *(trade->slOrder()));
        if (orderIt != _orders.end()) {
            _orders.erase(orderIt);
        }
    }
    
    if (trade->tpOrder()) {
        auto orderIt = std::find(_orders.begin(), _orders.end(), *(trade->tpOrder()));
        if (orderIt != _orders.end()) {
            _orders.erase(orderIt);
        }
    }
    
    // Set exit information and add to closed trades
    trade->setExitPrice(price);
    trade->setExitBar(barIndex);
    _closedTrades.push_back(trade);
    
    // Apply commission for trade exit and update cash
    double commission = calculateCommission(trade->size(), price);
    _cash += trade->pl() - commission;
    
    // Save commissions on the Trade instance for stats
    double openCommission = calculateCommission(trade->size(), trade->entryPrice());
    trade->setCommissions(commission + openCommission);
}