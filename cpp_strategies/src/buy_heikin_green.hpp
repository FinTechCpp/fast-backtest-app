#pragma once
#include "strategy.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>

struct BuyHeikinGreenConfig {
    int ema_short_period = 150;
    int ema_long_period = 198;
    int stoch_fastk = 10;
    int stoch_slowk = 7;
    int stoch_slowd = 3;
    int stoch_threshold = 20;
    int rsi_period = 14;         // Période pour le calcul du RSI
    int rsi_threshold = 30;      // Seuil pour le filtre RSI
    
    bool use_ema_short_filter = false;
    bool use_ema_long_filter = false;
    bool use_stoch_filter = false;
    bool use_rsi_filter = false;
    bool use_previous_ha_candle_red_filter = false;
};

class BuyHeikinGreen : public Strategy {
private:
    BuyHeikinGreenConfig config;
    
    // Indicator calculators
    std::unique_ptr<EMA> ema_short_calculator;
    std::unique_ptr<EMA> ema_long_calculator;
    std::unique_ptr<STOCH> stochastic_calculator;
    std::unique_ptr<RSI> rsi_calculator;
    std::unique_ptr<ATR> atr_calculator;
    
    // Indicator names
    std::string ema_short_name;
    std::string ema_long_name;
    std::string stoch_k_name;
    std::string stoch_d_name;
    std::string rsi_name;
    std::string atr_name;
    
    // Indicator values
    double current_ema_short = 0.0;
    double current_ema_long = 0.0;
    double current_stoch_k = 0.0;
    double current_stoch_d = 0.0;
    double current_rsi = 0.0;
    double previous_rsi = 0.0;
    double previous_2_rsi = 0.0;
    double current_atr = 0.0;
    double k_previous = 0.0;
    double d_previous = 0.0;
    
    // Filters
    std::vector<std::function<bool()>> active_filters;
    
    bool ema_short_filter() {
        if (current_ema_short == 0.0) {
            logger->log_filter_result("EMA Court", false);
            logger->log_filter_detail("EMA Court", "Valeur EMA non calculée (0.0)", LogLevel::DEBUG);
            return false;
        }
        bool result = price() > current_ema_short;
        logger->log_filter_result("EMA Court", result);
        logger->log_indicator_comparison(ema_short_name, price(), current_ema_short, 
                                       result ? ">" : "<=", result, LogLevel::DEBUG);
        return result;
    }
    
    bool ema_long_filter() {
        if (current_ema_long == 0.0) {
            logger->log_filter_result("EMA Long", false);
            logger->log_filter_detail("EMA Long", "Valeur EMA non calculée (0.0)", LogLevel::DEBUG);
            return false;
        }
        bool result = price() > current_ema_long;
        logger->log_filter_result("EMA Long", result);
        logger->log_indicator_comparison(ema_long_name, price(), current_ema_long, 
                                       result ? ">" : "<=", result, LogLevel::DEBUG);
        return result;
    }
    
    bool stoch_inf_threshold_filter() {
        if (current_stoch_k == 0.0) {
            logger->log_filter_result("Stochastique", false);
            logger->log_filter_detail("Stochastique", "Valeur K non calculée (0.0)", LogLevel::DEBUG);
            return false;
        }
        int threshold = config.stoch_threshold;
        bool result = current_stoch_k < threshold || (k_previous > 0.0 && k_previous < threshold);
        
        logger->log_filter_result("Stochastique", result);

        if (current_stoch_k < threshold) {
            logger->log_indicator_comparison(stoch_k_name, current_stoch_k, threshold, 
                                           "<", true, LogLevel::DEBUG);
        } else if (k_previous > 0.0 && k_previous < threshold) {
            logger->log_filter_detail("Stochastique", 
                                   "K précédent (" + std::to_string(k_previous) + 
                                   ") < seuil (" + std::to_string(threshold) + ")", 
                                   LogLevel::DEBUG);
        } else {
            logger->log_indicator_comparison(stoch_k_name, current_stoch_k, threshold, 
                                           ">=", false, LogLevel::DEBUG);
        }

        // Update previous values
        k_previous = current_stoch_k;
        d_previous = current_stoch_d;
        
        return result;
    }
    
    bool rsi_inf_threshold_filter() {
        if (current_rsi == 0.0) {
            logger->log_filter_result("RSI", false);
            logger->log_filter_detail("RSI", "Valeur RSI non calculée (0.0)", LogLevel::DEBUG);
            return false;
        }
    
        int threshold = config.rsi_threshold;
        
        // Vérifier les trois dernières valeurs
        bool current_below = current_rsi < threshold;
        bool prev_below = previous_rsi > 0.0 && previous_rsi < threshold;
        bool prev2_below = previous_2_rsi > 0.0 && previous_2_rsi < threshold;
        
        bool result = current_below || prev_below || prev2_below;
        
        // Journalisation détaillée
        logger->log_filter_result("RSI", result);
        
        if (result) {
            if (current_below) {
                logger->log_indicator_comparison("RSI actuel", current_rsi, threshold, "<", true, LogLevel::DEBUG);
            } else if (prev_below) {
                logger->log_indicator_comparison("RSI précédent", previous_rsi, threshold, "<", true, LogLevel::DEBUG);
            } else {
                logger->log_indicator_comparison("RSI antérieur", previous_2_rsi, threshold, "<", true, LogLevel::DEBUG);
            }
        } else {
            logger->log_filter_detail("RSI", "Actuel: " + std::to_string(current_rsi) + 
                                         ", Précédent: " + std::to_string(previous_rsi) + 
                                         ", Antérieur: " + std::to_string(previous_2_rsi) + 
                                         " - Tous au-dessus du seuil " + std::to_string(threshold), 
                                         LogLevel::DEBUG);
        }
        
        // Mettre à jour les valeurs historiques
        previous_2_rsi = previous_rsi;
        previous_rsi = current_rsi;
        
        return result;
    }
    
    bool previous_ha_candle_red_filter() {
        try {
            // Utiliser CandleManager pour obtenir l'information Heikin Ashi
            if (candle_manager.size() < 3) {
                logger->log_filter_result("Bougie HA précédente", false);
                logger->log_filter_detail("Bougie HA précédente", "Pas assez d'historique (min 3 bougies)", LogLevel::DEBUG);
                return false;
            }
            
            // Récupérer les bougies HA
            auto ha_candles = candle_manager.get_last_heikin_ashi_candles(2);
            if (ha_candles.size() < 2) {
                logger->log_filter_result("Bougie HA précédente", false);
                logger->log_filter_detail("Bougie HA précédente", "Pas assez de bougies HA", LogLevel::DEBUG);
                return false;
            }
            
            // La bougie précédente est à l'index 0 (l'avant-dernière)
            const BasicCandle& prev_ha = ha_candles[0];
            bool is_red = prev_ha.close < prev_ha.open;
            
            logger->log_filter_result("Bougie HA précédente", is_red);
            logger->log_filter_detail("Bougie HA précédente", 
                                  "Bougie précédente " + std::string(is_red ? "ROUGE" : "VERTE") + 
                                  " (open=" + std::to_string(prev_ha.open) + 
                                  ", close=" + std::to_string(prev_ha.close) + ")", 
                                  LogLevel::DEBUG);
            
            return is_red;
        } catch (const std::exception& e) {
            logger->log_filter_result("Bougie HA précédente", false);
            logger->log_filter_detail("Bougie HA précédente", "Erreur: " + std::string(e.what()), LogLevel::ERROR);
            return false;
        }
    }
    
    bool initialize_indicators() {
        if (candle_manager.size() < static_cast<size_t>(std::max({config.ema_long_period, config.stoch_fastk + config.stoch_slowk, config.rsi_period * 2}))) {
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
        
        // Initialize RSI
        current_rsi = rsi_calculator->initialize_with_history(close_history);
        
        // Initialize ATR
        current_atr = atr_calculator->initialize_with_history(
            high_history, low_history, close_history
        );
        
        return (current_ema_short > 0.0 && 
                current_ema_long > 0.0 && 
                current_stoch_k > 0.0 && 
                current_stoch_d > 0.0 &&
                current_rsi > 0.0);
    }
    
    bool update_indicators() {
        if (candle_manager.size() == 0) {
            logger->log_general("Buffer vide, impossible de mettre à jour les indicateurs", LogLevel::WARNING);
            return false;
        }
        
        if (!ema_short_calculator->initialized() ||
            !ema_long_calculator->initialized() ||
            !stochastic_calculator->initialized() ||
            !rsi_calculator->initialized() ||
            ((base_config.use_atr_for_sl || base_config.use_atr_for_tp) && !atr_calculator->initialized())) {

            logger->log_general("Initialisation des indicateurs requise", LogLevel::INFO);
            
            // Try to initialize indicators if they're not initialized
            if (!initialize_indicators()) {
                logger->log_general("Échec de l'initialisation des indicateurs", LogLevel::WARNING);
                return false;
            }

            logger->log_general("Indicateurs initialisés avec succès", LogLevel::INFO);
        }
        
        // Update EMAs
        current_ema_short = ema_short_calculator->update(current_candle.close);
        current_ema_long = ema_long_calculator->update(current_candle.close);

        logger->log_indicator_value(ema_short_name, current_ema_short);
        logger->log_indicator_value(ema_long_name, current_ema_long);
        
        // Update Stochastic
        auto stoch_values = stochastic_calculator->update(
            current_candle.high,
            current_candle.low,
            current_candle.close
        );
        current_stoch_k = stoch_values.first;
        current_stoch_d = stoch_values.second;

        logger->log_indicator_value(stoch_k_name, current_stoch_k);
        logger->log_indicator_value(stoch_d_name, current_stoch_d);
        
        // Update RSI
        current_rsi = rsi_calculator->update(current_candle.close);
        logger->log_indicator_value(rsi_name, current_rsi);
        
        // Update ATR
        current_atr = atr_calculator->update(
            current_candle.high,
            current_candle.low,
            current_candle.close
        );
        logger->log_indicator_value(atr_name, current_atr);
        
        return true;
    }

public:
    BuyHeikinGreen(const StrategyBaseConfig& base_cfg, const BuyHeikinGreenConfig& bhg_cfg) 
        : Strategy(base_cfg), config(bhg_cfg) {
        
        // Initialize indicator calculators
        ema_short_calculator = std::make_unique<EMA>(config.ema_short_period);
        ema_long_calculator = std::make_unique<EMA>(config.ema_long_period);
        stochastic_calculator = std::make_unique<STOCH>(
            config.stoch_fastk,
            config.stoch_slowk,
            config.stoch_slowd
        );
        rsi_calculator = std::make_unique<RSI>(config.rsi_period);
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
        rsi_name = "RSI_" + std::to_string(config.rsi_period);
        atr_name = "ATR_" + std::to_string(base_cfg.atr_period);
        
        // Setup active filters
        if (config.use_ema_short_filter) {
            active_filters.push_back([this]() { return this->ema_short_filter(); });
        }
        if (config.use_ema_long_filter) {
            active_filters.push_back([this]() { return this->ema_long_filter(); });
        }
        if (config.use_stoch_filter) {
            active_filters.push_back([this]() { return this->stoch_inf_threshold_filter(); });
        }
        if (config.use_rsi_filter) {
            active_filters.push_back([this]() { return this->rsi_inf_threshold_filter(); });
        }
        if (config.use_previous_ha_candle_red_filter) {
            active_filters.push_back([this]() { return this->previous_ha_candle_red_filter(); });
        }
    }

    void before() override {
        // Update all technical indicators
        update_indicators();
        
        // Obtenir la dernière bougie HA pour journalisation
        try {
            BasicCandle ha_current = candle_manager.get_latest_heikin_ashi();
            bool is_green = candle_manager.is_candle_green(ha_current);
            
            logger->log_general("Bougie HA courante calculée: Open=" + std::to_string(ha_current.open) + 
                            ", Close=" + std::to_string(ha_current.close) + 
                            ", Green=" + std::to_string(is_green), LogLevel::INFO);
        } catch (const std::exception& e) {
            logger->log_general("Erreur lors de la récupération des bougies HA: " + std::string(e.what()), LogLevel::ERROR);
        }
    }
    
    bool should_long() override {
        if (candle_manager.size() < 3) {
            logger->log_general("Pas assez d'historique (min 3 bougies)", LogLevel::WARNING);
            return false;
        }
        
        // Vérifier si les indicateurs sont prêts
        if (current_ema_short == 0.0 || current_ema_long == 0.0 || current_stoch_k == 0.0 || current_rsi == 0.0) {
            logger->log_general("Les indicateurs ne sont pas tous initialisés", LogLevel::WARNING);
            return false;
        }
        
        // Vérifier si on a besoin d'ATR mais que celui-ci n'est pas disponible
        bool needs_atr = base_config.use_atr_for_sl || base_config.use_atr_for_tp;
        if (needs_atr && current_atr <= 0.0) {
            logger->log_general("ATR requis mais non disponible", LogLevel::WARNING);
            return false;
        }
        
        // Vérifier si on a besoin de Min/Max mais qu'on n'a pas assez d'historique
        if (base_config.use_minmax_for_sl && candle_manager.size() < base_config.sl_minmax_periods) {
            logger->log_general("Pas assez d'historique pour le calcul Min/Max SL", LogLevel::WARNING);
            return false;
        }
        
        try {
            // Utiliser CandleManager pour vérifier si la bougie HA actuelle est verte
            bool is_current_ha_green = candle_manager.is_latest_heikin_ashi_green();
            
            // Log pour faciliter le débogage
            logger->log_general("Bougie HA courante: " + std::string(is_current_ha_green ? "VERTE" : "ROUGE"), LogLevel::INFO);
            
            // Only check if current candle is green
            return is_current_ha_green;
        } catch (const std::exception& e) {
            logger->log_general("Erreur lors de la vérification de la bougie HA: " + std::string(e.what()), LogLevel::ERROR);
            return false;
        }
    }
    
    void go_long() override {
        logger->log_general("Préparation d'un signal LONG", LogLevel::INFO);

        // Calcul du Stop Loss
        if (base_config.use_atr_for_sl && current_atr > 0.0) {
            logger->log_general("Utilisation de l'ATR pour calculer SL", LogLevel::INFO);

            // Vérification ATR
            if (current_atr <= 0.0) {
                current_atr = base_config.min_stop_loss_distance / base_config.stop_loss_atr_multiplier;
                logger->log_general("ATR non valide, utilisation d'une valeur de secours: " + 
                    std::to_string(current_atr), LogLevel::WARNING);
            }

            // Transformation logarithmique pour SL
            double log_atr = std::log(1.0 + current_atr);
            
            // Calcul SL basé sur ATR avec minimum
            stop_loss_distance = std::max(
                log_atr * base_config.stop_loss_atr_multiplier,
                base_config.min_stop_loss_distance
            );
            
            logger->log_general("SL calculé avec ATR: " + std::to_string(stop_loss_distance), LogLevel::INFO);
        } 
        else if (base_config.use_minmax_for_sl && buffer.size() >= base_config.sl_minmax_periods) {
            logger->log_general("Utilisation de Min/Max pour calculer SL (LONG)", LogLevel::INFO);
            
            // Recherche du minimum sur les n dernières périodes
            double min_price = current_candle.low;
            int n_periods = std::min(static_cast<int>(buffer.size()), base_config.sl_minmax_periods);
            
            for (int i = 1; i < n_periods; ++i) {
                min_price = std::min(min_price, buffer[buffer.size() - i - 1].low);
            }
            
            logger->log_general("Prix minimum trouvé: " + std::to_string(min_price), LogLevel::INFO);
            
            // SL = minimum - delta (pour LONG, le SL est sous le minimum)
            double sl_price = min_price - base_config.sl_minmax_delta;
            stop_loss_distance = price() - sl_price;
            
            // Assurer une distance minimale
            if (stop_loss_distance <= 0.0 || sl_price >= price()) {
                logger->log_general("SL Min/Max calculé invalide, utilisation de distance fixe", LogLevel::WARNING);
                stop_loss_distance = base_config.stop_loss_distance;
            }
            
            logger->log_general("SL Min/Max calculé: " + std::to_string(stop_loss_distance) + 
                              " (min=" + std::to_string(min_price) + 
                              ", delta=" + std::to_string(base_config.sl_minmax_delta) + 
                              ", prix SL=" + std::to_string(sl_price) + ")", 
                              LogLevel::INFO);
        } 
        else {
            // Utiliser valeur fixe pour SL
            stop_loss_distance = base_config.stop_loss_distance;
            logger->log_general("Utilisation de valeur fixe pour SL: " + 
                std::to_string(stop_loss_distance), LogLevel::INFO);
        }
        
        // Calcul du Take Profit
        if (base_config.use_atr_for_tp && current_atr > 0.0) {
            logger->log_general("Utilisation de l'ATR pour calculer TP", LogLevel::INFO);

            // Vérification ATR (uniquement si pas déjà fait)
            if (current_atr <= 0.0) {
                current_atr = base_config.min_take_profit_distance / base_config.take_profit_atr_multiplier;
                logger->log_general("ATR non valide, utilisation d'une valeur de secours: " + 
                    std::to_string(current_atr), LogLevel::WARNING);
            }

            // Transformation logarithmique pour TP
            double log_atr = std::log(1.0 + current_atr);
            
            // Calcul TP basé sur ATR avec minimum
            take_profit_distance = std::max(
                log_atr * base_config.take_profit_atr_multiplier,
                base_config.min_take_profit_distance
            );
            
            logger->log_general("TP calculé avec ATR: " + std::to_string(take_profit_distance), LogLevel::INFO);
        } else {
            // Utiliser valeur fixe pour TP
            take_profit_distance = base_config.take_profit_distance;
            logger->log_general("Utilisation de valeur fixe pour TP: " + 
                std::to_string(take_profit_distance), LogLevel::INFO);
        }
        
        // Calculate position size based on risk if enabled
        if (base_config.use_risk_based_sizing) {
            double initial_capital = base_config.cash;
            double risk_percentage = base_config.risk_percentage;
            double risk_amount = initial_capital * risk_percentage / 100.0;

            logger->log_risk_calculation(risk_amount, risk_percentage);
            
            // Calculate position size for SL to represent exactly risk_amount
            double risk_based_position_size = risk_amount / stop_loss_distance;

            logger->log_position_sizing(risk_based_position_size, risk_based_position_size, 
                "basé sur le risque", LogLevel::DEBUG);
            
            // Use total available capital with leverage
            double leveraged_capital = initial_capital * base_config.leverage_limit;
            
            // Limit max position size to a percentage of capital with leverage
            double max_position_value = leveraged_capital * base_config.max_position_percentage / 100.0;
            double max_position_size = max_position_value / price();

            logger->log_position_sizing(max_position_size, max_position_size, 
                "limite maximale", LogLevel::DEBUG);
            
            // Take the MINIMUM between risk-based size and limit
            double raw_position_size = std::min(risk_based_position_size, max_position_size);
            
            // If size >= 1, round to nearest integer
            if (raw_position_size >= 1.0) {
                buy_quantity = std::floor(raw_position_size);
                logger->log_position_sizing(raw_position_size, buy_quantity, 
                                         "arrondi à l'entier inférieur", LogLevel::INFO);
            } else {
                // Limit to minimum of 0.5
                buy_quantity = std::max(0.5, std::min(raw_position_size, 0.99));
                logger->log_position_sizing(raw_position_size, buy_quantity, 
                                         "limité entre 0.5 et 0.99", LogLevel::INFO);
            }
        } else {
            // Fixed default size (integer)
            buy_quantity = 1.0;
            logger->log_position_sizing(1.0, 1.0, "taille fixe", LogLevel::INFO);
        }
        
        buy_price = price();
        logger->log_sl_tp(stop_loss_distance, take_profit_distance);
    }
    
    void go_short() override {
        throw std::runtime_error("BuyHeikinGreen strategy does not support short selling");
    }
    
    std::vector<std::function<bool()>> filters() override {
        return active_filters;
    }
};