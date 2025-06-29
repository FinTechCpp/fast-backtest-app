#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
// #include "Strategies/buy_heikin_green.hpp"
#include "sell_heikin_red.hpp"
#include <memory>
#include <iostream>

/**
 * @brief Adapter enabling the use of the SellHeikinRed strategy with the C++ backtesting engine
 * 
 * This class serves as an interface between the SellHeikinRed strategy (which uses its own structure)
 * and the C++ backtesting engine, which expects a class derived from Strategy.
 */
class SellHeikinRedAdapter : public be::Strategy {
private:
    // Specific configuration for the SellHeikinRed strategy
    SellHeikinRedConfig strategy_config;

    // Instance of the SellHeikinRed strategy
    std::unique_ptr<SellHeikinRed> strategy;

    // Cache for trading signals
    bool should_enter_long = false;
    bool should_enter_short = false;

    // To track closed trades
    int last_closed_trade_count = 0;
    bool last_trade_closed = false;
    double last_trade_pnl = 0.0;
    
public:
    /**
     * @brief Constructor for the adapter
     * 
     * @param broker Broker used by the backtest
     * @param data Historical data used by the backtest
     * @param base_config Base configuration common to all strategies
     * @param shr_config Configuration specific to SellHeikinRed
     */
    SellHeikinRedAdapter(
        std::shared_ptr<be::Broker> broker, 
        std::shared_ptr<be::Data> data,
        const StrategyBaseConfig& base_config,
        const SellHeikinRedConfig& shr_config
    ) : be::Strategy(broker, data), strategy_config(shr_config) {
        // Create Strategy instance with the provided configurations
        strategy = std::make_unique<SellHeikinRed>(base_config, shr_config);
        // strategy->set_log_level(LogLevel::DEBUG);

        auto log_callback = [](const std::string& message, int level) {
            LogLevel logLevel = static_cast<LogLevel>(level);
            
            // Get the current timestamp with millisecond precision
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            // Format the timestamp
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
            ss << "," << std::setw(3) << std::setfill('0') << ms.count();
            
            
            // Display the log message with the timestamp and log level
            std::cout << ss.str() << " [" << logLevel << "]: " << message << std::endl;
        };

        strategy->set_log_callback(log_callback);
    }
    
    /**
     * @brief Initialization method for the strategy
     * 
     * This method is called once at the beginning of the backtest to allow
     * the strategy to initialize itself with historical data.
     */
    void init() override {
        // Initialize the strategy
    }
    
    /**
     * @brief Main method called on each new candle
     * 
     * This method converts market data into the format expected by the strategy,
     * updates the strategy, and processes any generated signals.
     */
    void next() override {
        // TODO: for the moment, too much use of getData() which returns all backtest data is very slow
        // especially since we only need the latest candle
        // Check if a position has been closed since the last candle
        // const std::vector<be::Trade> closedTrades = getClosedTrades();
        // if (closedTrades.size() > last_closed_trade_count) {
        //     be::Trade last_trade = closedTrades.back();

        //     // Check if the trade was closed at the last candle
        //     if (last_trade.exitDate() == getData()->getDate(-1)) {
        //         last_trade_closed = true;
        //         last_trade_pnl = last_trade.pl(); 
        //     }
            
        //     // Update the counter
        //     last_closed_trade_count = closedTrades.size();
        // }

        // Create a Candle object from the current data
        Candle candle;
        
        // Fill the DateTime structure from the current candle
        // Using the new interface
        const be::Candle& currentCandle = getData()->current();
        be::Date current_date = currentCandle.date;
        
        candle.ohlc.date.year = current_date.getYear();
        candle.ohlc.date.month = current_date.getMonth();
        candle.ohlc.date.day = current_date.getDay();
        candle.ohlc.date.time.hour = current_date.getHour();
        candle.ohlc.date.time.minute = current_date.getMinute();
        candle.ohlc.date.time.second = current_date.getSecond();
        
        // Fill the OHLC values using the new interface
        candle.ohlc.open = currentCandle.open;
        candle.ohlc.high = currentCandle.high;
        candle.ohlc.low = currentCandle.low;
        candle.ohlc.close = currentCandle.close;

        // Fill the position information
        // be::Position position = getPosition();
        // candle.position.in_position = position ? true : false;
        // candle.position.position_pl_pct = position ? position.plPercent() : 0.0;
        // // candle.position.entry_price = position ? position.entryPrice() : 0.0;
        // candle.position.position_size = position ? position.size() : 0.0;

        // Add the P&L of the last closed trade if there is one
        candle.position.closed_trade_pnl = 0.0;
        if (last_trade_closed) {
            candle.position.closed_trade_pnl = last_trade_pnl;
            last_trade_closed = false;
            last_trade_pnl = 0.0;
        }
        
        // Update the strategy signal with the new latest candle by executing strategy logic
        Signal* signal = strategy->update_candle(candle);


        if (!signal) {
            return; // No signal to process
        }
        
        // // Process the signal if there is one
        // if (signal->action == "LIQUIDATE") {
        //     if (position) {
        //         position.close();
        //         std::cout << "Closing position due to LIQUIDATE signal" << std::endl;
        //     }
        // }
        // else if (signal->action == "MOVE_SL") {
        //     // Déplacer le stop loss
        //     if (position) {
        //         // position.updateSl(signal->new_sl);
        //         std::cout << "Moving stop loss to " << signal->new_sl << std::endl;
        //     }
        // }
        // else if (!position && signal->action == "BUY") {
        //     // Exécuter un signal d'achat
        //     buy(
        //         signal->quantity,
        //         0,
        //         0,
        //         0,
        //         0,
        //         signal->stop_loss, 
        //         signal->take_profit
        //         // signal->tag
        //     );

        //     std::cout << "Opening BUY position: Price=" << signal->price 
        //              << ", Size=" << signal->quantity
        //              << ", SL=" << signal->stop_loss
        //              << ", TP=" << signal->take_profit << std::endl;
        // }
        // else if (!position && signal->action == "SELL") {
        //     // Exécuter un signal de vente
        //     sell(
        //         signal->quantity,
        //         0,
        //         0,
        //         0,
        //         0,
        //         signal->stop_loss, 
        //         signal->take_profit
        //         // signal->tag
        //     );

        //     std::cout << "Opening SELL position: Price=" << signal->price 
        //              << ", Size=" << signal->quantity
        //              << ", SL=" << signal->stop_loss
        //              << ", TP=" << signal->take_profit << std::endl;
        // }
    }
};