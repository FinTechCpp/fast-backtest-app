#pragma once
#include "base_indicator.hpp"
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>

/**
 * Average True Range (ATR) calculated incrementally
 */
class ATR : public IncrementalIndicator {
private:
    int period;
    double current_atr = 0.0;
    double previous_close = 0.0;
    std::deque<double> true_range_history;
    
public:
    ATR(int period);
    double initialize_with_history(
        const std::vector<double>& high_history,
        const std::vector<double>& low_history,
        const std::vector<double>& close_history);
    double update(double high, double low, double close);
    double get_value() const;
};

// Implémentation des méthodes...
inline ATR::ATR(int period) : period(period) {}

inline double ATR::initialize_with_history(
    const std::vector<double>& high_history,
    const std::vector<double>& low_history,
    const std::vector<double>& close_history) 
{
    if (high_history.size() < static_cast<size_t>(period + 1)) {
        return 0.0;
    }
    
    // Calculate True Ranges for the entire period
    std::vector<double> true_ranges;
    for (size_t i = 1; i < high_history.size(); ++i) {
        double high = high_history[i];
        double low = low_history[i];
        double prev_close = close_history[i-1];
        
        double tr = std::max({
            high - low,
            std::abs(high - prev_close),
            std::abs(low - prev_close)
        });
        true_ranges.push_back(tr);
    }
    
    // Calculate initial ATR as simple average of TR
    if (true_ranges.size() >= static_cast<size_t>(period)) {
        double sum = 0.0;
        for (size_t i = true_ranges.size() - period; i < true_ranges.size(); ++i) {
            sum += true_ranges[i];
        }
        current_atr = sum / period;
        previous_close = close_history.back();
        is_initialized = true;
    }
    
    return current_atr;
}

inline double ATR::update(double high, double low, double close) {
    if (!is_initialized) {
        if (previous_close == 0.0) {
            previous_close = close;
            return 0.0;
        }
        
        double tr = std::max({
            high - low,
            std::abs(high - previous_close),
            std::abs(low - previous_close)
        });
        
        true_range_history.push_back(tr);
        previous_close = close;
        
        if (true_range_history.size() >= static_cast<size_t>(period)) {
            double sum = 0.0;
            for (const auto& tr : true_range_history) {
                sum += tr;
            }
            current_atr = sum / period;
            is_initialized = true;
            return current_atr;
        }
        
        return 0.0;
    }
    
    // Calculate new True Range
    double tr = std::max({
        high - low,
        std::abs(high - previous_close),
        std::abs(low - previous_close)
    });
    
    // Update ATR using Wilder's smoothing method
    current_atr = ((current_atr * (period - 1)) + tr) / period;
    previous_close = close;
    
    return current_atr;
}

inline double ATR::get_value() const {
    return current_atr;
}