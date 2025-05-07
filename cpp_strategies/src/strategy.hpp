#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

struct Candle {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    std::unordered_map<std::string, double> indicators;
};

struct Signal {
    std::string action = ""; // "BUY", "SELL", "LIQUIDATE" ou vide
    double quantity = 0.0;
    double price = 0.0;
    double take_profit = 0.0;
    double stop_loss = 0.0;
};

struct StrategyBaseConfig {
    // Configuration de base
    std::vector<int> trading_days = {0, 1, 2, 3, 4};
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
    double leverage_limit = 20.0;
};

class Strategy {
protected:
    StrategyBaseConfig base_config;
    std::vector<Candle> buffer;
    Candle current_candle;
    double price = 0.0;
    
    // Pour signaux d'achat/vente
    double buy_quantity = 0.0;
    double take_profit_distance = 0.0;
    double stop_loss_distance = 0.0;
    
    std::unique_ptr<Signal> signal;

public:
    Strategy(const StrategyBaseConfig& config) : base_config(config), signal(new Signal()) {}
    virtual ~Strategy() = default;
    
    // Méthodes à implémenter dans les stratégies spécifiques
    virtual void before() {}
    virtual bool should_long() = 0;
    virtual bool should_short() { return false; }
    virtual void go_long() = 0;
    virtual void go_short() { throw std::runtime_error("Short not implemented"); }
    
    // Méthode principale
    Signal* update_candle(const Candle& candle) {
        // Sauvegarder la bougie actuelle
        current_candle = candle;
        
        // Ajouter la bougie au buffer
        buffer.push_back(candle);
        if (buffer.size() > 200) {  // Limiter la taille du buffer
            buffer.erase(buffer.begin());
        }
        
        // Mettre à jour le prix actuel
        price = candle.close;
        
        // Réinitialiser le signal
        signal->action = "";
        signal->quantity = 0.0;
        signal->price = 0.0;
        signal->take_profit = 0.0;
        signal->stop_loss = 0.0;
        
        // Exécuter les phases de la stratégie
        before();
        
        // Vérifier les conditions d'achat/vente
        if (should_long()) {
            go_long();
            signal->action = "BUY";
            signal->quantity = buy_quantity;
            signal->price = price;
            signal->take_profit = take_profit_distance;
            signal->stop_loss = stop_loss_distance;
        } 
        else if (should_short()) {
            go_short();
            signal->action = "SELL";
            // Configurer les propriétés du signal...
        }
        
        return signal.get();
    }
};