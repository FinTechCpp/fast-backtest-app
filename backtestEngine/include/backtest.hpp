#pragma once

#include <memory>
#include <map>
#include <string>
#include <functional>
#include <iostream>
#include "data.hpp"
#include "broker.hpp"
#include "strategy.hpp"
#include "stats.hpp"

namespace be {

/**
 * @brief Moteur principal de backtesting pour simuler et évaluer des stratégies de trading
 * 
 * La classe Backtest est responsable de:
 * - Coordonner l'exécution chronologique de la simulation
 * - Transmettre les ordres de la stratégie vers le broker
 * - Calculer les statistiques de performance
 * - Optimiser les paramètres de stratégie
 * 
 * Elle utilise un modèle d'injection de dépendances pour permettre une flexibilité maximale
 * dans la définition des stratégies et la configuration de l'environnement de simulation.
 */
class Backtest {
public:
    /**
     * @brief Constructeur du moteur de backtesting
     * 
     * @param data Données historiques de marché, incluant OHLCV et éventuellement d'autres séries temporelles
     * @param strategyFactory Fonction factory qui crée une stratégie basée sur un broker et les données
     * @param cash Montant du capital initial disponible pour le trading (valeur par défaut: 10000.0)
     * @param spread Écart entre le prix d'achat et de vente, représente les frais de marché implicites (valeur par défaut: 0.0)
     * @param commission Commission fixe ou proportionnelle par transaction (valeur par défaut: 0.0)
     * @param margin Marge requise, exprimée comme une fraction de la valeur nominale (ex: 0.1 = levier 10:1) (valeur par défaut: 1.0 = pas de levier)
     * @param tradeOnClose Si true, exécute les ordres au prix de clôture de la barre; sinon à l'ouverture suivante (valeur par défaut: false)
     * @param hedging Si true, permet les positions longues et courtes simultanées sur le même instrument (valeur par défaut: false)
     * @param exclusiveOrders Si true, un seul ordre peut être actif à la fois (valeur par défaut: false)
     * @param finalizeTrades Si true, ferme automatiquement tous les trades ouverts à la fin du backtest (valeur par défaut: false)
     * 
     * @throws std::invalid_argument Si les paramètres ne sont pas valides (ex: cash négatif, marge hors limites)
     * @throws std::invalid_argument Si les données contiennent des valeurs NaN
     */
    Backtest(std::shared_ptr<Data> data,
             std::function<std::shared_ptr<Strategy>(std::shared_ptr<Broker>, std::shared_ptr<Data>)> strategyFactory,
             double cash = 10000.0,
             double spread = 0.0,
             double commission = 0.0,
             double margin = 1.0,
             bool tradeOnClose = false,
             bool hedging = false,
             bool exclusiveOrders = false,
             bool finalizeTrades = false);
    
    /**
     * @brief Exécute la simulation complète de backtest avec les paramètres actuels
     * 
     * Cette méthode:
     * - Crée une instance du broker et de la stratégie
     * - Initialise la stratégie (appelant Strategy::init())
     * - Pour chaque barre des données, exécute séquentiellement:
     *   1. Mise à jour du broker (traitement des ordres existants)
     *   2. Exécution de la logique de la stratégie (génération de nouveaux ordres)
     * - Si finalizeTrades est activé, clôture tous les trades ouverts
     * - Calcule les statistiques de performance
     * 
     * @return Structure Stats contenant toutes les métriques de performance (rendement, risque, trading)
     * 
     * @throws std::runtime_error En cas d'erreur pendant l'exécution du backtest
     * @note Les erreurs non critiques (ex: plus d'argent) sont interceptées et le backtest continue avec les résultats partiels
     */
    Stats run();
    
    /**
     * @brief Optimise les paramètres de la stratégie par recherche exhaustive
     * 
     * Explore toutes les combinaisons possibles des paramètres spécifiés et
     * identifie celle qui maximise la métrique choisie.
     * 
     * @param params Map associant les noms des paramètres à leurs valeurs possibles
     * @param maximize Nom de la métrique à maximiser (ex: "SQN", "Return [%]", "Calmar Ratio")
     * 
     * @return Paire contenant {meilleurs paramètres, meilleures statistiques}
     * 
     * @throws std::invalid_argument Si params est vide ou si maximize n'est pas une métrique valide
     * @note Cette fonction peut être très coûteuse en temps d'exécution pour un grand nombre de paramètres
     */
    std::pair<std::map<std::string, double>, Stats> 
    optimize(const std::map<std::string, std::vector<double>>& params,
             const std::string& maximize = "SQN");
    
    /**
     * @brief Retourne la liste des trades fermés du dernier backtest
     * 
     * Permet d'accéder aux détails des transactions après l'exécution d'un backtest
     * pour analyse approfondie ou visualisation.
     * 
     * @return Vecteur de trades fermés partagés
     * @note Retourne une copie sécurisée des trades, pas une référence
     */
    std::vector<std::shared_ptr<Trade>> closedTrades() const {
        return _broker->closedTrades();
    }

    void setProgressCallback(std::function<void(size_t, size_t)> callback) {
        _progressCallback = callback;
    }

private:
    std::shared_ptr<Data> _data;           ///< Données de marché historiques
    std::shared_ptr<Broker> _broker;       ///< Broker qui exécute les ordres
    std::function<std::shared_ptr<Strategy>(std::shared_ptr<Broker>, std::shared_ptr<Data>)> _strategyFactory; ///< Fabrique de stratégies
    double _cash;                          ///< Capital initial
    double _spread;                        ///< Écart entre prix d'achat et de vente
    double _commission;                    ///< Commission par transaction
    double _margin;                        ///< Marge requise (fraction de la valeur nominale)
    bool _tradeOnClose;                    ///< Si vrai, exécute les ordres au prix de clôture
    bool _hedging;                         ///< Si vrai, permet les positions longues et courtes simultanées
    bool _exclusiveOrders;                 ///< Si vrai, un seul ordre actif à la fois
    bool _finalizeTrades;                  ///< Si vrai, ferme tous les trades à la fin
    Stats _lastResults;                    ///< Résultats du dernier backtest
    std::function<void(size_t, size_t)> _progressCallback;
};

} // namespace be