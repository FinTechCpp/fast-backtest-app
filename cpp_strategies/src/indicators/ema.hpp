#pragma once
#include "base_indicator.hpp"
#include <vector>
#include <deque>
#include <cmath>

/**
 * Exponential Moving Average (EMA) calculated incrementally
 */
class EMA : public IncrementalIndicator {
private:
    int period;
    double multiplier;
    double current_ema = 0.0;
    std::deque<double> price_history;
    
public:
    EMA(int period);
    double initialize_with_history(const std::vector<double>& price_history);
    double update(double price);
    double get_value() const;
};

// Implementation
inline EMA::EMA(int period) : period(period) {
    multiplier = 2.0 / (period + 1.0);
}

inline double EMA::initialize_with_history(const std::vector<double>& price_history) {
    if (price_history.size() < static_cast<size_t>(period)) {
        return 0.0;
    }
    
    // Calculate initial SMA
    double sum = 0.0;
    for (size_t i = price_history.size() - period; i < price_history.size(); ++i) {
        sum += price_history[i];
    }
    current_ema = sum / period;
    
    // Apply EMA formula for remaining prices
    for (size_t i = price_history.size() - period; i < price_history.size(); ++i) {
        current_ema = (price_history[i] - current_ema) * multiplier + current_ema;
    }
    
    is_initialized = true;
    return current_ema;
}

inline double EMA::update(double price) {
    if (!is_initialized) {
        price_history.push_back(price);
        if (price_history.size() >= static_cast<size_t>(period)) {
            std::vector<double> history(price_history.begin(), price_history.end());
            return initialize_with_history(history);
        }
        return 0.0;
    }
    
    // Calculate new EMA value
    current_ema = (price - current_ema) * multiplier + current_ema;
    return current_ema;
}

inline double EMA::get_value() const {
    return current_ema;
}