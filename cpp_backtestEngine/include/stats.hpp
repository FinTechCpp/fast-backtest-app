#pragma once

#include <vector>
#include <map>
#include <string>
#include "trade.hpp"
#include "data.hpp"

/**
 * @brief Structure contenant toutes les statistiques de performance du backtest
 * 
 * Cette structure remplace l'utilisation de std::map<std::string, double> par
 * un accès typé et direct aux statistiques via des attributs nommés.
 * Elle inclut également les données brutes (courbe d'équité et trades)
 * pour analyses et visualisations supplémentaires.
 */
struct Stats {
    // Données brutes pour analyse et visualisation
    std::vector<double> equityCurve;                ///< Courbe d'équité complète
    std::vector<std::shared_ptr<Trade>> trades;     ///< Liste des trades fermés
    
    // Statistiques temporelles
    // TODO : a changer pour utiliser des dates réelles (il faudra créer une structure Date)
    double start = 0;                    ///< Indice de la première barre
    double end = 0;                      ///< Indice de la dernière barre
    // TODO : a changer pour utiliser une structure Time 
    double duration = 0;                 ///< Durée du backtest en nombre de barres
    double exposureTimePct = 0;          ///< Pourcentage du temps avec positions ouvertes
    
    // Statistiques d'équité
    double equityFinal = 0;              ///< Équité finale en unités monétaires
    double equityPeak = 0;               ///< Équité maximale atteinte
    
    // Statistiques de rendement
    double returnPct = 0;                ///< Rendement total en pourcentage
    double buyHoldReturnPct = 0;         ///< Rendement d'une stratégie buy & hold
    double returnAnnPct = 0;             ///< Rendement annualisé en pourcentage
    double volatilityAnnPct = 0;         ///< Volatilité annualisée en pourcentage
    double cagrPct = 0;                  ///< Taux de croissance annuel composé
    
    // Ratios de risque
    double sharpeRatio = 0;              ///< Ratio de Sharpe (rendement ajusté au risque)
    double sortinoRatio = 0;             ///< Ratio de Sortino (risque négatif uniquement)
    double calmarRatio = 0;              ///< Ratio de Calmar (rendement / drawdown max)
    double alphaPct = 0;                 ///< Alpha en pourcentage (surperformance)
    double beta = 0;                     ///< Beta (corrélation avec le marché)
    
    // Statistiques de drawdown
    double maxDrawdownPct = 0;           ///< Drawdown maximal en pourcentage
    double avgDrawdownPct = 0;           ///< Drawdown moyen en pourcentage
    // TODO : a changer pour utiliser une structure Time
    double maxDrawdownDuration = 0;      ///< Durée maximale d'un drawdown en barres
    double avgDrawdownDuration = 0;      ///< Durée moyenne des drawdowns
    
    // Statistiques des trades
    double numTrades = 0;                ///< Nombre total de trades
    double winRatePct = 0;               ///< Pourcentage de trades gagnants
    double numWinningTrades = 0;         ///< Nombre de trades gagnants
    double numLosingTrades = 0;          ///< Nombre de trades perdants
    double numNeutralTrades = 0;         ///< Nombre de trades neutres
    double bestTradePct = 0;             ///< Meilleur trade en pourcentage
    double worstTradePct = 0;            ///< Pire trade en pourcentage
    double avgTradePct = 0;              ///< Trade moyen en pourcentage
    double maxTradeDuration = 0;         ///< Durée maximale d'un trade en barres
    double avgTradeDuration = 0;         ///< Durée moyenne des trades
    double profitFactor = 0;             ///< Facteur de profit (gains/pertes)
    double expectancyPct = 0;            ///< Espérance mathématique par trade
    double sqn = 0;                      ///< System Quality Number
    double kellyCriterion = 0;           ///< Critère de Kelly

    /**
     * @brief Convertit la structure en map pour compatibilité
     * @return Map associant les noms des statistiques à leurs valeurs
     */
    std::map<std::string, double> toMap() const;
};

/**
 * @brief Calcule les statistiques de performance
 * 
 * @param trades Liste des trades fermés
 * @param equity Courbe d'équité
 * @param data Données de marché
 * @return Structure contenant toutes les statistiques calculées
 */
Stats computeStats(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data);

/**
 * @brief Crée une structure de statistiques avec des valeurs NaN
 * @return Structure initialisée avec des valeurs par défaut (NaN)
 */
Stats dummyStats();

/**
 * @brief Version compatible de computeStats retournant une map
 * @deprecated Utilisez directement computeStats retournant une structure Stats
 */
std::map<std::string, double> computeStatsMap(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data);