#pragma once
#include "base_indicator.hpp"
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>

/**
 * Stochastic Oscillator calculated incrementally
 */
class STOCH : public IncrementalIndicator {
private:
    int fastk_period;
    int slowk_period;
    int slowd_period;
    
    std::deque<double> high_buffer;
    std::deque<double> low_buffer;
    std::deque<double> close_buffer;
    
    std::deque<double> raw_k_values;
    std::deque<double> k_values;
    std::deque<double> d_values;
    
    double current_k = 0.0;
    double current_d = 0.0;
    
public:
    STOCH(int fastk, int slowk, int slowd);
    std::pair<double, double> initialize_with_history(
        const std::vector<double>& high_history,
        const std::vector<double>& low_history,
        const std::vector<double>& close_history);
    std::pair<double, double> update(double high, double low, double close);
    std::pair<double, double> get_values() const;
};

inline STOCH::STOCH(int fastk, int slowk, int slowd) : fastk_period(fastk), slowk_period(slowk), slowd_period(slowd) {}

inline std::pair<double, double> STOCH::initialize_with_history(
    const std::vector<double>& high_history,
    const std::vector<double>& low_history,
    const std::vector<double>& close_history) 
{

    if (high_history.size() < static_cast<size_t>(fastk_period)) {
        return {0.0, 0.0};
    }

    // Initialize buffers
    high_buffer.assign(high_history.end() - fastk_period, high_history.end());
    low_buffer.assign(low_history.end() - fastk_period, low_history.end());
    close_buffer.assign(close_history.end() - fastk_period, close_history.end());

    // Calculate initial raw K values
    for (size_t i = 0; i <= high_history.size() - fastk_period; ++i) {
        double period_high = *std::max_element(high_history.begin() + i, high_history.begin() + i + fastk_period);
        double period_low = *std::min_element(low_history.begin() + i, low_history.begin() + i + fastk_period);
        double close = close_history[i + fastk_period - 1];
        
        double raw_k = 0.0;
        if (period_high > period_low) {
            raw_k = 100.0 * ((close - period_low) / (period_high - period_low));
        }
        
        raw_k_values.push_back(raw_k);
    }

    // Apply K smoothing
    if (raw_k_values.size() >= static_cast<size_t>(slowk_period)) {
        for (size_t i = 0; i <= raw_k_values.size() - slowk_period; ++i) {
            double sum = 0.0;
            for (size_t j = i; j < i + slowk_period; ++j) {
                sum += raw_k_values[j];
            }
            double smooth_k = sum / slowk_period;
            k_values.push_back(smooth_k);
        }
    }

    // Calculate D values
    if (k_values.size() >= static_cast<size_t>(slowd_period)) {
        for (size_t i = 0; i <= k_values.size() - slowd_period; ++i) {
            double sum = 0.0;
            for (size_t j = i; j < i + slowd_period; ++j) {
                sum += k_values[j];
            }
            double smooth_d = sum / slowd_period;
            d_values.push_back(smooth_d);
        }
    }

    if (!k_values.empty() && !d_values.empty()) {
        current_k = k_values.back();
        current_d = d_values.back();
        is_initialized = true;
    }

    return {current_k, current_d};
}

inline std::pair<double, double> STOCH::update(double high, double low, double close) {
    // Add new prices and remove oldest if needed
    high_buffer.push_back(high);
    low_buffer.push_back(low);
    close_buffer.push_back(close);

    if (high_buffer.size() > static_cast<size_t>(fastk_period)) {
        high_buffer.pop_front();
        low_buffer.pop_front();
        close_buffer.pop_front();
    }

    if (!is_initialized) {
        if (high_buffer.size() == static_cast<size_t>(fastk_period)) {
            std::vector<double> high_hist(high_buffer.begin(), high_buffer.end());
            std::vector<double> low_hist(low_buffer.begin(), low_buffer.end());
            std::vector<double> close_hist(close_buffer.begin(), close_buffer.end());
            return initialize_with_history(high_hist, low_hist, close_hist);
        }
        return {0.0, 0.0};
    }

    // Calculate new raw K value
    double period_high = *std::max_element(high_buffer.begin(), high_buffer.end());
    double period_low = *std::min_element(low_buffer.begin(), low_buffer.end());

    double raw_k = 0.0;
    if (period_high > period_low) {
        raw_k = 100.0 * ((close - period_low) / (period_high - period_low));
    }

    raw_k_values.push_back(raw_k);
    if (raw_k_values.size() > static_cast<size_t>(fastk_period + slowk_period)) {
        raw_k_values.pop_front();
    }

    // Calculate new smoothed K value
    if (raw_k_values.size() >= static_cast<size_t>(slowk_period)) {
        double sum = 0.0;
        for (size_t i = raw_k_values.size() - slowk_period; i < raw_k_values.size(); ++i) {
            sum += raw_k_values[i];
        }
        double smooth_k = sum / slowk_period;
        k_values.push_back(smooth_k);
        
        if (k_values.size() > static_cast<size_t>(fastk_period + slowk_period)) {
            k_values.pop_front();
        }
    }

    // Calculate new D value
    if (k_values.size() >= static_cast<size_t>(slowd_period)) {
        double sum = 0.0;
        for (size_t i = k_values.size() - slowd_period; i < k_values.size(); ++i) {
            sum += k_values[i];
        }
        double smooth_d = sum / slowd_period;
        d_values.push_back(smooth_d);
        
        if (d_values.size() > static_cast<size_t>(fastk_period + slowk_period)) {
            d_values.pop_front();
        }
    }

    // Update current values
    if (!k_values.empty() && !d_values.empty()) {
        current_k = k_values.back();
        current_d = d_values.back();
    }

    return {current_k, current_d};
}

inline std::pair<double, double> STOCH::get_values() const {
    return {current_k, current_d};
}