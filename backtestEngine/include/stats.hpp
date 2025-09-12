#pragma once

#include <vector>
#include <map>
#include <string>
#include <ostream>
#include "trade.hpp"
#include "data.hpp"

#include "beTypes.h"

namespace be {
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
// std::map<std::string, double> computeStatsMap(
//     const std::vector<std::shared_ptr<Trade>>& trades,
//     const std::vector<double>& equity,
//     const Data& data);

} // namespace be