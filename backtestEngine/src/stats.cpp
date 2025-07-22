#include "stats.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <iostream>
#include <vector>
#include <utility>
#include <sstream>
#include <iomanip>

namespace be {

// Valeur NaN pour initialiser les statistiques non calculables
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

// Structure pour stocker les informations sur les drawdowns
struct DrawdownInfo {
    std::vector<double> durations;
    std::vector<double> peaks;
    std::vector<size_t> zeroIndices;  // Ajout des indices zéro
};

// Fonction auxiliaire pour calculer la durée et les pics de drawdown
DrawdownInfo computeDrawdownDurationPeaks(const std::vector<double>& dd) {
    std::vector<size_t> zero_indices;
    
    // Trouver les indices où dd est égal à 0 (équivalent à dd == 0 en Python)
    for (size_t i = 0; i < dd.size(); ++i) {
        if (std::abs(dd[i]) < 1e-10) {
            zero_indices.push_back(i);
        }
    }
    
    // Ajouter l'indice final
    if (zero_indices.empty() || zero_indices.back() != dd.size() - 1) {
        zero_indices.push_back(dd.size() - 1);
    }
    
    DrawdownInfo info;
    info.zeroIndices = zero_indices;  // Stocker les indices dans la structure
    
    // Calculer la durée et le pic de drawdown pour chaque période
    for (size_t i = 1; i < zero_indices.size(); ++i) {
        size_t start = zero_indices[i-1];
        size_t end = zero_indices[i];
        
        // Ignorer les séquences où start et end sont adjacents
        if (end - start <= 1) continue;
        
        // Calculer la durée
        double duration = static_cast<double>(end - start);
        info.durations.push_back(duration);
        
        // Calculer le pic de drawdown dans cette période
        auto max_elem = std::max_element(dd.begin() + start, dd.begin() + end + 1);
        double peak_dd = *max_elem;
        info.peaks.push_back(peak_dd);
    }
    
    return info;
}

// Calcul de la moyenne géométrique
double geometricMean(const std::vector<double>& returns) {
    std::vector<double> filled_returns;
    
    // Remplacer les NaN par 0 et ajouter 1
    for (double ret : returns) {
        if (std::isnan(ret)) {
            filled_returns.push_back(1.0);
        } else {
            filled_returns.push_back(ret + 1.0);
        }
    }
    
    // Vérifier si des valeurs sont négatives ou nulles
    if (std::any_of(filled_returns.begin(), filled_returns.end(), 
                    [](double x) { return x <= 0; })) {
        return 0.0;
    }
    
    // Calcul de la moyenne géométrique
    double sum_of_logs = 0.0;
    for (double val : filled_returns) {
        sum_of_logs += std::log(val);
    }
    
    // Éviter la division par zéro
    if (filled_returns.empty()) {
        return NaN;
    }
    
    return std::exp(sum_of_logs / filled_returns.size()) - 1.0;
}

// Méthode pour convertir Stats en map (pour compatibilité)
std::map<std::string, double> Stats::toMap() const {
    std::map<std::string, double> map;
    
    // Valeurs temporelles
    // map["Start"] = start.toString();  // Convertir la date en chaîne de caractères
    // map["End"] = end.toString();      // Convertir la date en chaîne de caractères
    map["Duration"] = duration.getTotalDays();  // Durée en jours
    map["Exposure Time [%]"] = exposureTimePct;
    
    // Valeurs d'équité
    map["Equity Final [$]"] = equityFinal;
    map["Equity Peak [$]"] = equityPeak;
    map["Equity Initial [$]"] = equityInitial;

    // Valeurs de rendement
    map["Return [%]"] = returnPct;
    map["Buy & Hold Return [%]"] = buyHoldReturnPct;
    map["Return (Ann.) [%]"] = returnAnnPct;
    map["Volatility (Ann.) [%]"] = volatilityAnnPct;
    map["CAGR [%]"] = cagrPct;
    
    // Ratios de risque
    map["Sharpe Ratio"] = sharpeRatio;
    map["Sortino Ratio"] = sortinoRatio;
    map["Calmar Ratio"] = calmarRatio;
    map["Alpha [%]"] = alphaPct;
    map["Beta"] = beta;
    
    // Valeurs de drawdown
    map["Max. Drawdown [%]"] = maxDrawdownPct;
    map["Avg. Drawdown [%]"] = avgDrawdownPct;
    // map["Max. Drawdown Duration"] = maxDrawdownDuration.toString();
    // map["Avg. Drawdown Duration"] = avgDrawdownDuration.toString();
    
    // Statistiques des trades
    map["# Trades"] = numTrades;
    map["# Take Profit Trades"] = numTPTrades;
    map["Take Profit Trades [%]"] = pctTPTrades;
    map["# Losing Trades"] = numSLTrades;
    map["Losing Trades [%]"] = pctSLTrades;
    map["# Neutral Trades"] = numBETrades;
    map["Neutral Trades [%]"] = pctBETrades;
    map["Best Trade [%]"] = bestTradePct;
    map["Worst Trade [%]"] = worstTradePct;
    map["Avg. Trade [%]"] = avgTradePct;
    // map["Max. Trade Duration"] = maxTradeDuration.toString();
    // map["Avg. Trade Duration"] = avgTradeDuration.toString();
    map["Profit Factor"] = profitFactor;
    map["Expectancy [%]"] = expectancyPct;
    map["SQN"] = sqn;
    map["Kelly Criterion"] = kellyCriterion;
    
    return map;
}

std::ostream& operator<<(std::ostream& os, const Stats& stats) {
    os << "=== Statistiques de Performance ===\n";
    
    os << "\n-- Période --\n";
    os << "  Début: " << stats.start << "\n";
    os << "  Fin: " << stats.end << "\n";
    os << "  Durée: " << stats.duration << "\n";
    os << "  Exposition: " << stats.exposureTimePct << "%\n";
    
    os << "\n-- Résultats --\n";
    os << "  Équité finale: " << stats.equityFinal << "\n";
    os << "  Équité maximale: " << stats.equityPeak << "\n";
    os << "  Équité initiale: " << stats.equityInitial << "\n";
    os << "  Rendement: " << stats.returnPct << "%\n";
    os << "  Buy & Hold: " << stats.buyHoldReturnPct << "%\n";
    os << "  Rendement annualisé: " << stats.returnAnnPct << "%\n";
    os << "  CAGR: " << stats.cagrPct << "%\n";
    
    os << "\n-- Risque --\n";
    os << "  Volatilité annualisée: " << stats.volatilityAnnPct << "%\n";
    os << "  Ratio de Sharpe: " << stats.sharpeRatio << "\n";
    os << "  Ratio de Sortino: " << stats.sortinoRatio << "\n";
    os << "  Ratio de Calmar: " << stats.calmarRatio << "\n";
    os << "  Alpha: " << stats.alphaPct << "%\n";
    os << "  Beta: " << stats.beta << "\n";
    
    os << "\n-- Drawdowns --\n";
    os << "  Maximum: " << stats.maxDrawdownPct << "%\n";
    os << "  Moyen: " << stats.avgDrawdownPct << "%\n";
    os << "  Durée maximale: " << stats.maxDrawdownDuration << "\n";
    os << "  Durée moyenne: " << stats.avgDrawdownDuration << "\n";
    
    os << "\n-- Trades --\n";
    os << "  Nombre total: " << stats.numTrades << "\n";
    os << "  Gagnants: " << stats.numTPTrades << " (" << stats.pctTPTrades << "%)\n";
    os << "  Perdants: " << stats.numSLTrades << " (" << stats.pctSLTrades << "%)\n";
    os << "  Neutres: " << stats.numBETrades << " (" << stats.pctBETrades << "%)\n";
    os << "  Meilleur: " << stats.bestTradePct << "%\n";
    os << "  Pire: " << stats.worstTradePct << "%\n";
    os << "  Moyen: " << stats.avgTradePct << "%\n";
    os << "  Durée maximale: " << stats.maxTradeDuration << "\n";
    os << "  Durée moyenne: " << stats.avgTradeDuration << "\n";
    os << "  Facteur de profit: " << stats.profitFactor << "\n";
    os << "  Espérance: " << stats.expectancyPct << "%\n";
    os << "  SQN: " << stats.sqn << "\n";
    os << "  Critère de Kelly: " << stats.kellyCriterion << "\n";
    
    return os;
}

// Calcul des statistiques de trading avec la nouvelle structure
Stats computeStats(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data) {
    
    Stats stats;
    
    // Validation des entrées
    if (equity.empty() || data.size() == 0) {
        return dummyStats();
    }

    // Stocker les données brutes pour analyses futures
    stats.equityCurve = equity;          
    stats.trades = trades;               
    
    // Dates de début et fin du backtest
    stats.start = data.at(0).date;  // Première date du dataset
    stats.end = data.at(data.size() - 1).date;  // Dernière date du dataset
    stats.duration = stats.end - stats.start;  // Calcul de la durée totale du backtest
    
    // Calcul du drawdown: 1 - equity / max(equity)
    std::vector<double> dd(equity.size());
    std::vector<double> max_equity(equity.size());
    
    max_equity[0] = equity[0];
    for (size_t i = 1; i < equity.size(); ++i) {
        max_equity[i] = std::max(max_equity[i-1], equity[i]);
    }
    
    for (size_t i = 0; i < equity.size(); ++i) {
        dd[i] = 1.0 - equity[i] / max_equity[i];
    }
    
    // Calculer durée et pics de drawdown
    DrawdownInfo dd_info = computeDrawdownDurationPeaks(dd);
    
    // Calculer le temps d'exposition au marché
    std::vector<int> have_position(data.size(), 0);
    for (const auto& trade : trades) {
        size_t entry = trade->entryBar();
        size_t exit = trade->exitBar();
        for (size_t i = entry; i <= exit && i < have_position.size(); ++i) {
            have_position[i] = 1;
        }
    }
    
    if (!have_position.empty()) {
        stats.exposureTimePct = static_cast<double>(std::accumulate(have_position.begin(), have_position.end(), 0)) / have_position.size() * 100;
    }
    
    // Équité
    stats.equityFinal = equity.back();
    stats.equityPeak = *std::max_element(equity.begin(), equity.end());
    stats.equityInitial = equity.front();  // Équité initiale
    
    // Rendement total
    stats.returnPct = equity.size() > 1 && std::abs(equity.front()) > 1e-10 ? 
        (equity.back() - equity.front()) / equity.front() * 100 : NaN;
    
    // Rendement Buy & Hold - Utiliser at() pour accéder aux données
    size_t first_trading_bar = 1;  // Simplifié par rapport à _indicator_warmup_nbars
    double initial_price = data.at(first_trading_bar).close;  // Modifié
    double final_price = data.at(data.size() - 1).close;      // Modifié
    stats.buyHoldReturnPct = (final_price - initial_price) / initial_price * 100;
    
    // Extraire les données des trades
    std::vector<double> pl_values;
    std::vector<double> return_pct_values;
    std::vector<Duration> tradeDurations;
    
    for (const auto& trade : trades) {
        pl_values.push_back(trade->pl());
        return_pct_values.push_back(trade->plPercent());
        
        // Calculer la durée du trade en utilisant les dates réelles
        Date entryDate = data.at(trade->entryBar()).date;  // Modifié
        Date exitDate = data.at(trade->exitBar()).date;    // Modifié
        Duration tradeDuration = exitDate - entryDate;
        tradeDurations.push_back(tradeDuration);
    }
    
    // Nombre de trades
    size_t n_trades = trades.size();
    stats.numTrades = static_cast<unsigned int>(n_trades);
    
    // Statistiques des trades (victoires/défaites)
    std::ptrdiff_t tp_trades = std::count_if(trades.begin(), trades.end(),
        [](const auto& trade) { return trade->closeReason() == be::CloseReason::TakeProfit; });

    std::ptrdiff_t sl_trades = std::count_if(trades.begin(), trades.end(),
        [](const auto& trade) { return trade->closeReason() == be::CloseReason::StopLoss; });

    std::ptrdiff_t be_trades = std::count_if(trades.begin(), trades.end(),
        [](const auto& trade) { return trade->closeReason() == be::CloseReason::BreakEven; });

    std::ptrdiff_t manual_trades = std::count_if(trades.begin(), trades.end(),
        [](const auto& trade) { return trade->closeReason() == be::CloseReason::ManualClose; });

    std::ptrdiff_t unknown_trades = std::count_if(trades.begin(), trades.end(),
        [](const auto& trade) { return trade->closeReason() == be::CloseReason::Unknown; });


    stats.numTPTrades = static_cast<double>(tp_trades);
    stats.pctTPTrades = n_trades ? static_cast<double>(tp_trades) / n_trades * 100 : NaN;
    stats.numSLTrades = static_cast<double>(sl_trades);
    stats.pctSLTrades = n_trades ? static_cast<double>(sl_trades) / n_trades * 100 : NaN;
    stats.numBETrades = static_cast<double>(be_trades);
    stats.pctBETrades = n_trades ? static_cast<double>(be_trades) / n_trades * 100 : NaN;
    stats.numManualTrades = static_cast<double>(manual_trades);
    stats.pctManualTrades = n_trades ? static_cast<double>(manual_trades) / n_trades * 100 : NaN;
    stats.numUnknownTrades = static_cast<double>(unknown_trades);
    stats.pctUnknownTrades = n_trades ? static_cast<double>(unknown_trades) / n_trades * 100 : NaN;
    
    // Meilleurs et pires trades
    if (!return_pct_values.empty()) {
        stats.bestTradePct = *std::max_element(return_pct_values.begin(), return_pct_values.end()) * 100;
        stats.worstTradePct = *std::min_element(return_pct_values.begin(), return_pct_values.end()) * 100;
    } else {
        stats.bestTradePct = NaN;
        stats.worstTradePct = NaN;
    }
    
    // Rendement moyen par trade
    stats.avgTradePct = geometricMean(return_pct_values) * 100;
    
    // Durée des trades
    if (!tradeDurations.empty()) {
        // Trouver la durée maximale
        stats.maxTradeDuration = *std::max_element(tradeDurations.begin(), tradeDurations.end(),
            [](const Duration& a, const Duration& b) { return a.getTotalSeconds() < b.getTotalSeconds(); });
        
        // Calculer la durée moyenne
        double totalSeconds = 0.0;
        for (const auto& dur : tradeDurations) {
            totalSeconds += dur.getTotalSeconds();
        }
        stats.avgTradeDuration = Duration(totalSeconds / tradeDurations.size());
    }
    
    // Profit Factor (somme des gains / somme des pertes en valeur absolue)
    double sum_wins = 0.0;
    double sum_losses = 0.0;
    
    for (double pl : pl_values) {
        if (pl > 0) sum_wins += pl;
        else if (pl < 0) sum_losses += std::abs(pl);
    }
    
    stats.profitFactor = sum_losses == 0 ? NaN : sum_wins / sum_losses;
    
    // Expectancy (espérance de gain)
    if (!return_pct_values.empty()) {
        double sum_returns = std::accumulate(return_pct_values.begin(), return_pct_values.end(), 0.0);
        stats.expectancyPct = (sum_returns / return_pct_values.size()) * 100;
    } else {
        stats.expectancyPct = NaN;
    }
    
    // SQN (System Quality Number)
    if (!pl_values.empty()) {
        double pl_mean = std::accumulate(pl_values.begin(), pl_values.end(), 0.0) / pl_values.size();
        
        // Écart-type des profits/pertes
        double pl_var = 0.0;
        for (double pl : pl_values) {
            pl_var += (pl - pl_mean) * (pl - pl_mean);
        }
        double pl_std = pl_values.size() > 1 ? std::sqrt(pl_var / (pl_values.size() - 1)) : NaN;
        
        stats.sqn = std::sqrt(n_trades) * pl_mean / (pl_std == 0 ? NaN : pl_std);
    } else {
        stats.sqn = NaN;
    }
    
    // Kelly Criterion
    if (stats.pctTPTrades > 0 && !pl_values.empty()) {
        double avg_win = 0.0;
        double avg_loss = 0.0;
        int win_count = 0;
        int loss_count = 0;
        
        for (double pl : pl_values) {
            if (pl > 0) {
                avg_win += pl;
                win_count++;
            } else if (pl < 0) {
                avg_loss += std::abs(pl);
                loss_count++;
            }
        }
        
        avg_win = win_count > 0 ? avg_win / win_count : 0;
        avg_loss = loss_count > 0 ? avg_loss / loss_count : 0;
        
        if (avg_loss > 0) {
            stats.kellyCriterion = stats.pctTPTrades - (1 - stats.pctTPTrades) / (avg_win / avg_loss);
        } else {
            stats.kellyCriterion = NaN;
        }
    } else {
        stats.kellyCriterion = NaN;
    }
    
    // Drawdown maximum
    double max_dd = 0.0;
    for (double d : dd) {
        max_dd = std::max(max_dd, d);
    }
    stats.maxDrawdownPct = -max_dd * 100;  // Négatif par convention
    
    // Drawdown moyen
    if (!dd_info.peaks.empty()) {
        double avg_dd = std::accumulate(dd_info.peaks.begin(), dd_info.peaks.end(), 0.0) / dd_info.peaks.size();
        stats.avgDrawdownPct = -avg_dd * 100;  // Négatif par convention
    } else {
        stats.avgDrawdownPct = NaN;
    }
    
    // Durée des drawdowns
    if (!dd_info.durations.empty()) {
        // Pour chaque période de drawdown, utiliser les dates réelles
        std::vector<Duration> realDurations;
        
        for (size_t i = 1; i < dd_info.zeroIndices.size(); ++i) {
            size_t start = dd_info.zeroIndices[i-1];
            size_t end = dd_info.zeroIndices[i];
            
            if (end - start <= 1) continue;
            
            // Convertir de l'indice de barre aux dates réelles
            Date startDate = data.at(start).date;
            Date endDate = data.at(end).date;
            Duration realDuration = endDate - startDate;
            
            realDurations.push_back(realDuration);
        }
        
        // Trouver la durée maximale
        if (!realDurations.empty()) {
            stats.maxDrawdownDuration = *std::max_element(realDurations.begin(), realDurations.end(),
                [](const Duration& a, const Duration& b) { return a.getTotalSeconds() < b.getTotalSeconds(); });
            
            // Calculer la durée moyenne
            double totalSeconds = 0.0;
            for (const auto& dur : realDurations) {
                totalSeconds += dur.getTotalSeconds();
            }
            stats.avgDrawdownDuration = Duration(totalSeconds / realDurations.size());
        } else {
            stats.maxDrawdownDuration = Duration();
            stats.avgDrawdownDuration = Duration();
        }
    }
    
    // Calcul des rendements quotidiens (simplifié)
    std::vector<double> day_returns;
    for (size_t i = 1; i < equity.size(); ++i) {
        day_returns.push_back(equity[i] / equity[i-1] - 1.0);
    }
    
    // Rendement journalier moyen géométrique
    double gmean_day_return = geometricMean(day_returns);
    
    // Statistiques annualisées
    const double annual_trading_days = 252;  // Jours de trading par an (standard)
    
    // Rendement annualisé
    stats.returnAnnPct = (std::pow(1 + gmean_day_return, annual_trading_days) - 1) * 100;
    
    // Volatilité annualisée
    if (!day_returns.empty()) {
        double variance = 0.0;
        double mean_return = std::accumulate(day_returns.begin(), day_returns.end(), 0.0) / day_returns.size();
        
        for (double ret : day_returns) {
            variance += (ret - mean_return) * (ret - mean_return);
        }
        
        variance = day_returns.size() > 1 ? variance / (day_returns.size() - 1) : 0;
        stats.volatilityAnnPct = std::sqrt(variance * annual_trading_days) * 100;
    } else {
        stats.volatilityAnnPct = NaN;
    }
    
    // CAGR (Taux de croissance annuel composé)
    double years = stats.duration.getTotalYears();
    if (years > 0) {
        stats.cagrPct = (std::pow(equity.back() / equity.front(), 1.0 / years) - 1) * 100;
    } else {
        stats.cagrPct = NaN;
    }
    
    // Ratio de Sharpe
    const double risk_free_rate = 0.0;  // Taux sans risque (paramètre omis)
    double excess_return = stats.returnAnnPct / 100 - risk_free_rate;
    double volatility = stats.volatilityAnnPct / 100;
    
    if (volatility > 1e-6) {
        stats.sharpeRatio = excess_return / volatility;
    } else {
        stats.sharpeRatio = NaN;
    }
    
    // Ratio de Sortino (version simplifiée)
    if (!day_returns.empty()) {
        double sum_squared_downside = 0.0;
        int downside_count = 0;
        
        for (double ret : day_returns) {
            if (ret < 0) {
                sum_squared_downside += ret * ret;
                downside_count++;
            }
        }
        
        double downside_deviation = downside_count > 0 ? 
            std::sqrt(sum_squared_downside / downside_count) * std::sqrt(annual_trading_days) : 0;
            
        if (downside_deviation > 0) {
            stats.sortinoRatio = excess_return / downside_deviation;
        } else {
            stats.sortinoRatio = NaN;
        }
    } else {
        stats.sortinoRatio = NaN;
    }
    
    // Ratio de Calmar
    if (max_dd > 0) {
        stats.calmarRatio = (stats.cagrPct / 100) / max_dd;
    } else {
        stats.calmarRatio = NaN;
    }
    
    // Calculer le beta et l'alpha (modèle CAPM)
    std::vector<double> equity_log_returns;
    std::vector<double> market_log_returns;

    // Calculer les log returns pour l'equity et le marché
    for (size_t i = 1; i < equity.size(); ++i) {
        equity_log_returns.push_back(std::log(equity[i] / equity[i-1]));
    }

    for (size_t i = 1; i < data.size(); ++i) {
        market_log_returns.push_back(std::log(data.at(i).close / data.at(i-1).close));  // Modifié
    }

    // Calculer le beta seulement si nous avons assez de données
    if (equity_log_returns.size() > 1 && market_log_returns.size() > 1) {
        // Calculer les moyennes
        double equity_mean = std::accumulate(equity_log_returns.begin(), equity_log_returns.end(), 0.0) / equity_log_returns.size();
        double market_mean = std::accumulate(market_log_returns.begin(), market_log_returns.end(), 0.0) / market_log_returns.size();
        
        // Calculer les éléments de la matrice de covariance
        double cov_em = 0.0;  // Covariance equity-market
        double var_m = 0.0;   // Variance du marché
        
        size_t n = std::min(equity_log_returns.size(), market_log_returns.size());
        for (size_t i = 0; i < n; ++i) {
            cov_em += (equity_log_returns[i] - equity_mean) * (market_log_returns[i] - market_mean);
            var_m += (market_log_returns[i] - market_mean) * (market_log_returns[i] - market_mean);
        }
        
        // Éviter la division par zéro
        if (n > 1 && var_m > 0) {
            cov_em /= (n - 1);
            var_m /= (n - 1);
            
            // Calculer le beta
            stats.beta = cov_em / var_m;
            
            // Calculer l'alpha (CAPM)
            const double risk_free_rate = 0.0;  // Taux sans risque (paramètre omis)
            stats.alphaPct = stats.returnPct - risk_free_rate * 100 - stats.beta * (stats.buyHoldReturnPct - risk_free_rate * 100);
        } else {
            stats.beta = NaN;
            stats.alphaPct = NaN;
        }
    } else {
        stats.beta = NaN;
        stats.alphaPct = NaN;
    }
    
    return stats;
}

// Version dummyStats qui initialise tous les champs à NaN
Stats dummyStats() {
    Stats stats;
    
    // Initialiser les vecteurs vides
    stats.equityCurve.clear();
    stats.trades.clear();

    // Initialiser tous les champs à NaN
    stats.start = Date();
    stats.end = Date();
    stats.duration = Duration();
    stats.exposureTimePct = NaN;
    stats.equityFinal = NaN;
    stats.equityPeak = NaN;
    stats.equityInitial = NaN;
    stats.returnPct = NaN;
    stats.buyHoldReturnPct = NaN;
    stats.returnAnnPct = NaN;
    stats.volatilityAnnPct = NaN;
    stats.cagrPct = NaN;
    stats.sharpeRatio = NaN;
    stats.sortinoRatio = NaN;
    stats.calmarRatio = NaN;
    stats.alphaPct = NaN;
    stats.beta = NaN;
    stats.maxDrawdownPct = NaN;
    stats.avgDrawdownPct = NaN;
    stats.maxDrawdownDuration = Duration();
    stats.avgDrawdownDuration = Duration();
    stats.numTrades = 0;
    stats.numTPTrades = NaN;
    stats.pctTPTrades = NaN;
    stats.numSLTrades = NaN;
    stats.pctSLTrades = NaN;
    stats.numBETrades = NaN;
    stats.pctBETrades = NaN;
    stats.numManualTrades = NaN;
    stats.pctManualTrades = NaN;
    stats.numUnknownTrades = NaN;
    stats.pctUnknownTrades = NaN;
    stats.bestTradePct = NaN;
    stats.worstTradePct = NaN;
    stats.avgTradePct = NaN;
    stats.maxTradeDuration = Duration();
    stats.avgTradeDuration = Duration();
    stats.profitFactor = NaN;
    stats.expectancyPct = NaN;
    stats.sqn = NaN;
    stats.kellyCriterion = NaN;
    
    return stats;
}

// Pour la compatibilité avec le code existant
std::map<std::string, double> computeStatsMap(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data) {
    
    Stats stats = computeStats(trades, equity, data);
    return stats.toMap();
}

} // namespace be