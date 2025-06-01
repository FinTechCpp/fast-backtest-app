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
    
    // Calculer la durée et le pic de drawdown pour chaque période
    for (size_t i = 1; i < zero_indices.size(); ++i) {
        size_t start = zero_indices[i-1];
        size_t end = zero_indices[i];
        
        // Ignorer les séquences où start et end sont adjacents
        if (end - start <= 1) continue;
        
        // Calculer la durée
        double duration = end - start;
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
    map["Max. Drawdown Duration"] = maxDrawdownDuration.getTotalDays();
    map["Avg. Drawdown Duration"] = avgDrawdownDuration.getTotalDays();
    
    // Statistiques des trades
    map["# Trades"] = numTrades;
    map["Win Rate [%]"] = winRatePct;
    map["# Winning Trades"] = numWinningTrades;
    map["# Losing Trades"] = numLosingTrades;
    map["# Neutral Trades"] = numNeutralTrades;
    map["Best Trade [%]"] = bestTradePct;
    map["Worst Trade [%]"] = worstTradePct;
    map["Avg. Trade [%]"] = avgTradePct;
    map["Max. Trade Duration"] = maxTradeDuration.getTotalDays();
    map["Avg. Trade Duration"] = avgTradeDuration.getTotalDays();
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
    os << "  Gagnants: " << stats.numWinningTrades << " (" << stats.winRatePct << "%)\n";
    os << "  Perdants: " << stats.numLosingTrades << "\n";
    os << "  Neutres: " << stats.numNeutralTrades << "\n";
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
    stats.start = data.getDate(0);  // Première date du dataset
    stats.end = data.getDate(data.size() - 1);  // Dernière date du dataset
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
    
    // Rendement total
    stats.returnPct = equity.size() > 1 && std::abs(equity.front()) > 1e-10 ? 
        (equity.back() - equity.front()) / equity.front() * 100 : NaN;
    
    // Rendement Buy & Hold
    size_t first_trading_bar = 1;  // Simplifié par rapport à _indicator_warmup_nbars
    double initial_price = data.Close(first_trading_bar);
    double final_price = data.Close(-1);  // Dernier prix
    stats.buyHoldReturnPct = (final_price - initial_price) / initial_price * 100;
    
    // Extraire les données des trades
    std::vector<double> pl_values;
    std::vector<double> return_pct_values;
    std::vector<Duration> tradeDurations;
    
    for (const auto& trade : trades) {
        pl_values.push_back(trade->pl());
        return_pct_values.push_back(trade->plPercent());
        
        // Calculer la durée du trade en utilisant les dates réelles
        Date entryDate = data.getDate(trade->entryBar());
        Date exitDate = data.getDate(trade->exitBar());
        Duration tradeDuration = exitDate - entryDate;
        tradeDurations.push_back(tradeDuration);
    }
    
    // Nombre de trades
    size_t n_trades = trades.size();
    stats.numTrades = n_trades;
    
    // Statistiques des trades (victoires/défaites)
    int winning_trades = std::count_if(pl_values.begin(), pl_values.end(), [](double pl) { return pl > 0; });
    int losing_trades = std::count_if(pl_values.begin(), pl_values.end(), [](double pl) { return pl < 0; });
    int neutral_trades = std::count_if(pl_values.begin(), pl_values.end(), [](double pl) { return pl == 0; });
    
    double win_rate = n_trades ? static_cast<double>(winning_trades) / n_trades : NaN;
    
    stats.winRatePct = win_rate * 100;
    stats.numWinningTrades = winning_trades;
    stats.numLosingTrades = losing_trades;
    stats.numNeutralTrades = neutral_trades;
    
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
    if (win_rate > 0 && !pl_values.empty()) {
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
            stats.kellyCriterion = win_rate - (1 - win_rate) / (avg_win / avg_loss);
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
        // Convertir les durées de drawdown en objets Duration (en jours)
        double maxDDDurationInBars = *std::max_element(dd_info.durations.begin(), dd_info.durations.end());
        stats.maxDrawdownDuration = Duration::fromDays(maxDDDurationInBars);
        
        double avgDDDurationInBars = std::accumulate(dd_info.durations.begin(), dd_info.durations.end(), 0.0) / dd_info.durations.size();
        stats.avgDrawdownDuration = Duration::fromDays(avgDDDurationInBars);
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
    
    if (volatility > 0.001) {
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
    
    // Alpha et Beta (version simplifiée)
    stats.alphaPct = NaN;
    stats.beta = NaN;
    
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
    stats.winRatePct = NaN;
    stats.numWinningTrades = NaN;
    stats.numLosingTrades = NaN;
    stats.numNeutralTrades = NaN;
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