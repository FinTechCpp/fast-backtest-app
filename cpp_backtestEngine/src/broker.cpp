#include "broker.hpp"
#include "data.hpp"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <iostream>

namespace be {

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
    _equityCurve.resize(data->size(), cash);

    // Réinitialiser l'itérateur des données
    _data->reset();
}

void Broker::next() {
    _currentBar = _data->position();  // Utiliser la position actuelle de l'itérateur
    processOrders();
    
    // Log account equity for the equity curve
    _equityCurve[_currentBar] = _cash;
    
    // If equity is negative or zero, set all to 0 and stop the simulation
    if (_cash <= 0) {
        // Ensure margin available is also <= 0
        if (marginAvailable() > 0) {
            throw std::logic_error("Margin available is positive but equity is not");
        }
        
        // Close all trades
        for (auto& trade : _trades) {
            closeTrade(trade, _data->currentClose(), _currentBar);
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
            std::vector<Order>::iterator orderIt = _orders.begin();
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

double Broker::lastPrice() const {
    return _data->currentClose();
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

// TODO : pourquoi on travaille avec une copie des ordres ?
void Broker::processOrders() {
    try {
        // ATTENTION: Ne pas réitérer immédiatement pour vérifier les SL/TP
        // Créer une copie des ordres à traiter
        std::vector<Order> ordersCopy;
        for (const auto& order : _orders) {
            ordersCopy.push_back(order);
        }
        
        // Accès aux données OHLC
        Candle currentCandle = _data->current();
        double open = currentCandle.open;
        double high = currentCandle.high;
        double low = currentCandle.low;

        // Trouver le prix de clôture de la barre précédente
        double prevClose = 0.0;
        if (_currentBar > 0) {
            // Sauvegarde de la position actuelle
            size_t currentPos = _data->position();
            
            try {
                const Candle& prevCandle = _data->at(_currentBar - 1);
                prevClose = prevCandle.close;
            } catch (const std::exception&) {
                // En cas d'erreur, utiliser le prix d'ouverture actuel
                prevClose = open;
            }
        } else {
            prevClose = open;
        }
        
        // Traiter chaque ordre de la copie
        for (const Order& order : ordersCopy) {
            // Vérifier si l'ordre existe encore (il pourrait avoir été supprimé)
            auto orderIt = std::find(_orders.begin(), _orders.end(), order);
            if (orderIt == _orders.end()) {
                continue;
            }
            
            
            // Vérifier si le stop est atteint
            double stopPrice = order.stop();
            
            if (stopPrice > 0.0) {
                bool isStopHit = (order.isLong() && high >= stopPrice) || 
                                 (!order.isLong() && low <= stopPrice);
                if (!isStopHit) {
                    continue;
                }
                
                // Le stop est atteint, convertir en ordre market/limit
                orderIt->_replace("stopPrice", 0.0);
            }

            // Déterminer le prix d'achat
            double price = 0.0;
            
            // Gestion des ordres limit
            double limitPrice = order.limit();

            if (limitPrice > 0.0) {
                bool isLimitHit = (order.isLong() && low <= limitPrice) || 
                                  (!order.isLong() && high >= limitPrice);

                bool isLimitHitBeforeStop = isLimitHit && stopPrice > 0.0 && 
                    ((order.isLong() && limitPrice <= stopPrice) ||
                     (!order.isLong() && limitPrice >= stopPrice));

                if (!isLimitHit || isLimitHitBeforeStop) {
                    continue;
                }
                
                // Calculer le prix de remplissage
                if (order.isLong()) {
                    price = stopPrice > 0.0 ? std::min(stopPrice, limitPrice) : limitPrice;
                } else {
                    price = stopPrice > 0.0 ? std::max(stopPrice, limitPrice) : limitPrice;
                }
            } else {
                // Ordre market ou market-if-touched
                bool isContingentOrder = order.isContingent();

                price = (_tradeOnClose && !isContingentOrder) ? prevClose : open;
                
                if (stopPrice > 0.0) {
                    price = order.isLong() ? std::max(price, stopPrice) : std::min(price, stopPrice);
                }
            }
            
            // Indice temporel d'entrée/sortie
            bool isMarketOrder = limitPrice <= 0.0 && stopPrice <= 0.0;
            size_t timeIndex = _currentBar;
            if (isMarketOrder && _tradeOnClose && !order.isContingent()) {
                if (_currentBar > 0) {
                    timeIndex = _currentBar - 1;
                }
            }
            
            // Si l'ordre est un SL/TP, il doit fermer une position existante
            if (order.parentTrade()) {
                auto trade = order.parentTrade();
                double prevSize = trade->size();
                
                // Calculer la taille réelle à fermer
                double closeSize = std::copysign(
                    std::min(std::abs(prevSize), std::abs(order.size())), 
                    order.size()
                );
                
                // Vérifier si le trade existe encore
                auto tradeIt = std::find(_trades.begin(), _trades.end(), trade);
                if (tradeIt != _trades.end()) {
                    try {
                        reduceTrade(trade, price, closeSize, timeIndex);
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR in reduceTrade: " << e.what() << std::endl;
                    }
                }
                
                // Supprimer l'ordre
                auto orderToRemove = std::find(_orders.begin(), _orders.end(), order);
                if (orderToRemove != _orders.end()) {
                    _orders.erase(orderToRemove);
                }
            } else {
                // C'est un ordre autonome pour un nouveau trade
                double adjustedP = adjustedPrice(order.size(), price);
                double adjustedPriceWithCommission = adjustedP + calculateCommission(order.size(), price);
                
                // Traitement de la taille proportionnelle
                double size = order.size();
                if (std::abs(size) < 1.0) {
                    size = std::copysign(
                        std::floor((marginAvailable() * _leverage * std::abs(size)) / adjustedPriceWithCommission),
                        size
                    );
                    
                    if (size == 0) {
                        // Pas assez de marge, annuler l'ordre
                        auto orderToRemove = std::find(_orders.begin(), _orders.end(), order);
                        if (orderToRemove != _orders.end()) {
                            _orders.erase(orderToRemove);
                        }
                        continue;
                    }
                }
                
                int needSize = static_cast<int>(size);
                
                // Gestion des positions opposées si le hedging est désactivé
                if (!_hedging) {
                    for (auto trade : _trades) {
                        // Ignorer les trades dans la même direction
                        if ((trade->isLong() && order.isLong()) || 
                            (!trade->isLong() && !order.isLong())) {
                            continue;
                        }
                        
                        // Ordre plus grand que la position existante
                        if (std::abs(needSize) >= std::abs(trade->size())) {
                            needSize += static_cast<int>(trade->size());
                            try {
                                closeTrade(trade, price, timeIndex);
                            } catch (const std::exception& e) {
                                std::cerr << "ERROR in closeTrade: " << e.what() << std::endl;
                            }
                            
                            // Reprendre depuis le début car _trades a été modifié
                            break;
                        } else {
                            // La position existante est plus grande que l'ordre
                            try {
                                reduceTrade(trade, price, needSize, timeIndex);
                            } catch (const std::exception& e) {
                                std::cerr << "ERROR in reduceTrade: " << e.what() << std::endl;
                            }
                            needSize = 0;
                            break;
                        }
                    }
                }
                
                // Vérifier si nous avons assez de marge disponible
                // if (std::abs(needSize) * adjustedPriceWithCommission > marginAvailable() * _leverage) {
                //     // Pas assez de marge, annuler l'ordre
                //     auto orderToRemove = std::find(_orders.begin(), _orders.end(), order);
                //     if (orderToRemove != _orders.end()) {
                //         _orders.erase(orderToRemove);
                //     }
                //     continue;
                // }
                
                // Ouvrir un nouveau trade si besoin
                if (needSize != 0) {
                    // Traiter les points SL/TP
                    double slPrice = order.sl();
                    double tpPrice = order.tp();
                    
                    // Calculate SL/TP prices using points if specified
                    if (order.slPoints() > 0.0)
                        slPrice = price - order.slPoints() * (order.isLong() ? 1 : -1);

                    if (order.tpPoints() > 0.0)
                        tpPrice = price + order.tpPoints() * (order.isLong() ? 1 : -1);
                    
                    // Ouvrir le trade
                    try {
                        openTrade(price, needSize, slPrice, tpPrice, timeIndex, order.tag(), order);
                        
                        // MODIFICATION: Ne pas retraiter les ordres SL/TP dans la même barre
                        // Supprimer ou commenter ces lignes:
                        /*
                        if (slPrice > 0.0 || tpPrice > 0.0) {
                            if (isMarketOrder) {
                                continueProcessing = true;
                            } else if (stopPrice > 0.0 && order.limit() <= 0.0 && tpPrice > 0.0 && 
                                     ((order.isLong() && tpPrice <= high && (slPrice <= 0.0 || slPrice > low)) ||
                                     (!order.isLong() && tpPrice >= low && (slPrice <= 0.0 || slPrice < high)))) {
                                continueProcessing = true;
                            }
                        }
                        */
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR in openTrade: " << e.what() << std::endl;
                    }
                }
                
                // Supprimer l'ordre traité
                auto orderToRemove = std::find(_orders.begin(), _orders.end(), order);
                if (orderToRemove != _orders.end()) {
                    _orders.erase(orderToRemove);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "EXCEPTION in processOrders: " << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cerr << "UNKNOWN EXCEPTION in processOrders" << std::endl;
        throw;
    }
}

void Broker::openTrade(double price, double size, double sl, double tp, 
                      size_t barIndex, const std::string& tag, const Order& order) {
    // Create a new trade - utilise la bougie courante pour la date
    const Candle& currentCandle = _data->current();
    
    std::shared_ptr<Trade> trade = std::make_shared<Trade>(
        shared_from_this(), 
        size, 
        price, 
        barIndex, 
        currentCandle.date,  // Utilisation de la date de la bougie courante
        tag
    );

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
                                                  trade->entryBar(), trade->entryDate(), trade->tag());
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
    
    // Remove associated SL/TP orders
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
    trade->setExitDate(_data->currentDate());

    _closedTrades.push_back(trade);
    
    // Apply commission for trade exit and update cash
    double commission = calculateCommission(trade->size(), price);
    _cash += trade->pl() - commission;
    
    // Save commissions on the Trade instance for stats
    double openCommission = calculateCommission(trade->size(), trade->entryPrice());
    trade->setCommissions(commission + openCommission);

    // Mettre à jour l'equity curve avec le cash actuel après la clôture du trade
    if (barIndex < _equityCurve.size()) {
        _equityCurve[barIndex] = _cash;
    }
}

void Broker::cancelOrder(const Order& order) {
    auto it = std::find_if(_orders.begin(), _orders.end(),
        [&order](const Order& o) { return o == order; });
        
    if (it != _orders.end()) {
        _orders.erase(it);
    }
}

// cette methode est tres bien mais il faudrait mettre en commun avec processOrders() et mettre
// ce qui est en commun dans des fonction pour simplifier la lecture
void Broker::finalizeOrders() {
    try {
        // Accéder à la dernière bougie disponible plutôt qu'à la bougie courante
        const Candle& lastCandle = _data->at(_data->size() - 1);
        
        // Créer une copie des ordres à traiter
        std::vector<Order> ordersCopy = _orders;
        
        // Variables pour le traitement des ordres
        double open = lastCandle.open;
        double high = lastCandle.high;
        double low = lastCandle.low;
        double close = lastCandle.close;
        
        // Traiter les ordres de la même façon que processOrders() mais en utilisant
        // la dernière bougie et sans tenter d'accéder à current()
        
        // Code simplifié pour traiter les ordres de clôture
        for (const Order& order : ordersCopy) {
            auto orderIt = std::find(_orders.begin(), _orders.end(), order);
            if (orderIt == _orders.end()) continue;
            
            // Si c'est un ordre lié à un trade, fermer ce trade
            if (order.parentTrade()) {
                auto trade = order.parentTrade();
                auto tradeIt = std::find(_trades.begin(), _trades.end(), trade);
                if (tradeIt != _trades.end()) {
                    try {
                        // Fermer le trade avec le dernier prix
                        closeTrade(trade, close, _data->size() - 1);
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR in finalizeOrders/closeTrade: " << e.what() << std::endl;
                    }
                }
                
                // Supprimer l'ordre
                auto orderToRemove = std::find(_orders.begin(), _orders.end(), order);
                if (orderToRemove != _orders.end()) {
                    _orders.erase(orderToRemove);
                }
            }
            // Autres types d'ordres...
        }
        
        // Fermer tous les trades restants avec le dernier prix
        std::vector<std::shared_ptr<Trade>> tradesCopy = _trades;
        for (auto& trade : tradesCopy) {
            try {
                closeTrade(trade, close, _data->size() - 1);
            } catch (const std::exception& e) {
                std::cerr << "ERROR in finalizeOrders/closeTrade: " << e.what() << std::endl;
            }
        }
        
        // Mettre à jour l'equity curve pour la dernière barre
        size_t lastIndex = _data->size() - 1;
        if (lastIndex < _equityCurve.size()) {
            _equityCurve[lastIndex] = _cash;
        }
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de la finalisation des ordres: " << e.what() << std::endl;
    }
}


} // namespace be