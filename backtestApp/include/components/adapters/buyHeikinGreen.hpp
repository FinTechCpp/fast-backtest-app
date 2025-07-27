#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "buy_heikin_green.hpp"
#include <memory>
#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// #include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"


/**
 * @brief Adapter enabling the use of the BuyHeikinGreen strategy with the C++ backtesting engine
 * 
 * This class serves as an interface between the BuyHeikinGreen strategy (which uses its own structure)
 * and the C++ backtesting engine, which expects a class derived from Strategy.
 */
class BuyHeikinGreenAdapter : public be::Strategy {
private:
    // Specific configuration for the BuyHeikinGreen strategy
    BuyHeikinGreenConfig strategy_config;

    // Instance of the BuyHeikinGreen strategy
    std::unique_ptr<BuyHeikinGreen> strategy;

    // Cache for trading signals
    bool should_enter_long = false;
    bool should_enter_short = false;

    // To track closed trades
    size_t last_closed_trade_count = 0;
    bool last_trade_closed = false;
    double last_trade_pnl = 0.0;
    
public:
    /**
     * @brief Constructor for the adapter
     * 
     * @param broker Broker used by the backtest
     * @param data Historical data used by the backtest
     * @param base_config Base configuration common to all strategies
     * @param bhg_config Configuration specific to BuyHeikinGreen
     */
    BuyHeikinGreenAdapter(
        std::shared_ptr<be::Broker> broker, 
        std::shared_ptr<be::Data> data,
        const StrategyBaseConfig& base_config,
        const BuyHeikinGreenConfig& bhg_config
    ) : be::Strategy(broker, data), strategy_config(bhg_config) {
        // Create Strategy instance with the provided configurations
        strategy = std::make_unique<BuyHeikinGreen>(base_config, bhg_config);

        spdlog::drop("async_file_logger"); // Drop the previous logger if it exists
        auto async_file = spdlog::rotating_logger_mt<spdlog::async_factory>(
            "async_file_logger",       // Logger name
            "logs/Strategies/BuyHeikinGreen_async.log",      // Log file path
            100 * 1024 * 1024,          // Max file size (100 MB)
            1
        );
        async_file->set_level(spdlog::level::debug);

        auto log_callback = [async_file](const std::string& message, int level) {
            spdlog::level::level_enum spdlog_level = spdlog::level::info;
            switch (level) {
                case static_cast<int>(LogLevel::DEBUG):   spdlog_level = spdlog::level::debug; break;
                case static_cast<int>(LogLevel::INFO):    spdlog_level = spdlog::level::info; break;
                case static_cast<int>(LogLevel::WARNING): spdlog_level = spdlog::level::warn; break;
                case static_cast<int>(LogLevel::FATAL):   spdlog_level = spdlog::level::err; break;
            }
            
            async_file->log(spdlog_level, "{}", message);
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

        candle.ohlc.date.year = current_date.getYear();
        candle.ohlc.date.month = current_date.getMonth();
        candle.ohlc.date.day = current_date.getDay();
        candle.ohlc.date.time.hour = current_date.getHour();
        candle.ohlc.date.time.minute = current_date.getMinute();
        candle.ohlc.date.time.second = current_date.getSecond();

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
        if (signal->action == "LIQUIDATE") {
            for (const auto& trade : trades) {
                trade->close();
            }
        }
        else if (signal->action == "MOVE_SL") {
            // Récupérer le prix de trigger depuis le signal
            double triggerPrice = signal->price > 0 ? signal->price : 0.0;
            bool success = current_trade->setBreakEven(signal->new_sl, triggerPrice);
        }
        else if (trades.empty() && signal->action == "BUY" && signal->quantity > 0) {
            // Process a buy signal
            buy(
                signal->quantity,
                0,
                0,
                0,
                0,
                signal->stop_loss, 
                signal->take_profit
                // signal->tag
            );
        }
        else if (trades.empty() && signal->action == "SELL" && signal->quantity > 0) {
            // Process a sell signal
            sell(
                signal->quantity,
                0,
                0,
                0,
                0,
                signal->stop_loss, 
                signal->take_profit
                // signal->tag
            );
        }
    }
};