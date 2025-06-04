#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
// #include "Strategies/buy_heikin_green.hpp"
#include "sell_heikin_red.hpp"
#include <memory>
#include <iostream>

/**
 * @brief Adaptateur permettant d'utiliser la stratégie SellHeikinRed avec le moteur de backtest C++
 * 
 * Cette classe sert d'interface entre la stratégie SellHeikinRed (qui utilise sa propre structure)
 * et le moteur de backtest C++ qui attend une classe dérivée de Strategy.
 */
class SellHeikinRedAdapter : public be::Strategy {
private:
    // Configuration spécifique à la stratégie
    SellHeikinRedConfig strategy_config;

    // Instance de la stratégie SellHeikinRed
    std::unique_ptr<SellHeikinRed> strategy;

    // Cache pour les signaux de trading
    bool should_enter_long = false;
    bool should_enter_short = false;
    
    // Pour suivre les trades fermés
    int last_closed_trade_count = 0;
    bool last_trade_closed = false;
    double last_trade_pnl = 0.0;
    
public:
    /**
     * @brief Constructeur de l'adaptateur
     * 
     * @param broker Broker utilisé par le backtest
     * @param data Données historiques utilisées par le backtest
     * @param base_config Configuration de base commune à toutes les stratégies
     * @param shr_config Configuration spécifique à SellHeikinRed
     */
    SellHeikinRedAdapter(
        std::shared_ptr<be::Broker> broker, 
        std::shared_ptr<be::Data> data,
        const StrategyBaseConfig& base_config,
        const SellHeikinRedConfig& shr_config
    ) : be::Strategy(broker, data), strategy_config(shr_config) {
        // Créer l'instance de la stratégie
        strategy = std::make_unique<SellHeikinRed>(base_config, shr_config);
        strategy->set_log_level(LogLevel::DEBUG);

        auto log_callback = [](const std::string& message, int level) {
            LogLevel logLevel = static_cast<LogLevel>(level);
            
            // Obtenir le timestamp actuel avec précision milliseconde
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            // Formater le timestamp
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
            ss << "," << std::setw(3) << std::setfill('0') << ms.count();
            
            // Afficher le log avec le timestamp
            std::cout << ss.str() << " [" << logLevel << "]: " << message << std::endl;
        };

        g_py_log_callback = log_callback;
    }
    
    /**
     * @brief Initialisation de la stratégie
     * 
     * Cette méthode est appelée une fois au début du backtest pour permettre
     * à la stratégie de s'initialiser avec les données historiques.
     */
    void init() override {
        // Initialiser la stratégie

        
    }
    
    /**
     * @brief Méthode principale appelée à chaque nouvelle bougie
     * 
     * Cette méthode convertit les données de marché en format attendu par la stratégie,
     * met à jour la stratégie et traite les signaux générés.
     */
    void next() override {
        // TODO: C'est une cata on fait plein de getData() qui return l'ensemble des données du backtest c'esttres lent
        // surtout que on a besoin seulement de la derniere candle
        // Vérifier si une position a été fermée depuis la dernière bougie
        // const std::vector<be::Trade> closedTrades = getClosedTrades();
        // if (closedTrades.size() > last_closed_trade_count) {
        //     be::Trade last_trade = closedTrades.back();
            
        //     // Vérifier si le trade a été fermé à la dernière bougie
        //     if (last_trade.exitDate() == getData()->getDate(-1)) {
        //         last_trade_closed = true;
        //         last_trade_pnl = last_trade.pl(); 
        //     }
            
        //     // Mettre à jour le compteur
        //     last_closed_trade_count = closedTrades.size();
        // }

        // Créer un objet Candle à partir des données actuelles
        Candle candle;
        
        // Remplir la structure DateTime à partir de la bougie courante
        // Utilisation de la nouvelle interface
        const be::Candle& currentCandle = getData()->current();
        be::Date current_date = currentCandle.date;
        
        candle.date.year = current_date.getYear();
        candle.date.month = current_date.getMonth();
        candle.date.day = current_date.getDay();
        candle.date.time.hour = current_date.getHour();
        candle.date.time.minute = current_date.getMinute();
        candle.date.time.second = current_date.getSecond();
        
        // Remplir les valeurs OHLC avec la nouvelle interface
        candle.open = currentCandle.open;
        candle.high = currentCandle.high;
        candle.low = currentCandle.low;
        candle.close = currentCandle.close;
        
        // Remplir les informations de position
        be::Position position = getPosition();
        candle.in_position = position ? true : false;
        candle.position_pl_pct = position ? position.plPercent() : 0.0;
        // candle.entry_price = position ? position.entryPrice() : 0.0;
        candle.position_size = position ? position.size() : 0.0;
        
        // Ajouter le P&L du dernier trade fermé s'il y en a un
        candle.closed_trade_pnl = 0.0;
        if (last_trade_closed) {
            candle.closed_trade_pnl = last_trade_pnl;
            last_trade_closed = false;
            last_trade_pnl = 0.0;
        }
        
        // Mettre à jour la stratégie et obtenir le signal
        Signal* signal = strategy->update_candle(candle);


        if (!signal) {
            return; // Pas de signal à traiter
        }
        
        // Traiter le signal s'il y en a un
        if (signal->action == "LIQUIDATE") {
            if (position) {
                position.close();
                std::cout << "Closing position due to LIQUIDATE signal" << std::endl;
            }
        }
        else if (signal->action == "MOVE_SL") {
            // Déplacer le stop loss
            if (position) {
                // position.updateSl(signal->new_sl);
                std::cout << "Moving stop loss to " << signal->new_sl << std::endl;
            }
        }
        else if (!position && signal->action == "BUY") {
            // Exécuter un signal d'achat
            buy(
                signal->quantity,
                0,
                0,
                0,
                0,
                signal->stop_loss, 
                signal->take_profit
                // signal->tag
            );

            std::cout << "Opening BUY position: Price=" << signal->price 
                     << ", Size=" << signal->quantity
                     << ", SL=" << signal->stop_loss
                     << ", TP=" << signal->take_profit << std::endl;
        }
        else if (!position && signal->action == "SELL") {
            // Exécuter un signal de vente
            sell(
                signal->quantity,
                0,
                0,
                0,
                0,
                signal->stop_loss, 
                signal->take_profit
                // signal->tag
            );

            std::cout << "Opening SELL position: Price=" << signal->price 
                     << ", Size=" << signal->quantity
                     << ", SL=" << signal->stop_loss
                     << ", TP=" << signal->take_profit << std::endl;
        }
    }
};