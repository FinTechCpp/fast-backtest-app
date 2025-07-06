#include "components/technicalIndicators.h"

void TechnicalIndicators::calculateRSI(const std::vector<double>& closeData, int period, std::vector<double>& rsiValues)
{
    size_t dataSize = closeData.size();
    rsiValues.resize(dataSize);
    
    if (dataSize <= period) {
        std::fill(rsiValues.begin(), rsiValues.end(), 50.0);  // Valeur neutre par défaut
        return;
    }

    // Calculer les variations de prix (delta)
    std::vector<double> deltas(dataSize - 1);
    for (size_t i = 1; i < dataSize; ++i) {
        deltas[i - 1] = closeData[i] - closeData[i - 1];
    }

    // Séparer les variations positives et négatives
    std::vector<double> gains(dataSize - 1);
    std::vector<double> losses(dataSize - 1);
    for (size_t i = 0; i < deltas.size(); ++i) {
        gains[i] = (deltas[i] > 0) ? deltas[i] : 0;
        losses[i] = (deltas[i] < 0) ? -deltas[i] : 0;
    }

    // Valeurs par défaut pour les premières périodes où le RSI n'est pas défini
    for (int i = 0; i < period; ++i) {
        rsiValues[i] = 50.0;  // Valeur neutre
    }

    // Calculer la première moyenne
    double avgGain = 0;
    double avgLoss = 0;
    for (int i = 0; i < period; ++i) {
        avgGain += gains[i];
        avgLoss += losses[i];
    }
    avgGain /= period;
    avgLoss /= period;

    // Calculer le premier RSI
    double rs = (avgLoss > 0) ? (avgGain / avgLoss) : 100.0;
    rsiValues[period] = 100.0 - (100.0 / (1.0 + rs));

    // Calculer le RSI pour les points restants (méthode Wilder)
    for (size_t i = period + 1; i < dataSize; ++i) {
        // Calculer les moyennes lissées
        avgGain = ((period - 1) * avgGain + gains[i - 1]) / period;
        avgLoss = ((period - 1) * avgLoss + losses[i - 1]) / period;
        
        // Éviter division par zéro
        if (avgLoss > 0) {
            rs = avgGain / avgLoss;
            rsiValues[i] = 100.0 - (100.0 / (1.0 + rs));
        } else {
            rsiValues[i] = 100.0;
        }
    }
}

void TechnicalIndicators::calculateHeikinAshi(
    const std::vector<double>& open,
    const std::vector<double>& high,
    const std::vector<double>& low,
    const std::vector<double>& close,
    std::vector<double>& ha_open,
    std::vector<double>& ha_high,
    std::vector<double>& ha_low,
    std::vector<double>& ha_close)
{
    size_t size = open.size();
    if (size == 0) return;
    
    ha_open.resize(size);
    ha_high.resize(size);
    ha_low.resize(size);
    ha_close.resize(size);
    
    // Première bougie
    ha_open[0] = open[0];
    ha_close[0] = (open[0] + high[0] + low[0] + close[0]) / 4.0;
    ha_high[0] = high[0];
    ha_low[0] = low[0];
    
    // Calcul des autres bougies
    for (size_t i = 1; i < size; ++i) {
        ha_close[i] = (open[i] + high[i] + low[i] + close[i]) / 4.0;
        ha_open[i] = (ha_open[i-1] + ha_close[i-1]) / 2.0;
        ha_high[i] = std::max(std::max(high[i], ha_open[i]), ha_close[i]);
        ha_low[i] = std::min(std::min(low[i], ha_open[i]), ha_close[i]);
    }
}

void TechnicalIndicators::calculateEMA(const std::vector<double>& closeData, int period, std::vector<double>& emaValues)
{
    size_t dataSize = closeData.size();
    emaValues.resize(dataSize);
    
    if (dataSize <= period) {
        std::copy(closeData.begin(), closeData.end(), emaValues.begin());
        return;
    }
    
    // Calcul du facteur de lissage
    double multiplier = 2.0 / (period + 1.0);
    
    // Première valeur EMA = moyenne simple des 'period' premiers points
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += closeData[i];
        emaValues[i] = closeData[i];  // On utilise le prix lui-même pour les premiers points
    }
    emaValues[period-1] = sum / period;
    
    // Calcul de l'EMA pour les points restants
    for (size_t i = period; i < dataSize; ++i) {
        emaValues[i] = (closeData[i] - emaValues[i-1]) * multiplier + emaValues[i-1];
    }
}

void TechnicalIndicators::calculateSupertrend(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    double multiplier,
    std::vector<double>& supertrendValues,
    std::vector<int>& trendDirections)
{
    size_t dataSize = closeData.size();
    supertrendValues.resize(dataSize);
    trendDirections.resize(dataSize, 0);
    
    if (dataSize <= period) {
        std::fill(supertrendValues.begin(), supertrendValues.end(), 0.0);
        return;
    }
    
    // Calculer l'ATR
    std::vector<double> atrValues;
    calculateATR(highData, lowData, closeData, period, atrValues, false);
    
    // Calculer les bandes de base (HL2 +/- multiplier * ATR)
    std::vector<double> basicUpperBand(dataSize);
    std::vector<double> basicLowerBand(dataSize);
    std::vector<double> finalUpperBand(dataSize);
    std::vector<double> finalLowerBand(dataSize);
    
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < period) {
            basicUpperBand[i] = 0.0;
            basicLowerBand[i] = 0.0;
            finalUpperBand[i] = 0.0;
            finalLowerBand[i] = 0.0;
            supertrendValues[i] = 0.0;
            trendDirections[i] = 0;
            continue;
        }
        
        double hl2 = (highData[i] + lowData[i]) / 2.0;
        double atr = atrValues[i];
        
        // Calculer les bandes de base
        basicUpperBand[i] = hl2 + (multiplier * atr);
        basicLowerBand[i] = hl2 - (multiplier * atr);
        
        // Calculer les bandes finales (avec logique de maintien)
        if (i == period) {
            // Première valeur
            finalUpperBand[i] = basicUpperBand[i];
            finalLowerBand[i] = basicLowerBand[i];
        } else {
            // Bande supérieure finale : ne descend que si le prix de clôture précédent était au-dessus
            finalUpperBand[i] = (basicUpperBand[i] < finalUpperBand[i-1] || closeData[i-1] > finalUpperBand[i-1]) 
                               ? basicUpperBand[i] 
                               : finalUpperBand[i-1];
            
            // Bande inférieure finale : ne monte que si le prix de clôture précédent était en-dessous
            finalLowerBand[i] = (basicLowerBand[i] > finalLowerBand[i-1] || closeData[i-1] < finalLowerBand[i-1]) 
                               ? basicLowerBand[i] 
                               : finalLowerBand[i-1];
        }
    }
    
    // Calculer le Supertrend final et la direction
    for (size_t i = period; i < dataSize; ++i) {
        if (i == period) {
            // Première valeur - déterminer la tendance initiale
            if (closeData[i] <= finalUpperBand[i]) {
                supertrendValues[i] = finalUpperBand[i];
                trendDirections[i] = -1; // Tendance baissière
            } else {
                supertrendValues[i] = finalLowerBand[i];
                trendDirections[i] = 1;  // Tendance haussière
            }
        } else {
            // Logique de changement de tendance
            int prevTrend = trendDirections[i-1];
            double prevSupertrend = supertrendValues[i-1];
            
            if (prevTrend == 1) { // Tendance haussière précédente
                if (closeData[i] < finalLowerBand[i]) {
                    // Changement vers tendance baissière
                    supertrendValues[i] = finalUpperBand[i];
                    trendDirections[i] = -1;
                } else {
                    // Maintien tendance haussière
                    supertrendValues[i] = finalLowerBand[i];
                    trendDirections[i] = 1;
                }
            } else { // Tendance baissière précédente
                if (closeData[i] > finalUpperBand[i]) {
                    // Changement vers tendance haussière
                    supertrendValues[i] = finalLowerBand[i];
                    trendDirections[i] = 1;
                } else {
                    // Maintien tendance baissière
                    supertrendValues[i] = finalUpperBand[i];
                    trendDirections[i] = -1;
                }
            }
        }
    }
}

void TechnicalIndicators::calculateStochastic(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int fastKPeriod,
    int slowKPeriod,
    int slowDPeriod,
    std::vector<double>& kValues,
    std::vector<double>& dValues)
{
    // Vérification des données d'entrée
    size_t dataSize = closeData.size();
    if (dataSize == 0 || highData.size() != dataSize || lowData.size() != dataSize) {
        kValues.clear();
        dValues.clear();
        return;
    }
    
    // Redimensionner les vecteurs de sortie
    kValues.resize(dataSize);
    dValues.resize(dataSize);

    // Valeurs par défaut (50 est une valeur neutre pour l'oscillateur)
    std::fill(kValues.begin(), kValues.end(), 50.0);
    std::fill(dValues.begin(), dValues.end(), 50.0);
    
    if (dataSize < static_cast<size_t>(fastKPeriod)) {
        return;  // Pas assez de données pour calculer
    }
    
    // Étape 1: Calculer le %K brut (Fast %K) - La formule est:
    // %K = 100 * (C - L14) / (H14 - L14)
    // où C est le prix de clôture actuel, L14 est le plus bas sur 14 périodes
    // et H14 est le plus haut sur 14 périodes
    std::vector<double> rawK(dataSize);
    
    for (size_t i = fastKPeriod - 1; i < dataSize; ++i) {
        // Trouver le plus bas et le plus haut sur la période fastKPeriod
        double lowestLow = std::numeric_limits<double>::max();
        double highestHigh = std::numeric_limits<double>::lowest();
        
        for (size_t j = i - fastKPeriod + 1; j <= i; ++j) {
            lowestLow = std::min(lowestLow, lowData[j]);
            highestHigh = std::max(highestHigh, highData[j]);
        }
        
        // Calculer le %K brut
        double range = highestHigh - lowestLow;
        if (range > 0.0) {
            rawK[i] = ((closeData[i] - lowestLow) / range) * 100.0;
        } else {
            rawK[i] = 50.0; // Valeur neutre si la plage est nulle
        }
    }
    
    // Étape 2: Lisser le %K brut avec une moyenne mobile sur slowKPeriod pour obtenir le %K lent
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < fastKPeriod - 1 + slowKPeriod - 1) {
            kValues[i] = 50.0;  // Pas assez de données, valeur neutre
            continue;
        }
        
        double sum = 0.0;
        for (size_t j = 0; j < slowKPeriod; ++j) {
            sum += rawK[i - j];
        }
        kValues[i] = sum / slowKPeriod;
    }
    
    // Étape 3: Calculer le %D comme une moyenne mobile des valeurs %K sur slowDPeriod
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < fastKPeriod - 1 + slowKPeriod - 1 + slowDPeriod - 1) {
            dValues[i] = 50.0;  // Pas assez de données, valeur neutre
            continue;
        }
        
        double sum = 0.0;
        for (size_t j = 0; j < slowDPeriod; ++j) {
            sum += kValues[i - j];
        }
        dValues[i] = sum / slowDPeriod;
    }
}

void TechnicalIndicators::calculateATR(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    std::vector<double>& atrValues,
    bool useLogScale)
{
    size_t dataSize = closeData.size();
    atrValues.resize(dataSize);
    
    if (dataSize < static_cast<size_t>(period)) {
        std::fill(atrValues.begin(), atrValues.end(), 0.0);
        return;
    }
    
    // Calculer les variations de prix
    std::vector<double> tr(dataSize - 1);
    for (size_t i = 1; i < dataSize; ++i) {
        double highLow = highData[i] - lowData[i];
        double highClose = std::abs(highData[i] - closeData[i - 1]);
        double lowClose = std::abs(lowData[i] - closeData[i - 1]);
        tr[i - 1] = std::max({highLow, highClose, lowClose});
    }
    
    // Calculer la première moyenne
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += tr[i];
        double atrValue = sum / period;
        
        // Appliquer le logarithme immédiatement si nécessaire
        atrValues[i] = useLogScale ? std::log(atrValue + 1) : atrValue;
    }
    
    // Calculer l'ATR pour les points restants (méthode Wilder)
    for (size_t i = period; i < dataSize; ++i) {
        double atrValue = (atrValues[i - 1] * (period - 1) + tr[i - 1]) / period;
        
        // Si on utilise l'échelle logarithmique, on doit d'abord convertir la valeur précédente
        // de log(atr+1) vers atr avant de l'utiliser dans le calcul
        if (useLogScale) {
            double prevATR = std::exp(atrValues[i - 1]) - 1;  // Récupérer la vraie valeur ATR
            atrValue = (prevATR * (period - 1) + tr[i - 1]) / period;
            atrValues[i] = std::log(atrValue + 1);  // Stocker en logarithme
        } else {
            atrValues[i] = atrValue;  // Stocker normalement
        }
    }
}