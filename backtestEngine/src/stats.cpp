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

// NaN value for uninitialized non-computable statistics
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
constexpr unsigned int NaNUInt = std::numeric_limits<unsigned int>::min(); // Used for integer values

// Structure for storing drawdown information
struct DrawdownInfo {
    std::vector<double> durations;
    std::vector<double> peaks;
    std::vector<size_t> zeroIndices;  // Added zero indices
};

// Auxiliary function to compute drawdown duration and peaks
DrawdownInfo computeDrawdownDurationPeaks(const std::vector<double>& dd) {
    std::vector<size_t> zero_indices;

    // Find indices where dd is equal to 0 (equivalent to dd == 0 in Python)
    for (size_t i = 0; i < dd.size(); ++i) {
        if (std::abs(dd[i]) < 1e-10) {
            zero_indices.push_back(i);
        }
    }

    // Add final index
    if (zero_indices.empty() || zero_indices.back() != dd.size() - 1) {
        zero_indices.push_back(dd.size() - 1);
    }
    
    DrawdownInfo info;
    info.zeroIndices = zero_indices;  // Store indices in the structure

    // Compute drawdown duration and peaks for each period
    for (size_t i = 1; i < zero_indices.size(); ++i) {
        size_t start = zero_indices[i-1];
        size_t end = zero_indices[i];

        // Ignore sequences where start and end are adjacent
        if (end - start <= 1) continue;

        // Compute duration
        double duration = static_cast<double>(end - start);
        info.durations.push_back(duration);

        // Compute drawdown peak in this period
        auto max_elem = std::max_element(dd.begin() + start, dd.begin() + end + 1);
        double peak_dd = *max_elem;
        info.peaks.push_back(peak_dd);
    }
    
    return info;
}

double calculateSkewness(const std::vector<double>& returns) {
    if (returns.size() < 3) return NaN;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double sum_cubed_dev = 0.0;
    double sum_squared_dev = 0.0;
    
    for (double r : returns) {
        double deviation = r - mean;
        sum_cubed_dev += deviation * deviation * deviation;
        sum_squared_dev += deviation * deviation;
    }
    
    double variance = sum_squared_dev / returns.size();
    return sum_cubed_dev / (returns.size() * std::pow(variance, 1.5));
}

/**
 * @brief Calculates the Sortino ratio using a minimum acceptable return (MAR)
 * @param returns Vector of returns
 * @param excessReturn Annualized excess return
 * @param mar Minimum acceptable return (Minimum Acceptable Return)
 * @param annualFactor Annualization factor (usually 252 for trading days)
 * @return Sortino ratio
 */
double calculateSortinoRatio(const std::vector<double>& returns, double excessReturn, 
                             double mar = 0.0, double annualFactor = 252.0) {
    if (returns.empty()) return NaN;
    
    double sum_squared_downside = 0.0;
    int total_observations = returns.size();

    // Calculate the sum of squared negative deviations from the MAR
    for (double ret : returns) {
        if (ret < mar) {
            double downside = ret - mar;
            sum_squared_downside += downside * downside;
        }
    }

    // Calculate downside deviation
    // Note: we divide by the total number of observations (not just those below the MAR)
    double downside_deviation = std::sqrt(sum_squared_downside / total_observations);

    // Annualize downside deviation
    double annualized_downside_deviation = downside_deviation * std::sqrt(annualFactor);

    // Calculate Sortino ratio
    if (annualized_downside_deviation > 1e-10) {
        return excessReturn / annualized_downside_deviation;
    } else if (excessReturn > 0) {
        return std::numeric_limits<double>::infinity(); // Positive return with no downside risk
    } else if (excessReturn < 0) {
        return -std::numeric_limits<double>::infinity(); // Negative return with no downside risk
    } else {
        return 0.0; // Zero return with no downside risk
    }
}

/**
 * @brief Calculates the kurtosis of a series of returns
 * @param returns Vector of returns
 * @return Kurtosis of the distribution
 */
double calculateKurtosis(const std::vector<double>& returns) {
    if (returns.size() < 4) return NaN;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double sum_fourth_power = 0.0;
    double sum_squared_dev = 0.0;
    
    for (double r : returns) {
        double deviation = r - mean;
        double squared_dev = deviation * deviation;
        sum_squared_dev += squared_dev;
        sum_fourth_power += squared_dev * squared_dev;
    }
    
    double variance = sum_squared_dev / returns.size();
    if (variance < 1e-10) return NaN;  // Avoid division by zero

    // Kurtosis formula (unadjusted)
    return sum_fourth_power / (returns.size() * variance * variance);
}

/**
 * @brief Finds the worst price encountered during a trade
 * @param trade Shared pointer to the trade to analyze
 * @param data Market data
 * @return Worst price for the trade direction
 */
double findWorstPrice(const std::shared_ptr<be::Trade>& trade, const be::Data& data) {
    if (!trade || trade->entryBar() >= data.size() || trade->exitBar() >= data.size())
        return NaN;
    

    // Determine if it's a long or short trade
    bool isLong = trade->isLong();

    // Initial value of the worst price
    double worstPrice = isLong ? std::numeric_limits<double>::max() : std::numeric_limits<double>::lowest();

    // Loop through all candles during the trade duration
    for (size_t i = trade->entryBar(); i <= trade->exitBar() && i < data.size(); ++i) {
        const auto& candle = data.at(i);

        // For a long trade, the worst price is the lowest
        if (isLong) {
            worstPrice = std::min(worstPrice, candle.low);
        }
        // For a short trade, the worst price is the highest
        else {
            worstPrice = std::max(worstPrice, candle.high);
        }
    }
    
    return worstPrice;
}

// Calculates the geometric mean of a series of returns
double geometricMean(const std::vector<double>& returns) {
    std::vector<double> filled_returns;

    // Replace NaN with 0 and add 1
    for (double ret : returns) {
        if (std::isnan(ret))
            continue;
        
        filled_returns.push_back(ret + 1.0);
    }

    // Check if any values are negative or zero
    if (std::any_of(filled_returns.begin(), filled_returns.end(),
                    [](double x) { return x <= 0; })) {
        return 0.0;
    }

    // Calculate the geometric mean
    double sum_of_logs = 0.0;
    for (double val : filled_returns) {
        sum_of_logs += std::log(val);
    }

    // Avoid division by zero
    if (filled_returns.empty()) {
        return NaN;
    }
    
    return std::exp(sum_of_logs / filled_returns.size()) - 1.0;
}

// Method to convert Stats to map (for compatibility)
std::map<std::string, double> Stats::toMap() const {
    std::map<std::string, double> map;

    // Temporal values
    // map["Start"] = start.toString();  // Convert date to string
    // map["End"] = end.toString();      // Convert date to string
    map["Duration"] = duration.toDays();  // Duration in days
    map["Exposure Time [%]"] = exposureTimePct;

    // Equity values
    map["Equity Final [$]"] = equityFinal;
    map["Equity Peak [$]"] = equityPeak;
    map["Equity Initial [$]"] = equityInitial;

    // Performance values
    map["Return [%]"] = returnPct;
    map["Buy & Hold Return [%]"] = buyHoldReturnPct;
    map["Return (Ann.) [%]"] = returnAnnPct;
    map["Volatility (Ann.) [%]"] = volatilityAnnPct;
    map["CAGR [%]"] = cagrPct;

    // Risk ratios
    map["Sharpe Ratio"] = sharpeRatio;
    map["Sortino Ratio"] = sortinoRatio;
    map["Calmar Ratio"] = calmarRatio;
    map["Alpha [%]"] = alphaPct;
    map["Beta"] = beta;

    // Drawdown values
    map["Max. Drawdown [%]"] = maxDrawdownPct;
    map["Avg. Drawdown [%]"] = avgDrawdownPct;
    // map["Max. Drawdown Duration"] = maxDrawdownDuration.toString();
    // map["Avg. Drawdown Duration"] = avgDrawdownDuration.toString();

    // Trade statistics
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
    os << "  Durée: " << stats.duration.toString() << "\n";
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
    os << "  Durée maximale: " << stats.maxDrawdownDuration.toString() << "\n";
    os << "  Durée moyenne: " << stats.avgDrawdownDuration.toString() << "\n";
    
    os << "\n-- Trades --\n";
    os << "  Nombre total: " << stats.numTrades << "\n";
    os << "  Gagnants: " << stats.numTPTrades << " (" << stats.pctTPTrades << "%)\n";
    os << "  Perdants: " << stats.numSLTrades << " (" << stats.pctSLTrades << "%)\n";
    os << "  Neutres: " << stats.numBETrades << " (" << stats.pctBETrades << "%)\n";
    os << "  Meilleur: " << stats.bestTradePct << "%\n";
    os << "  Pire: " << stats.worstTradePct << "%\n";
    os << "  Moyen: " << stats.avgTradePct << "%\n";
    os << "  Durée maximale: " << stats.maxTradeDuration.toString() << "\n";
    os << "  Durée moyenne: " << stats.avgTradeDuration.toString() << "\n";
    os << "  Facteur de profit: " << stats.profitFactor << "\n";
    os << "  Espérance: " << stats.expectancyPct << "%\n";
    os << "  SQN: " << stats.sqn << "\n";
    os << "  Critère de Kelly: " << stats.kellyCriterion << "\n";

    os << "\n-- Autres --\n";
    os << "  Moyenne des erreurs absolues (MAE): " << stats.avgMAE << "\n";
    os << "  Maximum des erreurs absolues (MAE): " << stats.maxMAE << "\n";
    os << "  Ulcer Index: " << stats.ulcerIndex << "\n";
    os << "  Ulcer Performance Index: " << stats.ulcerPerformanceIndex << "\n";
    os << "  Skewness: " << stats.skewness << "\n";
    os << "  Kurtosis: " << stats.kurtosis << "\n";
    os << "  Omega Ratio: " << stats.omegaRatio << "\n";
    
    return os;
}

// Stats computation with the new structure
Stats computeStats(
    const std::vector<std::shared_ptr<Trade>>& trades,
    const std::vector<double>& equity,
    const Data& data) {
    
    Stats stats;

    // Validate inputs
    if (equity.empty() || data.size() == 0) {
        return dummyStats();
    }

    // Convert trades to TradeData
    stats.trades.reserve(trades.size());
    for (const auto& trade : trades) {
        if (trade) {
            stats.trades.push_back(trade->data());
        }
    }

    // Store raw data for future analysis
    stats.equityCurve = equity;
    // stats.trades = trades;

    // Start and end dates of the backtest
    stats.start = data.at(0).date;  // First date of the dataset
    stats.end = data.at(data.size() - 1).date;  // Last date of the dataset
    stats.duration = stats.end - stats.start;  // Calculate the total duration of the backtest

    // Calculate drawdown: 1 - equity / max(equity)
    std::vector<double> dd(equity.size());
    std::vector<double> max_equity(equity.size());
    
    max_equity[0] = equity[0];
    for (size_t i = 1; i < equity.size(); ++i) {
        max_equity[i] = std::max(max_equity[i-1], equity[i]);
    }
    
    for (size_t i = 0; i < equity.size(); ++i) {
        dd[i] = 1.0 - equity[i] / max_equity[i];
    }

    // Calculate duration and peaks of drawdown
    DrawdownInfo dd_info = computeDrawdownDurationPeaks(dd);

    // Calculate market exposure time
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

    // Equity
    stats.equityFinal = equity.back();
    stats.equityPeak = *std::max_element(equity.begin(), equity.end());
    stats.equityInitial = equity.front();  // Initial equity

    // Total return
    stats.returnPct = equity.size() > 1 && std::abs(equity.front()) > 1e-10 ? 
        (equity.back() - equity.front()) / equity.front() * 100 : NaN;

    // Buy & Hold return - Use at() to access data
    size_t first_trading_bar = 1;  // Simplified compared to _indicator_warmup_nbars
    double initial_price = data.at(first_trading_bar).close;  // Modified
    double final_price = data.at(data.size() - 1).close;      // Modified
    stats.buyHoldReturnPct = (final_price - initial_price) / initial_price * 100;

    // Extract trade data
    std::vector<double> pl_values;
    std::vector<double> return_pct_values;
    std::vector<Duration> tradeDurations;
    
    for (const auto& trade : trades) {
        pl_values.push_back(trade->pl());
        return_pct_values.push_back(trade->plPercent());

        // Calculate trade duration using actual dates
        Date entryDate = data.at(trade->entryBar()).date;  // Modified
        Date exitDate = data.at(trade->exitBar()).date;    // Modified
        Duration tradeDuration = exitDate - entryDate;
        tradeDurations.push_back(tradeDuration);
    }

    // Number of trades
    size_t n_trades = trades.size();
    stats.numTrades = static_cast<unsigned int>(n_trades);

    // Trade statistics (wins/losses)
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

    // Best and worst trades
    if (!return_pct_values.empty()) {
        stats.bestTradePct = *std::max_element(return_pct_values.begin(), return_pct_values.end()) * 100;
        stats.worstTradePct = *std::min_element(return_pct_values.begin(), return_pct_values.end()) * 100;
    } else {
        stats.bestTradePct = NaN;
        stats.worstTradePct = NaN;
    }

    // Average return per trade
    stats.avgTradePct = geometricMean(return_pct_values) * 100;

    // Trade durations
    if (!tradeDurations.empty()) {
        // Find maximum duration
        stats.maxTradeDuration = *std::max_element(tradeDurations.begin(), tradeDurations.end(),
            [](const Duration& a, const Duration& b) { return a.seconds < b.seconds; });

        // Calculate average duration
        double totalSeconds = 0.0;
        for (const auto& dur : tradeDurations) {
            totalSeconds += dur.seconds;
        }
        stats.avgTradeDuration = Duration(totalSeconds / tradeDurations.size());
    }

    // Profit Factor (sum of gains / sum of losses in absolute value)
    double sum_wins = 0.0;
    double sum_losses = 0.0;
    
    for (double pl : pl_values) {
        if (pl > 0) sum_wins += pl;
        else if (pl < 0) sum_losses += std::abs(pl);
    }
    
    stats.profitFactor = sum_losses == 0 ? NaN : sum_wins / sum_losses;
    
    // Expectancy (expected gain)
    if (!return_pct_values.empty()) {
        double sum_returns = std::accumulate(return_pct_values.begin(), return_pct_values.end(), 0.0);
        stats.expectancyPct = (sum_returns / return_pct_values.size()) * 100;
    } else {
        stats.expectancyPct = NaN;
    }
    
    // SQN (System Quality Number)
    if (!pl_values.empty()) {
        double pl_mean = std::accumulate(pl_values.begin(), pl_values.end(), 0.0) / pl_values.size();

        // Standard deviation of profits/losses
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
            stats.kellyCriterion = stats.pctTPTrades * 0.01 - (1 - stats.pctTPTrades) / (avg_win / avg_loss);
        } else {
            stats.kellyCriterion = NaN;
        }
    } else {
        stats.kellyCriterion = NaN;
    }

    // Maximum Drawdown
    double max_dd = 0.0;
    for (double d : dd) {
        max_dd = std::max(max_dd, d);
    }
    stats.maxDrawdownPct = -max_dd * 100;  // Negative by convention

    // Average Drawdown
    if (!dd_info.peaks.empty()) {
        double avg_dd = std::accumulate(dd_info.peaks.begin(), dd_info.peaks.end(), 0.0) / dd_info.peaks.size();
        stats.avgDrawdownPct = -avg_dd * 100;  // Negative by convention
    } else {
        stats.avgDrawdownPct = NaN;
    }

    // Drawdown durations
    if (!dd_info.durations.empty()) {
        // For each drawdown period, use the actual dates
        std::vector<Duration> realDurations;
        
        for (size_t i = 1; i < dd_info.zeroIndices.size(); ++i) {
            size_t start = dd_info.zeroIndices[i-1];
            size_t end = dd_info.zeroIndices[i];
            
            if (end - start <= 1) continue;

            // Convert from bar index to actual dates
            Date startDate = data.at(start).date;
            Date endDate = data.at(end).date;
            Duration realDuration = endDate - startDate;
            
            realDurations.push_back(realDuration);
        }

        // Find maximum duration
        if (!realDurations.empty()) {
            stats.maxDrawdownDuration = *std::max_element(realDurations.begin(), realDurations.end(),
                [](const Duration& a, const Duration& b) { return a.seconds < b.seconds; });

            // Calculate average duration
            double totalSeconds = 0.0;
            for (const auto& dur : realDurations) {
                totalSeconds += dur.seconds;
            }
            stats.avgDrawdownDuration = Duration(totalSeconds / realDurations.size());
        } else {
            stats.maxDrawdownDuration = Duration();
            stats.avgDrawdownDuration = Duration();
        }
    }

    // Calculate daily returns (simplified)
    std::vector<double> day_returns;
    for (size_t i = 1; i < equity.size(); ++i) {
        day_returns.push_back(equity[i] / equity[i-1] - 1.0);
    }

    // Geometric mean of daily returns
    double gmean_day_return = geometricMean(day_returns);

    // Annual trading days
    const double annual_trading_days = 252;  // Trading days per year (standard)

    // Annualized return
    stats.returnAnnPct = (std::pow(1 + gmean_day_return, annual_trading_days) - 1) * 100;

    // Annualized volatility
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

    // CAGR (Compound Annual Growth Rate)
    double years = stats.duration.toYears();
    if (years > 0) {
        stats.cagrPct = (std::pow(equity.back() / equity.front(), 1.0 / years) - 1) * 100;
    } else {
        stats.cagrPct = NaN;
    }

    // Sharpe Ratio
    const double risk_free_rate = 0.0;  // Risk-free rate (omitted parameter)
    double excess_return = stats.returnAnnPct / 100 - risk_free_rate;
    double volatility = stats.volatilityAnnPct / 100;
    
    if (volatility > 1e-6) {
        stats.sharpeRatio = excess_return / volatility;
    } else {
        stats.sharpeRatio = NaN;
    }

    // Sortino Ratio
    const double mar = 0.0;  // Minimum acceptable return, can be parameterized
    const double risk_free_annual = risk_free_rate * annual_trading_days; // Annualize the risk-free rate

    if (!day_returns.empty()) {
        // Excess return already calculated for Sharpe
        double excess_return = stats.returnAnnPct / 100 - risk_free_rate;

        // Use the new function to calculate Sortino
        stats.sortinoRatio = calculateSortinoRatio(day_returns, excess_return, mar, annual_trading_days);
    } else {
        stats.sortinoRatio = NaN;
    }

    // Calmar Ratio
    if (max_dd > 0) {
        stats.calmarRatio = (stats.cagrPct / 100) / max_dd;
    } else {
        stats.calmarRatio = NaN;
    }

    // Calculate beta and alpha (CAPM model)
    std::vector<double> equity_log_returns;
    std::vector<double> market_log_returns;

    // Calculate log returns for equity and market
    for (size_t i = 1; i < equity.size(); ++i) {
        equity_log_returns.push_back(std::log(equity[i] / equity[i-1]));
    }

    for (size_t i = 1; i < data.size(); ++i) {
        market_log_returns.push_back(std::log(data.at(i).close / data.at(i-1).close));  
    }

    // Calculate beta only if we have enough data
    if (equity_log_returns.size() > 1 && market_log_returns.size() > 1) {
        // Calculate means
        double equity_mean = std::accumulate(equity_log_returns.begin(), equity_log_returns.end(), 0.0) / equity_log_returns.size();
        double market_mean = std::accumulate(market_log_returns.begin(), market_log_returns.end(), 0.0) / market_log_returns.size();

        // Calculate covariance matrix elements
        double cov_em = 0.0;  // Covariance equity-market
        double var_m = 0.0;   // Market variance

        size_t n = std::min(equity_log_returns.size(), market_log_returns.size());
        for (size_t i = 0; i < n; ++i) {
            cov_em += (equity_log_returns[i] - equity_mean) * (market_log_returns[i] - market_mean);
            var_m += (market_log_returns[i] - market_mean) * (market_log_returns[i] - market_mean);
        }

        // Avoid division by zero
        if (n > 1 && var_m > 0) {
            cov_em /= (n - 1);
            var_m /= (n - 1);

            // Calculate beta
            stats.beta = cov_em / var_m;

            // Calculate alpha (CAPM)
            const double risk_free_rate = 0.0;  // Risk-free rate (omitted parameter)
            stats.alphaPct = stats.returnPct - risk_free_rate * 100 - stats.beta * (stats.buyHoldReturnPct - risk_free_rate * 100);
        } else {
            stats.beta = NaN;
            stats.alphaPct = NaN;
        }
    } else {
        stats.beta = NaN;
        stats.alphaPct = NaN;
    }


    // Calculate MAE for each trade
    std::vector<double> mae_values;
    for (const auto& trade : trades) {
        // Find the worst price during the trade
        double worst_price = findWorstPrice(trade, data);
        double entry_price = trade->entryPrice();
        double mae = trade->isLong() ? 
            (worst_price - entry_price) / entry_price : 
            (entry_price - worst_price) / entry_price;
        mae_values.push_back(mae);
    }

    // Calculate MAE statistics - Check if we have trades
    if (!mae_values.empty()) {
        stats.avgMAE = std::accumulate(mae_values.begin(), mae_values.end(), 0.0) / mae_values.size() * 100.0;
        stats.maxMAE = *std::min_element(mae_values.begin(), mae_values.end()) * 100.0;
    } else {
        stats.avgMAE = NaN;
        stats.maxMAE = NaN;
    }

    // Ulcer Index - square root of the average of the squared drawdowns
    double ulcer_sum = 0.0;
    for (double d : dd) {
        ulcer_sum += d * d;
    }
    stats.ulcerIndex = std::sqrt(ulcer_sum / dd.size()) * 100.0;

    // Return on Ulcer Index
    stats.ulcerPerformanceIndex = stats.returnAnnPct / stats.ulcerIndex;

    stats.skewness = calculateSkewness(day_returns);
    stats.kurtosis = calculateKurtosis(day_returns);

    double threshold = 0.0; // Return threshold (could be risk-free rate)
    double omega_pos = 0.0, omega_neg = 0.0;

    for (double ret : day_returns) {
        if (ret >= threshold) {
            omega_pos += (ret - threshold);
        } else {
            omega_neg += (threshold - ret);
        }
    }

    stats.omegaRatio = omega_neg > 0 ? omega_pos / omega_neg : NaN;
    
    return stats;
}

// dummyStats version which initializes all fields to NaN
Stats dummyStats() {
    Stats stats;

    // Initialize empty vectors
    stats.equityCurve.clear();
    stats.trades.clear();

    // Initialize all fields to NaN
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
    stats.numTPTrades = 0;
    stats.pctTPTrades = NaN;
    stats.numSLTrades = 0;
    stats.pctSLTrades = NaN;
    stats.numBETrades = 0;
    stats.pctBETrades = NaN;
    stats.numManualTrades = 0;
    stats.pctManualTrades = NaN;
    stats.numUnknownTrades = 0;
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
    stats.avgMAE = NaN;
    stats.maxMAE = NaN;
    stats.ulcerIndex = NaN;
    stats.ulcerPerformanceIndex = NaN;
    stats.skewness = NaN;
    stats.kurtosis = NaN;
    stats.omegaRatio = NaN;
    
    return stats;
}

// Pour la compatibilité avec le code existant
// std::map<std::string, double> computeStatsMap(
//     const std::vector<std::shared_ptr<Trade>>& trades,
//     const std::vector<double>& equity,
//     const Data& data) {
    
//     Stats stats = computeStats(trades, equity, data);
//     return stats.toMap();
// }

} // namespace be