#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "common.h"
#include "components/serializerAdapters.h"  // Include structures and serialization
#include "strategy.h"  // Include the generic strategy class
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"

/**
 * @brief Adapter enabling the use of generic strategies with the C++ backtesting engine
 * 
 * This class serves as an interface between the GenericStrategy (which uses its own structure)
 * and the C++ backtesting engine, which expects a class derived from Strategy.
 */
class StrategyAdapter : public be::Strategy {
private:
    // Instance of the Generic strategy
    std::unique_ptr<::Strategy> strategy;

    // To track closed trades
    size_t last_closed_trade_count = 0;
    
    std::string name;
public:
    /**
     * @brief Constructor for the adapter
     * 
     * @param broker Broker used by the backtest
     * @param data Historical data used by the backtest
     * @param strategyConfig Configuration of the strategy
     * @param generic_config Configuration specific to the Generic strategy
     */
    StrategyAdapter(std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, const StrategyConfig& strategyConfig, std::function<void(const std::string&)> logCallback = nullptr)
    : be::Strategy(broker, data), 
      name(strategyConfig.name),
      strategy(std::make_unique<::Strategy>(strategyConfig, logCallback))
    {
    }
    
    /**
     * @brief Strategy initialization method
     * 
     * This method is called once at the beginning of the backtest to allow
     * the strategy to initialize itself with historical data.
     */
    void init() override {
    }
    
    /**
     * @brief Main method called on each new candle
     * This method converts market data into the format expected by the strategy,
     * updates the strategy, and processes any generated signals.
     */
    void next() override {    
        Candle candle;
        
        // Fill the DateTime structure from the current candle
        const be::Candle& currentCandle = getData()->current();
        be::Date current_date = currentCandle.date;

        candle.ohlc.date.year = current_date.year;
        candle.ohlc.date.month = current_date.month;
        candle.ohlc.date.day = current_date.day;
        candle.ohlc.date.time.hour = current_date.hour;
        candle.ohlc.date.time.minute = current_date.minute;
        candle.ohlc.date.time.second = current_date.second;

        // Fill the OHLC values with the new interface
        candle.ohlc.open = currentCandle.open;
        candle.ohlc.high = currentCandle.high;
        candle.ohlc.low = currentCandle.low;
        candle.ohlc.close = currentCandle.close;

        // Fill the position information
        const std::vector<std::shared_ptr<be::Trade>>& trades = _broker->trades();
        // std::shared_ptr<be::Trade> current_trade = trades.empty() ? nullptr : trades.back();
        
        // Populate position info for break-even functionality
        if (!trades.empty()) {
            candle.position.entry_price = trades.back()->entryPrice();
            // Calculate take profit price from the trade's TP order
            // if (current_trade->tpOrder()) candle.position.take_profit_price = current_trade->tpOrder()->limit();
            if (trades.back()->tpOrder())
                candle.position.take_profit_price = trades.back()->tpOrder()->limitPrice();
        }

        // Get a reference to closedTrades instead of a copy
        const auto& closedTrades = _broker->closedTrades();
        size_t currentTradeCount = closedTrades.size();

        // Only if new trades have been closed
        if (currentTradeCount > last_closed_trade_count) {
            last_closed_trade_count = currentTradeCount;
            candle.position.closed_trade_pnl = closedTrades.back().pl;
        }

        // Update the strategy signal with the new latest candle by executing strategy logic
        Signal signal = strategy->update_candle(candle);

        // DateTime dtBreakPoint{2022, 8, 8, Time{21, 59, 0}};

        // if (candle.ohlc.date >= dtBreakPoint) {
        //     int a = 0;
        // }

        if (signal.type == SignalType::NONE)
            return; // No signal to process

        // Process the signal if there is one
        if (signal.type == SignalType::LIQUIDATE) {
            _broker->closeAllTrades();
        }
        else if (signal.type == SignalType::MOVE_SL && !trades.empty()) {
            // Récupérer le prix de trigger depuis le signal
            double triggerPrice = signal.price > 0 ? signal.price : 0.0;
            bool success = trades.back()->setBreakEven(signal.new_sl, triggerPrice);

            // _broker->setBreakEven(current_trade, signal.new_sl, triggerPrice);
        }
        else if (trades.empty() && signal.type == SignalType::BUY && signal.quantity > 0) {
            _broker->submitOrder(
                signal.quantity, 
                be::OrderSide::BUY, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::points(signal.stop_loss), 
                be::TPValue::points(signal.take_profit), 
                nullptr,
                strategy->getName()
            );
        }
        else if (trades.empty() && signal.type == SignalType::SELL && signal.quantity > 0) {
            _broker->submitOrder(
                signal.quantity, 
                be::OrderSide::SELL, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::points(signal.stop_loss), 
                be::TPValue::points(signal.take_profit), 
                nullptr,
                strategy->getName()
            );
        }
    }
};