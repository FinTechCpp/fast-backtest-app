#pragma once
#include "base_indicator.hpp"
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>

/**
 * Average True Range (ATR) logarithmique - retourne log(ATR + 1)
 */
class ATRLOG : public ATR {
private:

    
public:
    ATRLOG(int period, const std::string& name = "ATRLOG")
        : ATR(period, name) {}

    double initialize_with_history(const std::vector<BasicCandle>& history) override;
    double update(const BasicCandle& candle) override;
    double get_value() const override;
};

inline double ATRLOG::initialize_with_history(const std::vector<BasicCandle>& history)
{
    return std::log(1 + ATR::initialize_with_history(history));
}

inline double ATRLOG::update(const BasicCandle& candle)
{
    return std::log(1 + ATR::update(candle));
}

inline double ATRLOG::get_value() const
{
    return std::log(1 + ATR::get_value());
}