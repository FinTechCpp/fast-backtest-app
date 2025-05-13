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

    bool operator<(const Time& other) const;
    bool operator<=(const Time& other) const;
    bool operator==(const Time& other) const;
    bool operator!=(const Time& other) const;
};

struct DateTime {
    int year = 0;
    int month = 0;
    int day = 0;
    Time time;

    bool is_valid() const;
    bool operator==(const DateTime& other) const;
    bool operator!=(const DateTime& other) const;
    std::string to_string() const;
};

// Fonction utilitaire pour parser une chaîne de date ISO
DateTime parse_iso_datetime(const std::string& iso_date);

// Fonction pour obtenir le jour de la semaine (0=lundi, 6=dimanche)
int get_day_of_week(const DateTime& date);

extern std::function<void(const std::string&, int)> g_py_log_callback;

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

// Fonction de log C++ qui appelle le callback Python
void cpp_log(const std::string& message, int level = LogLevel::INFO);

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
    double closed_trade_pnl = 0.0; // P&L of the last closed trade
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

    // Perte maximale journalière
    bool use_daily_max_loss = false;
    double daily_max_loss_percentage = 2.0;
    double daily_max_loss_amount = 0.0; // Calculé à partir de cash et daily_max_loss_percentage
};

class Strategy {
public:
    // Constructor
    Strategy(const StrategyBaseConfig& config);
    
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
    Signal* update_candle(const Candle& candle);
    
    // Properties
    double price() const;
    double get_indicator_value(const std::string& indicator_name) const;

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

    // Suivi des pertes journalières
    DateTime current_trading_day;
    double daily_pnl = 0.0;
    bool trading_suspended_for_day = false;
    
    // Cache pour le dernier trade
    double last_trade_pnl = 0.0;

    double calculate_trade_risk(bool is_long);
    bool is_trade_risk_acceptable(double risk);
    bool is_new_trading_day();
    void update_daily_pnl_tracking();

    // Méthode pour vérifier si on est dans les horaires de trading
    bool check_time();
    
    std::unique_ptr<Signal> check_break_even();
    std::unique_ptr<Signal> generate_buy_signal();
    std::unique_ptr<Signal> generate_sell_signal();
    std::unique_ptr<Signal> generate_liquidation_signal();
    
    void reset();
    void execute_long();
    void execute_short();
    bool execute_filters();
    void execute();
};