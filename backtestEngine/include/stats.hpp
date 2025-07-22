#pragma once

#include <vector>
#include <map>
#include <string>
#include <ostream>
#include "trade.hpp"
#include "data.hpp"
#include "date.hpp"

namespace be {

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
    Date start = Date();                 ///< Indice de la première barre
    Date end = Date();                   ///< Indice de la dernière barre
    Duration duration = Duration();      ///< Durée du backtest
    double exposureTimePct = 0;          ///< Pourcentage du temps avec positions ouvertes

    // Statistiques d'équité
    double equityFinal = 0;              ///< Équité finale en unités monétaires
    double equityPeak = 0;               ///< Équité maximale atteinte
    double equityInitial = 0;            ///< Équité initiale au début du backtest
    
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
    Duration maxDrawdownDuration = Duration();   ///< Durée maximale d'un drawdown en barres
    Duration avgDrawdownDuration = Duration();   ///< Durée moyenne des drawdowns

    // Statistiques des trades
    unsigned int numTrades = 0;          ///< Nombre total de trades
    // double winRatePct = 0;               ///< Pourcentage de trades gagnants
    double numTPTrades = 0;         ///< Nombre de trades gagnants
    double pctTPTrades = 0;          ///< Pourcentage de trades gagnants
    double numSLTrades = 0;          ///< Nombre de trades perdants
    double pctSLTrades = 0;          ///< Pourcentage de trades perdants
    double numBETrades = 0;         ///< Nombre de trades neutres
    double pctBETrades = 0;          ///< Pourcentage de trades en break-even
    double numManualTrades = 0;          ///< Nombre de trades manuels
    double pctManualTrades = 0;     ///< Pourcentage de trades manuels
    double numUnknownTrades = 0;         ///< Nombre de trades avec raison de fermeture inconnue
    double pctUnknownTrades = 0;    ///< Pourcentage de trades avec raison de fermeture
    double bestTradePct = 0;             ///< Meilleur trade en pourcentage
    double worstTradePct = 0;            ///< Pire trade en pourcentage
    double avgTradePct = 0;              ///< Trade moyen en pourcentage
    Duration maxTradeDuration = Duration();      ///< Durée maximale d'un trade en barres
    Duration avgTradeDuration = Duration();      ///< Durée moyenne des trades
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
 * @brief Opérateur de flux pour afficher les statistiques de performance
 * 
 * Affiche les statistiques de manière structurée et lisible.
 * 
 * @param os Flux de sortie
 * @param stats Statistiques à afficher
 * @return Référence au flux de sortie
 */
std::ostream& operator<<(std::ostream& os, const Stats& stats);

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

} // namespace be