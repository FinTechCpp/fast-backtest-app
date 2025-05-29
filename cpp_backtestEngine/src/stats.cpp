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

// Calcul des statistiques de trading
std::map<std::string, double> computeStats(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data) {
    
    std::map<std::string, double> stats;
    
    // Validation des entrées
    if (equity.empty() || data.size() == 0) {
        return dummyStats();
    }
    
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
    
    // Statistiques de base sur les périodes
    stats["Start"] = 0;  // Indice de début
    stats["End"] = data.size() - 1;  // Indice de fin
    stats["Duration"] = data.size() - 1;  // Durée en nombre de barres
    
    // Calculer le temps d'exposition au marché
    std::vector<int> have_position(data.size(), 0);
    for (const auto& trade : trades) {
        size_t entry = trade->entryBar();
        size_t exit = trade->exitBar();
        for (size_t i = entry; i <= exit && i < have_position.size(); ++i) {
            have_position[i] = 1;
        }
    }
    
    double exposure_time = 0.0;
    if (!have_position.empty()) {
        exposure_time = static_cast<double>(std::accumulate(have_position.begin(), have_position.end(), 0)) / have_position.size() * 100;
    }
    stats["Exposure Time [%]"] = exposure_time;
    
    // Équité
    stats["Equity Final [$]"] = equity.back();
    stats["Equity Peak [$]"] = *std::max_element(equity.begin(), equity.end());
    
    // Rendement total
    stats["Return [%]"] = equity.size() > 1 && std::abs(equity.front()) > 1e-10 ? 
        (equity.back() - equity.front()) / equity.front() * 100 : NaN;
    
    // Rendement Buy & Hold
    size_t first_trading_bar = 1;  // Simplifié par rapport à _indicator_warmup_nbars
    double initial_price = data.Close(first_trading_bar);
    double final_price = data.Close(-1);  // Dernier prix
    stats["Buy & Hold Return [%]"] = (final_price - initial_price) / initial_price * 100;
    
    // Extraire les données des trades
    std::vector<double> pl_values;
    std::vector<double> return_pct_values;
    std::vector<double> durations;
    
    for (const auto& trade : trades) {
        pl_values.push_back(trade->pl());
        return_pct_values.push_back(trade->plPercent());
        durations.push_back(static_cast<double>(trade->exitBar() - trade->entryBar()));
    }
    
    // Nombre de trades
    size_t n_trades = trades.size();
    stats["# Trades"] = n_trades;
    
    // Statistiques des trades (victoires/défaites)
    int winning_trades = std::count_if(pl_values.begin(), pl_values.end(), [](double pl) { return pl > 0; });
    int losing_trades = std::count_if(pl_values.begin(), pl_values.end(), [](double pl) { return pl < 0; });
    int neutral_trades = std::count_if(pl_values.begin(), pl_values.end(), [](double pl) { return pl == 0; });
    
    double win_rate = n_trades ? static_cast<double>(winning_trades) / n_trades : NaN;
    
    stats["Win Rate [%]"] = win_rate * 100;
    stats["# Winning Trades"] = winning_trades;
    stats["# Losing Trades"] = losing_trades;
    stats["# Neutral Trades"] = neutral_trades;
    
    // Meilleurs et pires trades
    if (!return_pct_values.empty()) {
        stats["Best Trade [%]"] = *std::max_element(return_pct_values.begin(), return_pct_values.end()) * 100;
        stats["Worst Trade [%]"] = *std::min_element(return_pct_values.begin(), return_pct_values.end()) * 100;
    } else {
        stats["Best Trade [%]"] = NaN;
        stats["Worst Trade [%]"] = NaN;
    }
    
    // Rendement moyen par trade
    stats["Avg. Trade [%]"] = geometricMean(return_pct_values) * 100;
    
    // Durée des trades
    if (!durations.empty()) {
        stats["Max. Trade Duration"] = *std::max_element(durations.begin(), durations.end());
        stats["Avg. Trade Duration"] = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
    } else {
        stats["Max. Trade Duration"] = NaN;
        stats["Avg. Trade Duration"] = NaN;
    }
    
    // Profit Factor (somme des gains / somme des pertes en valeur absolue)
    double sum_wins = 0.0;
    double sum_losses = 0.0;
    
    for (double pl : pl_values) {
        if (pl > 0) sum_wins += pl;
        else if (pl < 0) sum_losses += std::abs(pl);
    }
    
    stats["Profit Factor"] = sum_losses == 0 ? NaN : sum_wins / sum_losses;
    
    // Expectancy (espérance de gain)
    if (!return_pct_values.empty()) {
        double sum_returns = std::accumulate(return_pct_values.begin(), return_pct_values.end(), 0.0);
        stats["Expectancy [%]"] = (sum_returns / return_pct_values.size()) * 100;
    } else {
        stats["Expectancy [%]"] = NaN;
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
        
        stats["SQN"] = std::sqrt(n_trades) * pl_mean / (pl_std == 0 ? NaN : pl_std);
    } else {
        stats["SQN"] = NaN;
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
            stats["Kelly Criterion"] = win_rate - (1 - win_rate) / (avg_win / avg_loss);
        } else {
            stats["Kelly Criterion"] = NaN;
        }
    } else {
        stats["Kelly Criterion"] = NaN;
    }
    
    // Drawdown maximum
    double max_dd = 0.0;
    for (double d : dd) {
        max_dd = std::max(max_dd, d);
    }
    stats["Max. Drawdown [%]"] = -max_dd * 100;  // Négatif par convention
    
    // Drawdown moyen
    if (!dd_info.peaks.empty()) {
        double avg_dd = std::accumulate(dd_info.peaks.begin(), dd_info.peaks.end(), 0.0) / dd_info.peaks.size();
        stats["Avg. Drawdown [%]"] = -avg_dd * 100;  // Négatif par convention
    } else {
        stats["Avg. Drawdown [%]"] = NaN;
    }
    
    // Durée des drawdowns
    if (!dd_info.durations.empty()) {
        stats["Max. Drawdown Duration"] = *std::max_element(dd_info.durations.begin(), dd_info.durations.end());
        stats["Avg. Drawdown Duration"] = std::accumulate(dd_info.durations.begin(), dd_info.durations.end(), 0.0) / dd_info.durations.size();
    } else {
        stats["Max. Drawdown Duration"] = NaN;
        stats["Avg. Drawdown Duration"] = NaN;
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
    stats["Return (Ann.) [%]"] = (std::pow(1 + gmean_day_return, annual_trading_days) - 1) * 100;
    
    // Volatilité annualisée
    if (!day_returns.empty()) {
        double variance = 0.0;
        double mean_return = std::accumulate(day_returns.begin(), day_returns.end(), 0.0) / day_returns.size();
        
        for (double ret : day_returns) {
            variance += (ret - mean_return) * (ret - mean_return);
        }
        
        variance = day_returns.size() > 1 ? variance / (day_returns.size() - 1) : 0;
        stats["Volatility (Ann.) [%]"] = std::sqrt(variance * annual_trading_days) * 100;
    } else {
        stats["Volatility (Ann.) [%]"] = NaN;
    }
    
    // CAGR (Taux de croissance annuel composé)
    double years = static_cast<double>(data.size()) / annual_trading_days;
    if (years > 0) {
        stats["CAGR [%]"] = (std::pow(equity.back() / equity.front(), 1.0 / years) - 1) * 100;
    } else {
        stats["CAGR [%]"] = NaN;
    }
    
    // Ratio de Sharpe
    const double risk_free_rate = 0.0;  // Taux sans risque (paramètre omis)
    double excess_return = stats["Return (Ann.) [%]"] / 100 - risk_free_rate;
    double volatility = stats["Volatility (Ann.) [%]"] / 100;
    
    if (volatility > 0.001) {
        stats["Sharpe Ratio"] = excess_return / volatility;
    } else {
        stats["Sharpe Ratio"] = NaN;
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
            stats["Sortino Ratio"] = excess_return / downside_deviation;
        } else {
            stats["Sortino Ratio"] = NaN;
        }
    } else {
        stats["Sortino Ratio"] = NaN;
    }
    
    // Ratio de Calmar
    if (max_dd > 0) {
        stats["Calmar Ratio"] = (stats["CAGR [%]"] / 100) / max_dd;
    } else {
        stats["Calmar Ratio"] = NaN;
    }
    
    // Alpha et Beta (version simplifiée)
    // Pour un calcul précis, il faudrait une série temporelle des rendements du marché
    stats["Alpha [%]"] = NaN;
    stats["Beta"] = NaN;
    
    return stats;
}

std::map<std::string, double> dummyStats() {
    std::map<std::string, double> stats;
    
    // Initialiser toutes les statistiques avec NaN
    stats["Start"] = NaN;
    stats["End"] = NaN;
    stats["Duration"] = NaN;
    stats["Exposure Time [%]"] = NaN;
    stats["Equity Final [$]"] = NaN;
    stats["Equity Peak [$]"] = NaN;
    stats["Return [%]"] = NaN;
    stats["Buy & Hold Return [%]"] = NaN;
    stats["Return (Ann.) [%]"] = NaN;
    stats["Volatility (Ann.) [%]"] = NaN;
    stats["CAGR [%]"] = NaN;
    stats["Sharpe Ratio"] = NaN;
    stats["Sortino Ratio"] = NaN;
    stats["Calmar Ratio"] = NaN;
    stats["Alpha [%]"] = NaN;
    stats["Beta"] = NaN;
    stats["Max. Drawdown [%]"] = NaN;
    stats["Avg. Drawdown [%]"] = NaN;
    stats["Max. Drawdown Duration"] = NaN;
    stats["Avg. Drawdown Duration"] = NaN;
    stats["# Trades"] = NaN;
    stats["Win Rate [%]"] = NaN;
    stats["# Winning Trades"] = NaN;
    stats["# Losing Trades"] = NaN;
    stats["# Neutral Trades"] = NaN;
    stats["Best Trade [%]"] = NaN;
    stats["Worst Trade [%]"] = NaN;
    stats["Avg. Trade [%]"] = NaN;
    stats["Max. Trade Duration"] = NaN;
    stats["Avg. Trade Duration"] = NaN;
    stats["Profit Factor"] = NaN;
    stats["Expectancy [%]"] = NaN;
    stats["SQN"] = NaN;
    stats["Kelly Criterion"] = NaN;
    
    return stats;
}