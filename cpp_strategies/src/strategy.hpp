#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "indicators.hpp"

struct Candle {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    std::unordered_map<std::string, double> indicators;
    
    // Position information
    bool in_position = false;
    double entry_price = 0.0;
    double position_size = 0.0;
    double position_pl_pct = 0.0;
};

struct Signal {
    std::string action = ""; // "BUY", "SELL", "LIQUIDATE", "MOVE_SL"
    double quantity = 0.0;
    double price = 0.0;
    double take_profit = 0.0;
    double stop_loss = 0.0;
    double new_sl = 0.0;  // For MOVE_SL action
};

struct StrategyBaseConfig {
    // Time settings
    int trading_from_hour = 7;
    int trading_from_minute = 0;
    int trading_to_hour = 23;
    int trading_to_minute = 0;
    std::vector<int> trading_days = {0, 1, 2, 3, 4};  // 0=Monday, 6=Sunday
    
    // Fixed SL/TP values as fallback
    double take_profit_distance = 30.0;
    double stop_loss_distance = 20.0;
    
    // ATR parameters
    bool use_atr_for_sl_tp = false;
    int atr_period = 14;
    double stop_loss_atr_multiplier = 2.0;
    double take_profit_atr_multiplier = 3.0;
    
    // Minimum values to avoid too tight SL/TP
    double min_stop_loss_distance = 5.0;
    double min_take_profit_distance = 5.0;
    
    // Risk management
    bool use_risk_based_sizing = false;
    double risk_percentage = 1.0;
    double cash = 100000.0;
    double max_position_percentage = 100.0;
    double leverage_limit = 20.0;
    
    // Break-even parameters
    bool use_break_even = false;
    double break_even_threshold = 0.7;
};

class Strategy {
protected:
    StrategyBaseConfig base_config;
    std::vector<Candle> buffer;
    Candle current_candle;
    bool in_position = false;
    double entry_price = 0.0;
    double position_size = 0.0;
    double position_pl_pct = 0.0;
    
    // Signal components
    double buy_quantity = 0.0;
    double buy_price = 0.0;
    double sell_quantity = 0.0;
    double sell_price = 0.0;
    double take_profit_distance = 0.0;
    double stop_loss_distance = 0.0;
    
    // Execution control
    bool is_executing = false;
    std::unique_ptr<Signal> signal;
    
    // Cache for time checking
    std::string last_check_date;
    bool weekday_check = false;
    bool time_check = false;
    
    // Utility methods
    bool check_time() {
        if (current_candle.date.empty()) return false;
        
        // Parse date string to time components - assuming ISO format
        if (current_candle.date != last_check_date) {
            last_check_date = current_candle.date;
            
            // Parse date to get day of week (0=Monday, 6=Sunday)
            std::tm tm = {};
            std::istringstream ss(current_candle.date.substr(0, 19));
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            
            if (ss.fail()) {
                return false;
            }
            
            std::time_t time = std::mktime(&tm);
            std::tm* local_tm = std::localtime(&time);
            int weekday = local_tm->tm_wday;
            // Convert Sunday=0 to Sunday=6
            weekday = (weekday == 0) ? 6 : weekday - 1;
            
            // Check if current day is a trading day
            weekday_check = std::find(base_config.trading_days.begin(), 
                                     base_config.trading_days.end(), 
                                     weekday) != base_config.trading_days.end();
            if (!weekday_check) {
                return false;
            }
            
            // Check trading hours
            int hour = local_tm->tm_hour;
            int minute = local_tm->tm_min;
            
            bool after_start = (hour > base_config.trading_from_hour || 
                               (hour == base_config.trading_from_hour && 
                                minute >= base_config.trading_from_minute));
            
            bool before_end = (hour < base_config.trading_to_hour || 
                              (hour == base_config.trading_to_hour && 
                               minute <= base_config.trading_to_minute));
                               
            time_check = after_start && before_end;
            return time_check;
        }
        
        // Use cached result
        return weekday_check && time_check;
    }
    
    std::unique_ptr<Signal> check_break_even() {
        if (!in_position || !base_config.use_break_even) {
            return nullptr;
        }
        
        if (entry_price == 0.0 || position_pl_pct == 0.0) {
            return nullptr;
        }
        
        // Calculate threshold based on take profit distance
        double threshold_pct = (base_config.take_profit_distance / entry_price) * 100.0;
        
        // Check if we've reached the threshold to activate break-even
        if (position_pl_pct > (base_config.break_even_threshold * threshold_pct)) {
            auto be_signal = std::make_unique<Signal>();
            be_signal->action = "MOVE_SL";
            be_signal->new_sl = entry_price;
            return be_signal;
        }
        
        return nullptr;
    }
    
    std::unique_ptr<Signal> generate_buy_signal() {
        auto sig = std::make_unique<Signal>();
        sig->action = "BUY";
        sig->quantity = buy_quantity;
        sig->price = buy_price;
        sig->take_profit = take_profit_distance;
        sig->stop_loss = stop_loss_distance;
        return sig;
    }
    
    std::unique_ptr<Signal> generate_sell_signal() {
        auto sig = std::make_unique<Signal>();
        sig->action = "SELL";
        sig->quantity = sell_quantity;
        sig->price = sell_price;
        sig->take_profit = take_profit_distance;
        sig->stop_loss = stop_loss_distance;
        return sig;
    }
    
    std::unique_ptr<Signal> generate_liquidation_signal() {
        auto sig = std::make_unique<Signal>();
        sig->action = "LIQUIDATE";
        return sig;
    }
    
    void reset() {
        buy_quantity = 0.0;
        buy_price = 0.0;
        sell_quantity = 0.0;
        sell_price = 0.0;
        take_profit_distance = 0.0;
        stop_loss_distance = 0.0;
        signal = nullptr;
    }
    
    void execute_long() {
        go_long();
        
        if (buy_quantity <= 0.0 || buy_price <= 0.0) {
            throw std::runtime_error("Buy parameters not properly set");
        }
        
        signal = generate_buy_signal();
    }
    
    void execute_short() {
        go_short();
        
        if (sell_quantity <= 0.0 || sell_price <= 0.0) {
            throw std::runtime_error("Sell parameters not properly set");
        }
        
        signal = generate_sell_signal();
    }
    
    bool execute_filters() {
        for (const auto& filter : filters()) {
            if (!filter()) {
                return false;
            }
        }
        return true;
    }
    
    void execute() {
        if (is_executing) {
            return;
        }
        
        is_executing = true;
        
        // Quick time check before executing anything else
        if (!check_time()) {
            signal = generate_liquidation_signal();
            is_executing = false;
            return;
        }
        
        before();
        
        bool should_long_val = should_long();
        bool should_short_val = should_long_val ? false : should_short();
        
        if (!should_long_val && !should_short_val) {
            reset();
            is_executing = false;
            return;
        }
        
        if (!execute_filters()) {
            reset();
            is_executing = false;
            return;
        }
        
        if (should_long_val) {
            execute_long();
        } else {
            execute_short();
        }
        
        after();
        is_executing = false;
    }

public:
    // Constructor
    Strategy(const StrategyBaseConfig& config) 
        : base_config(config), signal(std::make_unique<Signal>()) {}
    
    virtual ~Strategy() = default;
    
    // Core strategy methods to implement in derived classes
    virtual void before() {}
    virtual void after() {}
    virtual bool should_long() = 0;
    virtual bool should_short() { return false; }
    virtual void go_long() = 0;
    virtual void go_short() {
        throw std::runtime_error("Short not implemented");
    }
    
    // Default implementation for filters
    virtual std::vector<std::function<bool()>> filters() {
        return {};
    }
    
    // Main update method
    Signal* update_candle(const Candle& candle) {
        // Store the current candle
        current_candle = candle;
        
        // Update position information
        in_position = candle.in_position;
        entry_price = candle.entry_price;
        position_size = candle.position_size;
        position_pl_pct = candle.position_pl_pct;
        
        // Add to buffer for historical calculations
        buffer.push_back(candle);
        if (buffer.size() > 200) {  // Limit buffer size
            buffer.erase(buffer.begin());
        }
        
        // Check for break-even signal before executing strategy
        auto be_signal = check_break_even();
        if (be_signal) {
            signal = std::move(be_signal);
            return signal.get();
        }
        
        // Execute strategy
        execute();
        
        return signal.get();
    }
    
    // Properties
    double price() const {
        return current_candle.close;
    }
    
    double get_indicator_value(const std::string& indicator_name) const {
        auto it = current_candle.indicators.find(indicator_name);
        if (it != current_candle.indicators.end()) {
            return it->second;
        }
        return 0.0;
    }
};