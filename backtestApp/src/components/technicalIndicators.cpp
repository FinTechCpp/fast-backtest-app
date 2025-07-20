#include "components/technicalIndicators.h"

std::vector<double> TechnicalIndicators::calculateRSI(const std::vector<double>& closeData, int period)
{
    size_t dataSize = closeData.size();
    std::vector<double> rsiValues(dataSize, 0.0);

    if (dataSize <= period) {
        std::fill(rsiValues.begin(), rsiValues.end(), 50.0);  // Valeur neutre par défaut
        return rsiValues;
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

    return rsiValues;
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>> TechnicalIndicators::calculateHeikinAshi(
    const std::vector<double>& open,
    const std::vector<double>& high,
    const std::vector<double>& low,
    const std::vector<double>& close)
{
    size_t size = open.size();
    if (size == 0) return {};

    std::vector<double> ha_open(size);
    std::vector<double> ha_high(size);
    std::vector<double> ha_low(size);
    std::vector<double> ha_close(size);

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

    return {ha_open, ha_high, ha_low, ha_close};
}

std::vector<double> TechnicalIndicators::calculateEMA(const std::vector<double>& closeData, int period)
{
    size_t dataSize = closeData.size();
    std::vector<double> emaValues(dataSize, 0.0);

    if (dataSize <= period) {
        std::copy(closeData.begin(), closeData.end(), emaValues.begin());
        return emaValues;
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
    return emaValues;
}

std::tuple<std::vector<double>, std::vector<int>> TechnicalIndicators::calculateSupertrend(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    double multiplier)
{
    size_t dataSize = closeData.size();
    std::vector<double> supertrendValues(dataSize);
    std::vector<int> trendDirections(dataSize, 0);

    if (dataSize <= period) {
        std::fill(supertrendValues.begin(), supertrendValues.end(), 0.0);
        return {supertrendValues, trendDirections};
    }
    
    // Calculer l'ATR
    std::vector<double> atrValues = calculateATR(highData, lowData, closeData, period, false);
    
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

    return {supertrendValues, trendDirections};
}

std::tuple<std::vector<double>, std::vector<double>> TechnicalIndicators::calculateStochastic(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int fastKPeriod,
    int slowKPeriod,
    int slowDPeriod)
{
    // Vérification des données d'entrée
    size_t dataSize = closeData.size();
    if (dataSize == 0 || highData.size() != dataSize || lowData.size() != dataSize)
        return {std::vector<double>(), std::vector<double>()};
    
    // Redimensionner les vecteurs de sortie
    std::vector<double> kValues(dataSize);
    std::vector<double> dValues(dataSize);

    // Valeurs par défaut (50 est une valeur neutre pour l'oscillateur)
    std::fill(kValues.begin(), kValues.end(), 50.0);
    std::fill(dValues.begin(), dValues.end(), 50.0);
    
    if (dataSize < static_cast<size_t>(fastKPeriod)) {
        return {kValues, dValues};  // Pas assez de données pour calculer le Stochastic
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

    return {kValues, dValues};
}

std::vector<double> TechnicalIndicators::calculateATR(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    bool useLogScale)
{
    size_t dataSize = closeData.size();
    std::vector<double> atrValues(dataSize, 0.0);

    if (dataSize < static_cast<size_t>(period)) {
        std::fill(atrValues.begin(), atrValues.end(), 0.0);
        return atrValues;
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

    return atrValues;
}

std::vector<PivotPeriod> TechnicalIndicators::calculatePivotPoints(
    const std::vector<double>& openData,
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    const std::vector<be::Date>& dates,
    PivotPointsInstance::PeriodType periodType,
    PivotPointsInstance::CalculationMethod calcMethod
) {
    if (openData.empty() || highData.empty() || lowData.empty() || closeData.empty() || dates.empty())
        return {};

    std::vector<PivotPeriod> pivotPeriods;

    // Initialiser tous les vecteurs de niveaux avec des zéros
    size_t dataSize = highData.size();
    std::vector<size_t> periodBoundaries;
    periodBoundaries.push_back(0);
    
    // il faut utiliser les arrayMath pour determiner les indices des nouveau jours, mois etc
    be::Date currentDate = dates[0];
    
    for (size_t i = 1; i < dataSize; ++i) {
        const be::Date& date = dates[i];
        
        bool newPeriod = false;
        switch (periodType) {
            case PivotPointsInstance::PeriodType::FourHour: {
                // On considère une nouvelle période si l'heure courante est dans {13, 17, 21, 1}
                // et différente de la précédente (pour éviter de splitter plusieurs fois sur la même heure)
                int hour = static_cast<int>(date.getHour());
                bool isBoundary = (hour == 13 || hour == 17 || hour == 21 || hour == 1);
                int prevHour = static_cast<int>(currentDate.getHour());
                newPeriod = isBoundary && (hour != prevHour);
                // On force aussi le split si le jour/mois/année change
                newPeriod = newPeriod ||
                            (date.getDay() != currentDate.getDay()) ||
                            (date.getMonth() != currentDate.getMonth()) ||
                            (date.getYear() != currentDate.getYear());
                break;
            }
            case PivotPointsInstance::PeriodType::Daily:
                // Nouvelle journée si le jour a changé
                newPeriod = (date.getDay() != currentDate.getDay() ||
                             date.getMonth() != currentDate.getMonth() ||
                             date.getYear() != currentDate.getYear());
                break;
                
            case PivotPointsInstance::PeriodType::Weekly: {
                // Nouvelle semaine si la différence de jours > 2 (week-end ou jours fériés)
                int dayDiff = static_cast<int>(date.getDay() - dates[i-1].getDay());
                bool isMonday = (dayDiff > 2);
                newPeriod = isMonday;
                break;
            }
                
            case PivotPointsInstance::PeriodType::Monthly:
                // Nouveau mois
                newPeriod = (date.getMonth() != currentDate.getMonth() ||
                             date.getYear() != currentDate.getYear());
                break;
        }
        
        if (newPeriod) {
            periodBoundaries.push_back(i);
            currentDate = date;
        }
    }
    periodBoundaries.push_back(dataSize);  // Ajouter la fin
    
    // Pour chaque période, calculer les niveaux de pivot
    for (size_t i = 0; i < periodBoundaries.size() - 1; ++i) {
        size_t start = periodBoundaries[i];
        size_t end = periodBoundaries[i+1] - 1;
        
        // Si première période incomplète (sauf pour quotidien)
        if (i == 0 && periodType != PivotPointsInstance::PeriodType::Daily) {
            continue;
        }
        
        // Obtenir high, low, close pour la période précédente
        double open = 0.0;
        double high = -std::numeric_limits<double>::max();
        double low = std::numeric_limits<double>::max();
        double close = 0.0;
        
        // Si c'est la première période, on utilise les données actuelles
        size_t calcStart = (i == 0) ? start : periodBoundaries[i-1];
        size_t calcEnd = (i == 0) ? end : start - 1;
        
        for (size_t j = calcStart; j <= calcEnd; ++j) {
            high = std::max(high, highData[j]);
            low = std::min(low, lowData[j]);
        }
        open = openData[calcStart];  // Première valeur de la période
        close = closeData[calcEnd];  // Dernière valeur
        
        // Le reste du calcul des points pivots reste inchangé
        double pivot;
        switch (calcMethod) {
            case PivotPointsInstance::CalculationMethod::OHLC:
                pivot = (high + low + close + open) / 4.0;
                break;
            case PivotPointsInstance::CalculationMethod::HL0:
                pivot = (high + low + open) / 3.0;
                break;
            case PivotPointsInstance::CalculationMethod::HLC:
            default:
                pivot = (high + low + close) / 3.0;
                break;
        }


        double r1 = (2.0 * pivot) - low;
        double s1 = (2.0 * pivot) - high;
        double r2 = pivot + (high - low);
        double s2 = pivot - (high - low);
        double r3 = high + 2.0 * (pivot - low);
        double s3 = low - 2.0 * (high - pivot);
        
        double mpr1 = (pivot + r1) / 2.0;
        double mr1r2 = (r1 + r2) / 2.0;
        double mr2r3 = (r2 + r3) / 2.0;
        double mps1 = (pivot + s1) / 2.0;
        double ms1s2 = (s1 + s2) / 2.0;
        double ms2s3 = (s2 + s3) / 2.0;

        // Créer une nouvelle période de pivot
        PivotPeriod period;
        period.rawStartIndex = start;
        period.rawEndIndex = end;
        
        // Stocker un segment unique pour chaque niveau durant cette période
        using LT = PivotPointsInstance::LevelType;
        period.levelValues[static_cast<int>(LT::Pivot)] = pivot;
        period.levelValues[static_cast<int>(LT::R1)] = r1;
        period.levelValues[static_cast<int>(LT::R2)] = r2;
        period.levelValues[static_cast<int>(LT::R3)] = r3;
        period.levelValues[static_cast<int>(LT::S1)] = s1;
        period.levelValues[static_cast<int>(LT::S2)] = s2;
        period.levelValues[static_cast<int>(LT::S3)] = s3;
        period.levelValues[static_cast<int>(LT::M_PR1)] = mpr1;
        period.levelValues[static_cast<int>(LT::M_R1R2)] = mr1r2;
        period.levelValues[static_cast<int>(LT::M_R2R3)] = mr2r3;
        period.levelValues[static_cast<int>(LT::M_PS1)] = mps1;
        period.levelValues[static_cast<int>(LT::M_S1S2)] = ms1s2;
        period.levelValues[static_cast<int>(LT::M_S2S3)] = ms2s3;
        
        pivotPeriods.push_back(period);
    }

    return pivotPeriods;
}