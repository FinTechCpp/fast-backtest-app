// Strategy.h
#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <unordered_map>

struct Candle {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    std::unordered_map<std::string, double> indicators;
};

struct Signal {
    std::string action; // "BUY", "SELL", "LIQUIDATE"
    double quantity;
    double price;
    double take_profit;
    double stop_loss;
};

class StrategyBaseConfig {
public:
    // Équivalent de la configuration Python
    int trading_from_hour = 15;
    int trading_from_min = 30;
    int trading_to_hour = 21;
    int trading_to_min = 58;
    std::vector<int> trading_days = {0, 1, 2, 3, 4}; // Lundi à vendredi
    
    double take_profit_distance = 30.0;
    double stop_loss_distance = 20.0;
    
    bool use_atr_for_sl_tp = false;
    int atr_period = 14;
    double stop_loss_atr_multiplier = 2.0;
    double take_profit_atr_multiplier = 3.0;
    
    double min_stop_loss_distance = 5.0;
    double min_take_profit_distance = 5.0;
    
    bool use_risk_based_sizing = false;
    double risk_percentage = 1.0;
    double cash = 100000.0;
    double max_position_percentage = 100.0;
    double leverage_limit = 20.0;
};

class Strategy {
public:
    Strategy(const StrategyBaseConfig& config);
    virtual ~Strategy();
    
    // Méthodes à implémenter dans les classes dérivées
    virtual bool should_long() = 0;
    virtual bool should_short() = 0;
    virtual void go_long() = 0;
    virtual void go_short() = 0;
    
    // Méthodes communes
    virtual void before();
    virtual void after();
    virtual std::vector<std::function<bool()>> filters();
    virtual Candle add_missing_indicators(const Candle& candle);
    
    // Méthode principale qui sera appelée par Python
    Signal* update_candle(const Candle& candle);
    
    // Helpers
    bool check_time();
    void execute();
    double get_price() const;
    Signal* generate_buy_signal();
    Signal* generate_sell_signal();
    Signal* generate_liquidation_signal();

protected:
    StrategyBaseConfig base_config;
    std::string name;
    double buy_quantity = 0;
    double buy_price = 0;
    double sell_quantity = 0;
    double sell_price = 0;
    double stop_loss_distance = 0;
    double take_profit_distance = 0;
    Signal* current_signal = nullptr;
    
    // État interne
    std::vector<Candle> buffer;
    Candle current_candle;
};