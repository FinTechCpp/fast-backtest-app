#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <deque>

/**
 * Base class for all incremental indicators
 */
class IncrementalIndicator {
protected:
    bool is_initialized = false;
    
public:
    virtual ~IncrementalIndicator() = default;
    bool requires_initialization() const { return !is_initialized; }
    bool initialized() const { return is_initialized; }
};

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
    EMA(int period) : period(period) {
        multiplier = 2.0 / (period + 1.0);
    }
    
    double initialize_with_history(const std::vector<double>& price_history) {
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
    
    double update(double price) {
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
    
    double get_value() const {
        return current_ema;
    }
};

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
    STOCH(int fastk, int slowk, int slowd) 
        : fastk_period(fastk), slowk_period(slowk), slowd_period(slowd) {}
    
    std::pair<double, double> initialize_with_history(
        const std::vector<double>& high_history,
        const std::vector<double>& low_history,
        const std::vector<double>& close_history) {
        
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
    
    std::pair<double, double> update(double high, double low, double close) {
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
    
    std::pair<double, double> get_values() const {
        return {current_k, current_d};
    }
};

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
    RSI(int period) : period(period) {}
    
    double initialize_with_history(const std::vector<double>& price_history) {
        if (price_history.size() < static_cast<size_t>(period + 1)) {
            return 0.0;
        }
        
        // Calculate first average gain and loss
        double total_gain = 0.0;
        double total_loss = 0.0;
        
        for (size_t i = 1; i < period + 1; ++i) {
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
    
    double update(double price) {
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
    
    double get_value() const {
        return current_rsi;
    }
};

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
    ATR(int period) : period(period) {}
    
    double initialize_with_history(
        const std::vector<double>& high_history,
        const std::vector<double>& low_history,
        const std::vector<double>& close_history) {
        
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
    
    double update(double high, double low, double close) {
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
    
    double get_value() const {
        return current_atr;
    }
};