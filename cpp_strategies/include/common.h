#pragma once
#include <string>
#include <sstream>
#include <functional>

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
    bool operator<(const DateTime& other) const;
    bool operator<=(const DateTime& other) const;
    bool operator>(const DateTime& other) const;
    bool operator>=(const DateTime& other) const;
    std::string to_string() const;
};

// Niveaux de log
enum LogLevel {
    CRITICAL = 50,
    FATAL = CRITICAL,
    ERROR = 40,
    WARNING = 30,
    WARN = WARNING,
    INFO = 20,
    DEBUG = 10,
    NOTSET = 0
};

struct BasicCandle {
    DateTime date;
    double open;
    double high;
    double low;
    double close;

    BasicCandle() = default;

    BasicCandle(const DateTime& dt, double o, double h, double l, double c)
        : date(dt), open(o), high(h), low(l), close(c) {}
};

// Structure de bougie
struct Candle : public BasicCandle {
    bool in_position = false;
    double entry_price = 0.0;
    double position_size = 0.0;
    double position_pl_pct = 0.0;
    double closed_trade_pnl = 0.0;

    Candle() = default;

    Candle(const DateTime& dt, double o, double h, double l, double c)
        : BasicCandle(dt, o, h, l, c) {}

    Candle(const DateTime& dt, double o, double h, double l, double c,
           bool in_pos, double entry_price, double pos_size, double pos_pl_pct)
        : BasicCandle(dt, o, h, l, c), in_position(in_pos), entry_price(entry_price),
          position_size(pos_size), position_pl_pct(pos_pl_pct) {}
};

extern std::function<void(const std::string&, int)> g_py_log_callback;

void cpp_log(const std::string& message, int level = LogLevel::INFO);


struct StrategyBaseConfig {
    // Time settings
    Time trading_from = {7, 0, 0};   // 7:00 AM
    Time trading_to = {23, 0, 0};    // 11:00 PM
    std::vector<int> trading_days = {0, 1, 2, 3, 4};  // 0=Monday, 6=Sunday
    
    // Fixed SL/TP values
    double take_profit_distance = 30.0;
    double stop_loss_distance = 20.0;
    
    // Paramètres ATR pour SL et TP
    bool use_atr_for_sl = false;     // Important: valeur par défaut false
    bool use_atr_for_tp = false;     // Important: valeur par défaut false
    int atr_period = 14;
    double stop_loss_atr_multiplier = 2.0;
    double take_profit_atr_multiplier = 3.0;
    double min_stop_loss_distance = 5.0;
    double min_take_profit_distance = 5.0;
    
    // Nouveaux paramètres Min/Max pour SL
    bool use_minmax_for_sl = false;
    int sl_minmax_periods = 5;
    double sl_minmax_delta = 5.0;
        
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