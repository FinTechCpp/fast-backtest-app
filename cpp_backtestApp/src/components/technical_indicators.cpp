#include "components/technical_indicators.h"

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