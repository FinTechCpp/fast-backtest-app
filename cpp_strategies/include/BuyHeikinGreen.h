// BuyHeikinGreen.h
#pragma once
#include "Strategy.h"

struct BuyHeikinGreenConfig {
    int ema_short_period = 50;
    int ema_long_period = 200;
    int stoch_fastk = 10;
    int stoch_slowk = 7;
    int stoch_slowd = 3;
    int stoch_threshold = 50;
    
    bool use_ema_short_filter = true;
    bool use_ema_long_filter = true;
    bool use_stoch_filter = true;
    bool use_previous_ha_candle_red_filter = true;
};

class BuyHeikinGreen : public Strategy {
private:
    BuyHeikinGreenConfig config;
    std::string ema_short_name;
    std::string ema_long_name;
    std::string stoch_k_name;
    std::string stoch_d_name;
    std::string atr_name;
    
    // Cache Heikin Ashi
    struct HACacheItem {
        double open;
        double close;
        bool is_green;
    };
    
    HACacheItem ha_current;
    HACacheItem ha_previous;
    
    // Pour le stochastique
    double k_previous = 0;
    double d_previous = 0;
    
    // Méthodes pour les filtres
    bool ema_short_filter();
    bool ema_long_filter();
    bool previous_ha_candle_red_filter();
    bool stoch_inf_threshold_filter();
    
public:
    BuyHeikinGreen(const StrategyBaseConfig& base_config, 
                  const BuyHeikinGreenConfig& buy_config);
    
    // Implémentation des méthodes abstraites
    bool should_long() override;
    bool should_short() override;
    void go_long() override;
    void go_short() override;
    void before() override;
    std::vector<std::function<bool()>> filters() override;
    Candle add_missing_indicators(const Candle& candle) override;
};