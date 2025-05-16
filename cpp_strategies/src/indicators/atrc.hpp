#pragma once
#include "base_indicator.hpp"
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>

/**
 * Average True Range (ATR) calculated incrementally
 */
class ATRC : public IncrementalIndicator {
private:
    int period;
    double current_atr = 0.0;
    std::deque<double> true_range_history;
    
public:
    ATRC(int period);
    double initialize_with_history(
        const std::vector<double>& high_history,
        const std::vector<double>& low_history);
    double update(double high, double low);
    double get_value() const;
};

inline ATRC::ATRC(int period) : period(period) {}

inline double ATRC::initialize_with_history(
    const std::vector<double>& high_history,
    const std::vector<double>& low_history) 
{
    if (high_history.size() < static_cast<size_t>(period + 1)) {
        return 0.0;
    }
    
    // Calculate True Ranges for the entire period
    std::vector<double> true_ranges;
    for (size_t i = 1; i < high_history.size(); ++i) {
        true_ranges.push_back(high_history[i] - low_history[i]);
    }
    
    // Calculate initial ATR as simple average of TR
    if (true_ranges.size() >= static_cast<size_t>(period)) {
        double sum = 0.0;
        for (size_t i = true_ranges.size() - period; i < true_ranges.size(); ++i) {
            sum += true_ranges[i];
        }
        current_atr = sum / period;
        is_initialized = true;
    }
    
    return current_atr;
}

inline double ATRC::update(double high, double low) {
    if (!is_initialized) {
        double tr = high - low;
        
        true_range_history.push_back(tr);
        
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
    double tr = high - low;
    
    // Update ATR using Wilder's smoothing method
    current_atr = ((current_atr * (period - 1)) + tr) / period;
    
    return current_atr;
}

inline double ATRC::get_value() const {
    return current_atr;
}