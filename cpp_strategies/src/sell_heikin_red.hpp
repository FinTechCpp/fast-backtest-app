#pragma once
#include "strategy.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>

struct SellHeikinRedConfig {
    int ema_short_period = 150;
    int ema_long_period = 198;
    int stoch_fastk = 10;
    int stoch_slowk = 7;
    int stoch_slowd = 3;
    int stoch_threshold = 80;  // Inversé par rapport à BuyHeikinGreen
    
    bool use_ema_short_filter = true;
    bool use_ema_long_filter = true;
    bool use_stoch_filter = true;
    bool use_previous_ha_candle_green_filter = true;  // Inversé
};

class SellHeikinRed : public Strategy {
private:
    SellHeikinRedConfig config;
    
    // Indicator calculators
    std::unique_ptr<EMA> ema_short_calculator;
    std::unique_ptr<EMA> ema_long_calculator;
    std::unique_ptr<STOCH> stochastic_calculator;
    std::unique_ptr<ATR> atr_calculator;
    
    // Indicator names
    std::string ema_short_name;
    std::string ema_long_name;
    std::string stoch_k_name;
    std::string stoch_d_name;
    std::string atr_name;
    
    // Indicator values
    double current_ema_short = 0.0;
    double current_ema_long = 0.0;
    double current_stoch_k = 0.0;
    double current_stoch_d = 0.0;
    double current_atr = 0.0;
    double k_previous = 0.0;
    double d_previous = 0.0;
    
    // Cache for Heikin Ashi candles
    struct HeikinAshiValue {
        double open = 0.0;
        double close = 0.0;
        bool is_green = false;
    };
    
    HeikinAshiValue ha_current;
    HeikinAshiValue ha_previous;
    
    // Filters - inversés par rapport à BuyHeikinGreen
    std::vector<std::function<bool()>> active_filters;
    
    bool ema_short_filter() {
        if (current_ema_short == 0.0) {
            return false;
        }
        return price() < current_ema_short;  // Inversé: < au lieu de >
    }
    
    bool ema_long_filter() {
        if (current_ema_long == 0.0) {
            return false;
        }
        return price() < current_ema_long;  // Inversé: < au lieu de >
    }
    
    bool stoch_above_threshold_filter() {  // Inversé: above au lieu de inf
        if (current_stoch_k == 0.0) {
            return false;
        }
        
        int threshold = config.stoch_threshold;
        bool result = current_stoch_k > threshold || (k_previous > 0.0 && k_previous > threshold);
        
        // Update previous values
        k_previous = current_stoch_k;
        d_previous = current_stoch_d;
        
        return result;
    }
    
    bool previous_ha_candle_green_filter() {  // Inversé: green au lieu de red
        if (buffer.size() < 3) {
            return false;
        }
        
        return ha_previous.is_green;  // Inversé: is_green au lieu de !is_green
    }
    
    bool initialize_indicators() {
        if (buffer.size() < static_cast<size_t>(std::max(config.ema_long_period, config.stoch_fastk + config.stoch_slowk))) {
            return false;
        }
        
        // Extract data for initialization
        std::vector<double> close_history(buffer.size());
        std::vector<double> high_history(buffer.size());
        std::vector<double> low_history(buffer.size());
        
        for (size_t i = 0; i < buffer.size(); ++i) {
            close_history[i] = buffer[i].close;
            high_history[i] = buffer[i].high;
            low_history[i] = buffer[i].low;
        }
        
        // Initialize EMAs
        current_ema_short = ema_short_calculator->initialize_with_history(close_history);
        current_ema_long = ema_long_calculator->initialize_with_history(close_history);
        
        // Initialize Stochastic
        auto stoch_values = stochastic_calculator->initialize_with_history(
            high_history, low_history, close_history
        );
        current_stoch_k = stoch_values.first;
        current_stoch_d = stoch_values.second;
        
        // Initialize ATR
        current_atr = atr_calculator->initialize_with_history(
            high_history, low_history, close_history
        );
        
        return (current_ema_short > 0.0 && 
                current_ema_long > 0.0 && 
                current_stoch_k > 0.0 && 
                current_stoch_d > 0.0);
    }
    
    bool update_indicators() {
        if (buffer.empty()) {
            return false;
        }
        
        if (!ema_short_calculator->initialized() ||
            !ema_long_calculator->initialized() ||
            !stochastic_calculator->initialized() ||
            (base_config.use_atr_for_sl_tp && !atr_calculator->initialized())) {
            
            // Try to initialize indicators if they're not initialized
            if (!initialize_indicators()) {
                return false;
            }
        }
        
        // Update EMAs
        current_ema_short = ema_short_calculator->update(current_candle.close);
        current_ema_long = ema_long_calculator->update(current_candle.close);
        
        // Update Stochastic
        auto stoch_values = stochastic_calculator->update(
            current_candle.high,
            current_candle.low,
            current_candle.close
        );
        current_stoch_k = stoch_values.first;
        current_stoch_d = stoch_values.second;
        
        // Update ATR
        current_atr = atr_calculator->update(
            current_candle.high,
            current_candle.low,
            current_candle.close
        );
        
        return true;
    }

public:
    SellHeikinRed(const StrategyBaseConfig& base_cfg, const SellHeikinRedConfig& shr_cfg) 
        : Strategy(base_cfg), config(shr_cfg) {
        
        // Initialize indicator calculators
        ema_short_calculator = std::make_unique<EMA>(config.ema_short_period);
        ema_long_calculator = std::make_unique<EMA>(config.ema_long_period);
        stochastic_calculator = std::make_unique<STOCH>(
            config.stoch_fastk,
            config.stoch_slowk,
            config.stoch_slowd
        );
        atr_calculator = std::make_unique<ATR>(base_cfg.atr_period);
        
        // Initialize indicator names
        ema_short_name = "EMA_" + std::to_string(config.ema_short_period);
        ema_long_name = "EMA_" + std::to_string(config.ema_long_period);
        stoch_k_name = "STOCH_K_" + std::to_string(config.stoch_fastk) + "_" +
                      std::to_string(config.stoch_slowk) + "_" +
                      std::to_string(config.stoch_slowd);
        stoch_d_name = "STOCH_D_" + std::to_string(config.stoch_fastk) + "_" +
                      std::to_string(config.stoch_slowk) + "_" +
                      std::to_string(config.stoch_slowd);
        atr_name = "ATR_" + std::to_string(base_cfg.atr_period);
        
        // Setup active filters
        if (config.use_ema_short_filter) {
            active_filters.push_back([this]() { return this->ema_short_filter(); });
        }
        if (config.use_ema_long_filter) {
            active_filters.push_back([this]() { return this->ema_long_filter(); });
        }
        if (config.use_stoch_filter) {
            active_filters.push_back([this]() { return this->stoch_above_threshold_filter(); });
        }
        if (config.use_previous_ha_candle_green_filter) {
            active_filters.push_back([this]() { return this->previous_ha_candle_green_filter(); });
        }
    }

    void before() override {
        // Update all technical indicators
        update_indicators();
        
        // Need at least 2 candles for HA calculation
        if (buffer.size() < 2) {
            return;
        }
        
        // Get current and previous candles
        const Candle& current = current_candle;
        const Candle& prev = buffer[buffer.size() - 2];
        
        // If we need to initialize HA candles
        if (ha_current.close == 0.0) {
            // Initialize both candles
            if (buffer.size() >= 3) {
                const Candle& prev2 = buffer[buffer.size() - 3];
                
                // Calculate HA for previous candle
                double ha_close_prev = (prev.open + prev.high + prev.low + prev.close) / 4.0;
                double ha_open_prev = (prev2.open + prev2.close) / 2.0;
                
                ha_previous.open = ha_open_prev;
                ha_previous.close = ha_close_prev;
                ha_previous.is_green = ha_close_prev > ha_open_prev;
            } else {
                // Not enough history, initialize with basic values
                ha_previous.open = prev.open;
                ha_previous.close = prev.close;
                ha_previous.is_green = prev.close > prev.open;
            }
        } else {
            // Move current values to previous (reuse calculations)
            ha_previous = ha_current;
        }
        
        // Calculate HA for current candle
        double ha_close_current = (current.open + current.high + current.low + current.close) / 4.0;
        double ha_open_current = (ha_previous.open + ha_previous.close) / 2.0;
        
        // Update cache for current candle
        ha_current.open = ha_open_current;
        ha_current.close = ha_close_current;
        ha_current.is_green = ha_close_current > ha_open_current;
    }
    
    bool should_long() override {
        return false;  // Cette stratégie ne prend pas de positions longues
    }
    
    bool should_short() override {  // Implémente should_short au lieu de should_long
        if (buffer.size() < 3) {
            return false;
        }
        
        // Vérifier si la bougie actuelle est rouge (pas verte)
        return !ha_current.is_green;
    }
    
    void go_long() override {
        throw std::runtime_error("SellHeikinRed strategy does not support long positions");
    }
    
    void go_short() override {  // Implémente go_short au lieu de go_long
        // Si on utilise l'ATR pour les SL/TP
        if (base_config.use_atr_for_sl_tp && current_atr > 0.0) {
            // Vérifier que l'ATR n'est pas zéro ou négatif
            if (current_atr <= 0.0) {
                current_atr = base_config.min_stop_loss_distance / base_config.stop_loss_atr_multiplier;
            }
            
            // Calculer le SL basé sur l'ATR avec un minimum
            stop_loss_distance = std::max(
                current_atr * base_config.stop_loss_atr_multiplier,
                base_config.min_stop_loss_distance
            );
            
            // Calculer le TP basé sur l'ATR avec un minimum
            take_profit_distance = std::max(
                current_atr * base_config.take_profit_atr_multiplier,
                base_config.min_take_profit_distance
            );
        } else {
            // Utiliser des valeurs fixes par défaut
            stop_loss_distance = base_config.stop_loss_distance;
            take_profit_distance = base_config.take_profit_distance;
        }
        
        // Calculer la taille de position basée sur le risque si activé
        if (base_config.use_risk_based_sizing) {
            double initial_capital = base_config.cash;
            double risk_amount = initial_capital * base_config.risk_percentage / 100.0;
            
            // Calculer la taille de position pour que le SL représente exactement risk_amount
            double risk_based_position_size = risk_amount / stop_loss_distance;
            
            // Utiliser le capital total disponible avec effet de levier
            double leveraged_capital = initial_capital * base_config.leverage_limit;
            
            // Limiter la taille max de position à un pourcentage du capital avec levier
            double max_position_value = leveraged_capital * base_config.max_position_percentage / 100.0;
            double max_position_size = max_position_value / price();
            
            // Prendre le MINIMUM entre la taille basée sur le risque et la limite
            double raw_position_size = std::min(risk_based_position_size, max_position_size);
            
            // Si taille >= 1, arrondir à l'entier le plus proche
            if (raw_position_size >= 1.0) {
                sell_quantity = std::floor(raw_position_size);
            } else {
                // Limiter au minimum de 0.5
                sell_quantity = std::max(0.5, std::min(raw_position_size, 0.99));
            }
        } else {
            // Taille fixe par défaut (entier)
            sell_quantity = 1.0;
        }
        
        sell_price = price();
    }
    
    std::vector<std::function<bool()>> filters() override {
        return active_filters;
    }
};