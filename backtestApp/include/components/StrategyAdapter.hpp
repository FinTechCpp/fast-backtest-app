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

    // Cache for trading signals
    bool should_enter_long = false;
    bool should_enter_short = false;

    // To track closed trades
    size_t last_closed_trade_count = 0;
    bool last_trade_closed = false;
    double last_trade_pnl = 0.0;
    
    // Logger
    std::shared_ptr<spdlog::logger> async_file;
    
public:
    /**
     * @brief Constructor for the adapter
     * 
     * @param broker Broker used by the backtest
     * @param data Historical data used by the backtest
     * @param strategyConfig Configuration of the strategy
     * @param generic_config Configuration specific to the Generic strategy
     */
    StrategyAdapter(std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, const StrategyConfig& strategyConfig) 
    : be::Strategy(broker, data) {
        strategy = std::make_unique<::Strategy>(strategyConfig);

        spdlog::drop("async_file_logger"); // Drop the previous logger if it exists
        async_file = spdlog::rotating_logger_mt<spdlog::async_factory>(
            "async_file_logger",       // Logger name
            "logs/Strategies/GenericStrategy_async.log",      // Log file path
            30 * 1024 * 1024,          // Max file size (100 MB)
            1
        );
        async_file->set_level(spdlog::level::debug);

        auto log_callback = [this](const std::string& message, int level) {
            spdlog::level::level_enum spdlog_level = spdlog::level::info;
            switch (level) {
                case static_cast<int>(LogLevel::DEBUG):   spdlog_level = spdlog::level::debug; break;
                case static_cast<int>(LogLevel::INFO):    spdlog_level = spdlog::level::info; break;
                case static_cast<int>(LogLevel::WARNING): spdlog_level = spdlog::level::warn; break;
                case static_cast<int>(LogLevel::FATAL):   spdlog_level = spdlog::level::err; break;
            }
            
            this->async_file->log(spdlog_level, "{}", message);
        };

        strategy->set_log_callback(log_callback);
    }
    
    /**
     * @brief Strategy initialization method
     * 
     * This method is called once at the beginning of the backtest to allow
     * the strategy to initialize itself with historical data.
     */
    void init() override {
        async_file->debug("GenericStrategyAdapter init() called");
        // Initialize the strategy
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
        std::shared_ptr<be::Trade> current_trade = trades.empty() ? nullptr : trades.back();
        
        // Populate position info for break-even functionality
        if (current_trade) {
            candle.position.entry_price = current_trade->entryPrice();
            // Calculate take profit price from the trade's TP order
            if (current_trade->tpOrder()) candle.position.take_profit_price = current_trade->tpOrder()->limit();
        }

        // Get a reference to closedTrades instead of a copy
        const auto& closedTrades = _broker->closedTrades();
        size_t currentTradeCount = closedTrades.size();

        // Only if new trades have been closed
        if (currentTradeCount > last_closed_trade_count) {
            last_closed_trade_count = currentTradeCount;
            candle.position.closed_trade_pnl = closedTrades.back()->pl();
        }

        // Update the strategy signal with the new latest candle by executing strategy logic
        Signal* signal = strategy->update_candle(candle);

        if (!signal)
            return; // No signal to process

        // Process the signal if there is one
        if (signal->type == SignalType::LIQUIDATE) {
            for (const auto& trade : trades) {
                trade->close();
            }
        }
        else if (signal->type == SignalType::MOVE_SL) {
            // Récupérer le prix de trigger depuis le signal
            double triggerPrice = signal->price > 0 ? signal->price : 0.0;
            bool success = current_trade->setBreakEven(signal->new_sl, triggerPrice);
        }
        else if (trades.empty() && signal->type == SignalType::BUY && signal->quantity > 0) {
            // Process a buy signal
            buy(
                signal->quantity,
                0,
                0,
                0,
                0,
                signal->stop_loss, 
                signal->take_profit
            );
            async_file->info("BUY signal executed: qty={}, SL={}, TP={}", 
                           signal->quantity, signal->stop_loss, signal->take_profit);
        }
        else if (trades.empty() && signal->type == SignalType::SELL && signal->quantity > 0) {
            // Process a sell signal
            sell(
                signal->quantity,
                0,
                0,
                0,
                0,
                signal->stop_loss, 
                signal->take_profit
            );
            async_file->info("SELL signal executed: qty={}, SL={}, TP={}", 
                           signal->quantity, signal->stop_loss, signal->take_profit);
        }
    }
};