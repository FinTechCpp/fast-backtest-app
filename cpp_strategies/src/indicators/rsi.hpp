#pragma once
#include "base_indicator.hpp"
#include <vector>
#include <deque>
#include <cmath>

/**
 * Relative Strength Index (RSI) calculated incrementally
 */
class RSI : public IncrementalIndicator {
private:
    int period;
    double current_rsi = 0.0;
    double prev_close = 0.0;
    
    double avg_gain = 0.0;
    double avg_loss = 0.0;
    
    std::deque<double> close_history;
    bool first_avg_calculated = false;
    
public:
    RSI(int period);
    double initialize_with_history(const std::vector<double>& price_history);
    double update(double price);
    double get_value() const;
};

// Implémentation des méthodes...
inline RSI::RSI(int period) : period(period) {}

inline double RSI::initialize_with_history(const std::vector<double>& price_history) {
    if (price_history.size() < static_cast<size_t>(period + 1)) {
        return 0.0;
    }
    
    // Calculate first average gain and loss
    double total_gain = 0.0;
    double total_loss = 0.0;
    
    for (size_t i = 1; i < static_cast<size_t>(period) + 1; ++i) {
        double change = price_history[i] - price_history[i-1];
        if (change > 0) {
            total_gain += change;
        } else {
            total_loss -= change;  // Convert to positive value
        }
    }
    
    avg_gain = total_gain / period;
    avg_loss = total_loss / period;
    
    // Calculate initial RSI value
    if (avg_loss == 0.0) {
        current_rsi = 100.0;
    } else {
        double rs = avg_gain / avg_loss;
        current_rsi = 100.0 - (100.0 / (1.0 + rs));
    }
    
    // Set the previous close
    prev_close = price_history[period];
    first_avg_calculated = true;
    is_initialized = true;
    
    return current_rsi;
}

inline double RSI::update(double price) {
    if (!is_initialized) {
        close_history.push_back(price);
        
        if (close_history.size() >= static_cast<size_t>(period + 1)) {
            std::vector<double> history(close_history.begin(), close_history.end());
            return initialize_with_history(history);
        }
        
        if (!close_history.empty() && close_history.size() > 1) {
            prev_close = close_history[close_history.size() - 2];
        }
        
        return 0.0;
    }
    
    // Calculate current gain/loss
    double change = price - prev_close;
    double current_gain = (change > 0) ? change : 0.0;
    double current_loss = (change < 0) ? -change : 0.0;
    
    // Update averages using Wilder's smoothing method
    avg_gain = ((avg_gain * (period - 1)) + current_gain) / period;
    avg_loss = ((avg_loss * (period - 1)) + current_loss) / period;
    
    // Calculate RSI
    if (avg_loss == 0.0) {
        current_rsi = 100.0;
    } else {
        double rs = avg_gain / avg_loss;
        current_rsi = 100.0 - (100.0 / (1.0 + rs));
    }
    
    prev_close = price;
    return current_rsi;
}

inline double RSI::get_value() const {
    return current_rsi;
}
