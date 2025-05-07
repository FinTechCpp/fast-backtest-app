#pragma once
#include "strategy.hpp"
#include <algorithm>
#include <cmath>

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
    struct HeikinAshiValue {
        double open = 0.0;
        double close = 0.0;
        bool is_green = false;
    };
    
    HeikinAshiValue ha_current;
    HeikinAshiValue ha_previous;
    
    double k_previous = 0.0;
    double d_previous = 0.0;
    
    bool check_filters() {
        // Vérification des filtres actifs
        if (config.use_ema_short_filter && 
            !ema_short_filter()) {
            return false;
        }
        
        if (config.use_ema_long_filter && 
            !ema_long_filter()) {
            return false;
        }
        
        if (config.use_stoch_filter && 
            !stoch_filter()) {
            return false;
        }
        
        if (config.use_previous_ha_candle_red_filter && 
            !previous_ha_candle_red_filter()) {
            return false;
        }
        
        return true;
    }
    
    bool ema_short_filter() {
        if (ema_short_name.empty() || 
            current_candle.indicators.find(ema_short_name) == current_candle.indicators.end()) {
            return false;
        }
        return price > current_candle.indicators[ema_short_name];
    }
    
    bool ema_long_filter() {
        if (ema_long_name.empty() || 
            current_candle.indicators.find(ema_long_name) == current_candle.indicators.end()) {
            return false;
        }
        return price > current_candle.indicators[ema_long_name];
    }
    
    bool stoch_filter() {
        if (stoch_k_name.empty() || 
            current_candle.indicators.find(stoch_k_name) == current_candle.indicators.end()) {
            return false;
        }
        
        double threshold = config.stoch_threshold;
        double current_k = current_candle.indicators[stoch_k_name];
        
        return current_k < threshold || (k_previous > 0 && k_previous < threshold);
    }
    
    bool previous_ha_candle_red_filter() {
        return !ha_previous.is_green;
    }

public:
    BuyHeikinGreen(const StrategyBaseConfig& base_cfg, const BuyHeikinGreenConfig& bhg_cfg) 
        : Strategy(base_cfg), config(bhg_cfg) {
        // Initialiser les noms des indicateurs
        ema_short_name = "EMA_" + std::to_string(config.ema_short_period);
        ema_long_name = "EMA_" + std::to_string(config.ema_long_period);
        stoch_k_name = "STOCH_K_" + std::to_string(config.stoch_fastk) + "_" +
                      std::to_string(config.stoch_slowk) + "_" +
                      std::to_string(config.stoch_slowd);
        stoch_d_name = "STOCH_D_" + std::to_string(config.stoch_fastk) + "_" +
                      std::to_string(config.stoch_slowk) + "_" +
                      std::to_string(config.stoch_slowd);
        atr_name = "ATR_" + std::to_string(base_cfg.atr_period);
    }

    void before() override {
        // On a besoin d'au moins 2 bougies pour calculer les valeurs HA
        if (buffer.size() < 2) return;
        
        // Récupérer la bougie actuelle et la précédente
        const Candle& current = current_candle;
        const Candle& prev = buffer[buffer.size() - 2];
        
        // Si c'est la première fois qu'on calcule ou après réinitialisation
        if (ha_current.close == 0.0) {
            // Initialiser les deux bougies
            if (buffer.size() >= 3) {
                const Candle& prev2 = buffer[buffer.size() - 3];
                
                // Calculer HA pour la bougie précédente
                double ha_close_prev = (prev.open + prev.high + prev.low + prev.close) / 4;
                double ha_open_prev = (prev2.open + prev2.close) / 2;
                
                ha_previous.open = ha_open_prev;
                ha_previous.close = ha_close_prev;
                ha_previous.is_green = ha_close_prev > ha_open_prev;
            } else {
                // Si pas assez d'historique
                ha_previous.open = prev.open;
                ha_previous.close = prev.close;
                ha_previous.is_green = prev.close > prev.open;
            }
        } else {
            // Déplacer les valeurs actuelles vers précédentes
            ha_previous = ha_current;
        }
        
        // Calculer HA pour la bougie actuelle
        double ha_close_current = (current.open + current.high + current.low + current.close) / 4;
        double ha_open_current = (ha_previous.open + ha_previous.close) / 2;
        
        ha_current.open = ha_open_current;
        ha_current.close = ha_close_current;
        ha_current.is_green = ha_close_current > ha_open_current;
        
        // Mettre à jour les valeurs du stochastique
        if (current_candle.indicators.find(stoch_k_name) != current_candle.indicators.end() &&
            current_candle.indicators.find(stoch_d_name) != current_candle.indicators.end()) {
            k_previous = current_candle.indicators[stoch_k_name];
            d_previous = current_candle.indicators[stoch_d_name];
        }
    }
    
    bool should_long() override {
        // Vérifier s'il y a assez de données
        if (buffer.size() < 3) {
            return false;
        }
        
        // Vérifier si la bougie actuelle est verte
        if (!ha_current.is_green) {
            return false;
        }
        
        // Vérifier tous les filtres
        return check_filters();
    }
    
    void go_long() override {
        // Récupérer l'ATR si disponible
        double current_atr = 0.0;
        if (base_config.use_atr_for_sl_tp && 
            current_candle.indicators.find(atr_name) != current_candle.indicators.end()) {
            current_atr = current_candle.indicators[atr_name];
        }
        
        // Calculer les distances SL/TP
        if (base_config.use_atr_for_sl_tp && current_atr > 0) {
            stop_loss_distance = std::max(
                current_atr * base_config.stop_loss_atr_multiplier,
                base_config.min_stop_loss_distance
            );
            
            take_profit_distance = std::max(
                current_atr * base_config.take_profit_atr_multiplier,
                base_config.min_take_profit_distance
            );
        } else {
            // Valeurs par défaut
            stop_loss_distance = base_config.stop_loss_distance;
            take_profit_distance = base_config.take_profit_distance;
        }
        
        // Calculer la taille de position
        if (base_config.use_risk_based_sizing) {
            double initial_capital = base_config.cash;
            double risk_amount = initial_capital * base_config.risk_percentage / 100.0;
            
            // Taille basée sur le risque
            double risk_based_position_size = risk_amount / stop_loss_distance;
            
            // Taille maximale basée sur le capital
            double leveraged_capital = initial_capital * base_config.leverage_limit;
            double max_position_size = leveraged_capital / price;
            
            // Prendre le minimum
            double raw_position_size = std::min(risk_based_position_size, max_position_size);
            
            // Arrondir selon la valeur
            if (raw_position_size >= 1.0) {
                buy_quantity = std::floor(raw_position_size);
            } else {
                buy_quantity = std::max(0.5, std::min(raw_position_size, 0.99));
            }
        } else {
            // Taille fixe par défaut
            buy_quantity = 1.0;
        }
    }
};