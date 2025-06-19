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
     * @param period La période pour le calcul du RSI
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

    /**
     * @brief Calcule l'indicateur SuperTrend
     * 
     * @param highData Les prix les plus hauts
     * @param lowData Les prix les plus bas
     * @param closeData Les prix de clôture
     * @param period La période pour le calcul du SuperTrend
     * @param multiplier Le multiplicateur pour le SuperTrend
     * @param supertrendValues Vecteur de sortie qui contiendra les valeurs SuperTrend calculées
     * @param trendDirections Vecteur de sortie qui contiendra les directions de tendance (1 pour tendance haussière, -1 pour tendance baissière)
     */
    static void calculateSupertrend(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        double multiplier,
        std::vector<double>& supertrendValues,
        std::vector<int>& trendDirections
    );

    /**
     * @brief Calcule l'indicateur Stochastique
     * 
     * @param highData Les prix les plus hauts
     * @param lowData Les prix les plus bas
     * @param closeData Les prix de clôture
     * @param fastKPeriod La période pour calculer %K brut
     * @param slowKPeriod La période de lissage pour %K
     * @param slowDPeriod La période pour calculer %D
     * @param kValues Vecteur de sortie qui contiendra les valeurs %K lissées
     * @param dValues Vecteur de sortie qui contiendra les valeurs %D
     */
    static void calculateStochastic(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int fastKPeriod,
        int slowKPeriod,
        int slowDPeriod,
        std::vector<double>& kValues,
        std::vector<double>& dValues);

    /**
     * @brief Calcule l'Average True Range (ATR)
     * 
     * @param highData Les prix les plus hauts
     * @param lowData Les prix les plus bas
     * @param closeData Les prix de clôture
     * @param period La période pour le calcul de l'ATR
     * @param atrValues Vecteur de sortie qui contiendra les valeurs ATR calculées
     * @param useLogScale Indique si l'échelle logarithmique doit être utilisée pour l'ATR
     */

    static void calculateATR(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        std::vector<double>& atrValues,
        bool useLogScale = false
    );


};