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
    std::string to_string() const;
};

// Niveaux de log
enum LogLevel {
    DEBUG = 3,
    WARNING = 2,
    INFO = 1,
    ERROR = 0
};

// Structure de bougie
struct Candle {
    DateTime date;
    double open;
    double high;
    double low;
    double close;
    double volume = 0.0;
    
    bool in_position = false;
    double entry_price = 0.0;
    double position_size = 0.0;
    double position_pl_pct = 0.0;
    double closed_trade_pnl = 0.0;
};

extern std::function<void(const std::string&, int)> g_py_log_callback;

void cpp_log(const std::string& message, int level = LogLevel::INFO);