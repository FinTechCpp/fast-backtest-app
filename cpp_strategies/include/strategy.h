#pragma once
#include "common.h"
#include "CandleManager.hpp"
#include "PositionManager.hpp"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "indicators/indicators.hpp"
#include "strategyLogger.hpp"

// Fonction utilitaire pour parser une chaîne de date ISO
DateTime parse_iso_datetime(const std::string& iso_date);

// Fonction pour obtenir le jour de la semaine (0=lundi, 6=dimanche)
int get_day_of_week(const DateTime& date);

struct Signal {
    std::string action = ""; // "BUY", "SELL", "LIQUIDATE", "MOVE_SL"
    double quantity = 0.0;
    double price = 0.0;
    double take_profit = 0.0;
    double stop_loss = 0.0;
    double new_sl = 0.0;  // For MOVE_SL action
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

protected:
    StrategyBaseConfig base_config;
    CandleManager candle_manager;
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

    std::unique_ptr<StrategyLogger> logger;

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