#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

/**
 * @brief Classe utilitaire pour le calcul d'indicateurs techniques
 * 
 * Cette classe fournit des méthodes statiques pour calculer divers indicateurs
 * techniques utilisés en analyse financière.
 */
class TechnicalIndicators {
public:
    /**
     * @brief Calcule l'indicateur RSI (Relative Strength Index)
     * 
     * @param closeData Les prix de clôture
     * @param period La période pour le calcul du RSI (généralement 14)
     * @param rsiValues Vecteur de sortie qui contiendra les valeurs RSI calculées
     */
    static void calculateRSI(const std::vector<double>& closeData, int period, std::vector<double>& rsiValues);

    /**
     * @brief Calcule les bougies Heikin-Ashi
     * 
     * @param open Prix d'ouverture
     * @param high Plus hauts
     * @param low Plus bas
     * @param close Prix de clôture
     * @param ha_open Prix d'ouverture Heikin-Ashi (sortie)
     * @param ha_high Plus hauts Heikin-Ashi (sortie)
     * @param ha_low Plus bas Heikin-Ashi (sortie)
     * @param ha_close Prix de clôture Heikin-Ashi (sortie)
     */
    static void calculateHeikinAshi(
        const std::vector<double>& open,
        const std::vector<double>& high,
        const std::vector<double>& low,
        const std::vector<double>& close,
        std::vector<double>& ha_open,
        std::vector<double>& ha_high,
        std::vector<double>& ha_low,
        std::vector<double>& ha_close
    );

    /**
     * @brief Calcule la moyenne mobile exponentielle (EMA)
     * 
     * @param closeData Les prix de clôture
     * @param period La période pour le calcul de l'EMA
     * @param emaValues Vecteur de sortie qui contiendra les valeurs EMA calculées
     */
    static void calculateEMA(const std::vector<double>& closeData, int period, std::vector<double>& emaValues);
    
    // D'autres méthodes pour d'autres indicateurs peuvent être ajoutées...
};