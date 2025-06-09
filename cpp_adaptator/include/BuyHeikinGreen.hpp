#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
// #include "Strategies/buy_heikin_green.hpp"
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
    size_t last_closed_trade_count = 0;
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
        strategy->set_log_level(LogLevel::WARNING);

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
        // Au lieu de récupérer tous les trades à chaque fois
        // auto allTrades = getClosedTrades(); // NE PAS FAIRE CECI
    
        // Compter les trades fermés pour détecter les nouveaux
        //Récupérer une référence à closedTrades au lieu d'une copie
        const auto& closedTrades = _broker->closedTrades();
        size_t currentTradeCount = closedTrades.size();
        
        // Seulement si de nouveaux trades ont été fermés
        if (currentTradeCount > last_closed_trade_count) {
            // Traiter uniquement les nouveaux trades si nécessaire

            last_closed_trade_count = currentTradeCount;
        }
        
        // Créer un objet Candle à partir des données actuelles
        Candle candle;
        
        // Remplir la structure DateTime à partir de la bougie courante
        // Utilisation de la nouvelle interface
        const be::Candle& currentCandle = getData()->current();
        be::Date current_date = currentCandle.date;

        candle.ohlc.date.year = current_date.getYear();
        candle.ohlc.date.month = current_date.getMonth();
        candle.ohlc.date.day = current_date.getDay();
        candle.ohlc.date.time.hour = current_date.getHour();
        candle.ohlc.date.time.minute = current_date.getMinute();
        candle.ohlc.date.time.second = current_date.getSecond();

        // Remplir les valeurs OHLC avec la nouvelle interface
        candle.ohlc.open = currentCandle.open;
        candle.ohlc.high = currentCandle.high;
        candle.ohlc.low = currentCandle.low;
        candle.ohlc.close = currentCandle.close;

        // Remplir les informations de position
        const std::vector<std::shared_ptr<be::Trade>>& trades = _broker->trades();
        std::shared_ptr<be::Trade> last_trade = trades.empty() ? nullptr : trades.back();

        candle.position.in_position = last_trade ? true : false;
        if (candle.position.in_position) {
            // candle.position.position_pl_pct = last_trade->plPercent();
            candle.position.entry_price = last_trade->entryPrice();
            candle.position.take_profit_price = last_trade->tp();
        }
        
        // Ajouter le P&L du dernier trade fermé s'il y en a un
        candle.position.closed_trade_pnl = 0.0;
        if (last_trade_closed) {
            candle.position.closed_trade_pnl = last_trade_pnl;
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
            for (const auto& trade : trades) {
                trade->close();
            }
        }
        else if (signal->action == "MOVE_SL") {
            // Déplacer le stop loss
            last_trade->sl(signal->new_sl);
        }
        else if (trades.empty() && signal->action == "BUY") {
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
        }
        else if (trades.empty() && signal->action == "SELL") {
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
        }
    }
};