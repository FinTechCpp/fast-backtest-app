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


struct Time {
    int hour = 0;
    int minute = 0;
    int second = 0;

    bool operator<(const Time& other) const {
        return std::tie(hour, minute, second) < std::tie(other.hour, other.minute, other.second);
    }

    bool operator<=(const Time& other) const {
        return std::tie(hour, minute, second) <= std::tie(other.hour, other.minute, other.second);
    }

    bool operator==(const Time& other) const {
        return hour == other.hour && minute == other.minute && second == other.second;
    }

    bool operator!=(const Time& other) const {
        return !(*this == other);
    }
};

struct DateTime {
    int year = 0;
    int month = 0;
    int day = 0;
    Time time;

    bool is_valid() const {
        return year > 0 && month > 0 && month <= 12 && day > 0 && day <= 31;
    }

    bool operator==(const DateTime& other) const {
        return year == other.year && month == other.month && day == other.day && time == other.time;
    }

    bool operator!=(const DateTime& other) const {
        return !(*this == other);
    }

    // Méthode pour convertir en string (utile pour le débogage)
    std::string to_string() const {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d", 
                year, month, day, time.hour, time.minute, time.second);
        return std::string(buffer);
    }
};

// Fonction utilitaire pour parser une chaîne de date ISO
DateTime parse_iso_datetime(const std::string& iso_date) {
    DateTime result;
    
    // Vérification de la longueur minimale
    if (iso_date.size() < 19) {
        return result;  // Return invalid date
    }
    
    std::tm tm = {};
    std::istringstream ss(iso_date.substr(0, 19));
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        return result;  // Return invalid date
    }
    
    result.year = tm.tm_year + 1900;  // tm_year est années depuis 1900
    result.month = tm.tm_mon + 1;     // tm_mon est 0-11
    result.day = tm.tm_mday;
    result.time.hour = tm.tm_hour;
    result.time.minute = tm.tm_min;
    result.time.second = tm.tm_sec;
    
    return result;
}

// Fonction pour obtenir le jour de la semaine (0=lundi, 6=dimanche)
int get_day_of_week(const DateTime& date) {
    // Formule pour calculer le jour de la semaine
    std::tm timeinfo = {};
    timeinfo.tm_year = date.year - 1900;
    timeinfo.tm_mon = date.month - 1;
    timeinfo.tm_mday = date.day;
    
    std::time_t time = std::mktime(&timeinfo);
    std::tm* local_tm = std::localtime(&time);
    int weekday = local_tm->tm_wday;
    
    // Convertir de Sunday=0 à Sunday=6
    return (weekday == 0) ? 6 : weekday - 1;
}

struct Candle {
    DateTime date;
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
    Time trading_from = {7, 0, 0};   // 7:00 AM
    Time trading_to = {23, 0, 0};    // 11:00 PM
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
    DateTime last_check_date;
    bool weekday_check = false;
    bool time_check = false;
    
    // Méthode pour vérifier si on est dans les horaires de trading
    bool check_time() {
        if (!current_candle.date.is_valid()) {
            return false;
        }
        
        // Vérifier si la date a changé depuis la dernière vérification
        if (current_candle.date != last_check_date) {
            last_check_date = current_candle.date;
            
            // Calculer le jour de la semaine (0=lundi, 6=dimanche)
            int weekday = get_day_of_week(current_candle.date);
            
            // Vérifier si c'est un jour de trading
            weekday_check = std::find(base_config.trading_days.begin(), 
                                     base_config.trading_days.end(), 
                                     weekday) != base_config.trading_days.end();
            
            if (!weekday_check) {
                return false;
            }
            
            // Vérifier les heures de trading
            const Time& current_time = current_candle.date.time;
            
            bool after_start = (base_config.trading_from < current_time || 
                               base_config.trading_from == current_time);
                               
            bool before_end = (current_time < base_config.trading_to || 
                              current_time == base_config.trading_to);
                               
            time_check = after_start && before_end;
            return time_check;
        }
        
        // Utiliser le résultat mis en cache
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