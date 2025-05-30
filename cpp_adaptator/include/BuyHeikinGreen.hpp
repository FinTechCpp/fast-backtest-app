#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "buy_heikin_green.hpp"
#include <memory>
#include <iostream>

/**
 * @brief Adaptateur permettant d'utiliser la stratégie BuyHeikinGreen avec le moteur de backtest C++
 * 
 * Cette classe sert d'interface entre la stratégie BuyHeikinGreen (qui utilise sa propre structure)
 * et le moteur de backtest C++ qui attend une classe dérivée de Strategy.
 */
class BuyHeikinGreenAdapter : public be::Strategy {
private:
    // Configuration spécifique à la stratégie
    BuyHeikinGreenConfig strategy_config;
    
    // Instance de la stratégie BuyHeikinGreen
    std::unique_ptr<BuyHeikinGreen> strategy;
    
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
     * @param bhg_config Configuration spécifique à BuyHeikinGreen
     */
    BuyHeikinGreenAdapter(
        std::shared_ptr<be::Broker> broker, 
        std::shared_ptr<be::Data> data,
        const StrategyBaseConfig& base_config,
        const BuyHeikinGreenConfig& bhg_config
    ) : be::Strategy(broker, data), strategy_config(bhg_config) {
        // Créer l'instance de la stratégie
        strategy = std::make_unique<BuyHeikinGreen>(base_config, bhg_config);
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
        // Vérifier si une position a été fermée depuis la dernière bougie
        // const auto trades = getBroker()->closedTrades();
        // if (trades.size() > last_closed_trade_count) {
        //     auto last_trade = trades.back();
            
        //     // Vérifier si le trade a été fermé à la dernière bougie
        //     if (last_trade->exitBar() == getBroker()->getCurrentBar()) {
        //         last_trade_closed = true;
        //         last_trade_pnl = last_trade->pl(); 
        //     }
            
        //     // Mettre à jour le compteur
        //     last_closed_trade_count = trades.size();
        // }

        // Créer un objet Candle à partir des données actuelles
        Candle candle;
        
        // Remplir la structure DateTime
        be::Date current_date = getData()->getDate(-1);
        candle.date.year = current_date.getYear();
        candle.date.month = current_date.getMonth();
        candle.date.day = current_date.getDay();
        candle.date.time.hour = current_date.getHour();
        candle.date.time.minute = current_date.getMinute();
        candle.date.time.second = current_date.getSecond();
        
        // Remplir les valeurs OHLC
        candle.open = getData()->Open(-1);
        candle.high = getData()->High(-1);
        candle.low = getData()->Low(-1);
        candle.close = getData()->Close(-1);
        
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
            std::cout << "No signal generated for this candle." << std::endl;
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