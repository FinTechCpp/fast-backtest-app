#include <spdlog/spdlog.h>
#include "Strategies/buy_heikin_green.hpp"
#include "Strategies/sell_heikin_red.hpp"
#include "common.h"
#include "strategy.h"
#include "igBroker.h"
#include "trading_ig_config.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

// Contrôle global pour l'arrêt propre du programme
std::atomic<bool> running{true};

// Gestionnaire de signal pour arrêter proprement
void signalHandler(int signal) {
    spdlog::info("Signal reçu ({}), arrêt en cours...", signal);
    running = false;
}

// Mock du broker IG - À implémenter avec les vraies API plus tard
class IGLiveBroker {
private:
    ig::IGService& ig_service;
    std::string symbol;
    // std::vector<std::shared_ptr<be::Trade>> active_trades;
    // std::vector<std::shared_ptr<be::Trade>> closed_trades;

public:
    IGLiveBroker(ig::IGService& service, const std::string& market_symbol) 
        : ig_service(service), symbol(market_symbol) {
    }

    // Récupérer la dernière bougie
    Candle getCurrentCandle() {
        // TODO: Implémentation réelle avec l'API IG
        spdlog::debug("Récupération de la bougie actuelle");
        BasicCandle ohlc;
        PositionInfo position;
        
        return Candle(ohlc, position);
    }
    
    // Méthodes pour exécuter les ordres basés sur les signaux
    bool buy(double quantity, double stop_loss, double take_profit) {
        spdlog::info("🔵 SIGNAL D'ACHAT: Quantité={:.2f}, SL={:.2f}, TP={:.2f}", 
                    quantity, stop_loss, take_profit);
        
        // TODO: Implémentation réelle avec l'API IG
        return true;
    }
    
    bool sell(double quantity, double stop_loss, double take_profit) {
        spdlog::info("🔴 SIGNAL DE VENTE: Quantité={:.2f}, SL={:.2f}, TP={:.2f}", 
                    quantity, stop_loss, take_profit);
        
        // TODO: Implémentation réelle avec l'API IG
        return true;
    }
    
    bool closeAllPositions() {
        spdlog::info("🟡 LIQUIDATION: Fermeture de toutes les positions");
        
        // TODO: Implémentation réelle avec l'API IG
        return true;
    }
    
    bool modifyStopLoss(double new_sl) {
        spdlog::info("🟢 MODIFICATION SL: Nouveau SL={:.2f}", new_sl);
        
        // TODO: Implémentation réelle avec l'API IG
        return true;
    }
    
    // Accesseurs
    // const std::vector<std::shared_ptr<be::Trade>>& getActiveTrades() const {
    //     return active_trades;
    // }
    
    // const std::vector<std::shared_ptr<be::Trade>>& getClosedTrades() const {
    //     return closed_trades;
    // }
};

// Classe pour gérer la boucle principale de trading
class TradingLoop {
private:
    std::unique_ptr<BuyHeikinGreen> strategy;
    IGLiveBroker& broker;
    std::chrono::seconds interval;
    std::thread trading_thread;

public:
    TradingLoop(std::unique_ptr<BuyHeikinGreen> strategy_ptr, IGLiveBroker& broker_ref, 
                int interval_seconds = 20) 
        : strategy(std::move(strategy_ptr)), 
          broker(broker_ref),
          interval(interval_seconds) {
    }
    
    void start() {
        trading_thread = std::thread(&TradingLoop::run, this);
    }
    
    void join() {
        if (trading_thread.joinable()) {
            trading_thread.join();
        }
    }

private:
    void run() {
        spdlog::info("Démarrage de la boucle de trading (intervalle: {} secondes)", interval.count());
        
        while (running) {
            try {
                // 1. Récupérer la bougie actuelle
                Candle current_candle = broker.getCurrentCandle();
                spdlog::debug("Bougie: {}/{}/{}/{} (O/H/L/C)",
                             current_candle.open(), current_candle.high(),
                             current_candle.low(), current_candle.close());
                
                // 2. Mettre à jour la stratégie et récupérer le signal
                Signal* signal = strategy->update_candle(current_candle);
                
                // 3. Traiter le signal s'il existe
                if (signal) {
                    processSignal(*signal);
                }
                
                // Attendre l'intervalle configuré
                std::this_thread::sleep_for(interval);
                
            } catch (const std::exception& e) {
                spdlog::error("Erreur dans la boucle de trading: {}", e.what());
                std::this_thread::sleep_for(std::chrono::seconds(5)); // Pause plus courte en cas d'erreur
            }
        }
        
        spdlog::info("Boucle de trading arrêtée");
    }
    
    void processSignal(const Signal& signal) {
        if (signal.action.empty()) {
            return; // Pas de signal à traiter
        }
        
        if (signal.action == "BUY") {
            broker.buy(signal.quantity, signal.stop_loss, signal.take_profit);
        }
        else if (signal.action == "SELL") {
            broker.sell(signal.quantity, signal.stop_loss, signal.take_profit);
        }
        else if (signal.action == "LIQUIDATE") {
            broker.closeAllPositions();
        }
        else if (signal.action == "MOVE_SL") {
            broker.modifyStopLoss(signal.new_sl);
        }
        else {
            spdlog::warn("Action non reconnue: {}", signal.action);
        }
    }
};

int main() {
    // Configurer le gestionnaire de signal
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Configuration des logs
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::info("Démarrage de l'application de trading IG...");

    try {
        // Chargement de la configuration
        Config config;
        spdlog::info("Configuration chargée pour l'utilisateur {}", 
                    config.username.empty() ? "[NON DÉFINI]" : config.username);

        // Initialisation du service IG
        ig::IGService ig_service(config.username, 
                            config.password, 
                            config.api_key, 
                            config.acc_type, 
                            config.acc_number);

        // Création du broker
        IGLiveBroker broker(ig_service, "IX.D.NASDAQ.IFM.IP");  // Symbole à configurer

        // Configuration de la stratégie
        StrategyBaseConfig base_config;
        base_config.enable_logging = true;
        base_config.logLevel = LogLevel::DEBUG;
        
        // Paramètres temporels
        base_config.trading_from = {7, 0, 0};
        base_config.trading_to = {23, 0, 0};
        base_config.trading_days = {0, 1, 2, 3, 4};
        
        // Paramètres SL/TP
        base_config.stop_loss_distance = 20.0;
        base_config.take_profit_distance = 30.0;
        
        // Autres paramètres
        // [...]
        
        // Configuration spécifique à la stratégie
        BuyHeikinGreenConfig bhg_config;
        bhg_config.ema_short_period = 20;
        bhg_config.ema_long_period = 50;
        bhg_config.stoch_fastk = 10;
        bhg_config.stoch_slowk = 7;
        bhg_config.stoch_slowd = 3;
        bhg_config.stoch_threshold = 20;
        bhg_config.rsi_period = 14;
        bhg_config.rsi_threshold = 30;
        
        bhg_config.use_ema_short_filter = true;
        bhg_config.use_ema_long_filter = true;
        bhg_config.use_stoch_filter = true;
        bhg_config.use_rsi_filter = true;
        bhg_config.use_previous_ha_candle_red_filter = true;

        // Création de la stratégie
        auto strategy = std::make_unique<BuyHeikinGreen>(base_config, bhg_config);
        
        // Configuration du logger pour la stratégie
        auto strategy_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
            "strategy_logger",
            "logs/strategy_log.log",
            50 * 1024 * 1024,
            1
        );
        strategy_logger->set_level(spdlog::level::debug);

        auto log_callback = [strategy_logger](const std::string& message, int level) {
            spdlog::level::level_enum spdlog_level = spdlog::level::info;
            switch (level) {
                case static_cast<int>(LogLevel::DEBUG):   spdlog_level = spdlog::level::debug; break;
                case static_cast<int>(LogLevel::INFO):    spdlog_level = spdlog::level::info; break;
                case static_cast<int>(LogLevel::WARNING): spdlog_level = spdlog::level::warn; break;
                case static_cast<int>(LogLevel::FATAL):   spdlog_level = spdlog::level::err; break;
            }
            strategy_logger->log(spdlog_level, "{}", message);
        };

        strategy->set_log_callback(log_callback);
        
        // Création et démarrage de la boucle de trading
        TradingLoop trading_loop(std::move(strategy), broker, 20);
        trading_loop.start();
        
        spdlog::info("Application de trading démarrée. Appuyez sur Ctrl+C pour quitter.");
        
        // Attendre que la boucle se termine (via signal)
        trading_loop.join();
        
        spdlog::info("Application de trading terminée avec succès.");
        return 0;
        
    } catch (const std::exception& e) {
        spdlog::critical("Erreur fatale: {}", e.what());
        return 1;
    }
}