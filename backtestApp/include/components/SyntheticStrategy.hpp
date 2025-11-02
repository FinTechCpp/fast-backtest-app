// Future tests for the backtest engine with synthetic data and a strategy that triggers specific situations

#pragma once

#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <iostream>

class SyntheticStrategy : public be::Strategy {
private:

    size_t candleCount = 0;

public:
    SyntheticStrategy(std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data)
    : be::Strategy(broker, data) {}
    
    void init() override {}
    
    void next() override {  
        candleCount++;

        if (candleCount == 5) {
            _broker->closeAllTrades();
        }

        if (candleCount == 42) {
            int breakPoint = 0;
        }

        if (candleCount == 1 || candleCount == 8 || candleCount == 28 || candleCount == 40 || candleCount == 42) 
            _broker->submitOrder(
                1, 
                be::OrderSide::BUY, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::price(900), 
                be::TPValue::price(1100), 
                nullptr
            );

        if (candleCount == 60 || candleCount == 88 || candleCount == 108) 
            _broker->submitOrder(
                1, 
                be::OrderSide::BUY, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::price(970), 
                be::TPValue::price(1030), 
                nullptr
            );

        if (candleCount == 125) {
            _broker->closeAllTrades();
        }

        if (candleCount == 121 || candleCount == 128 || candleCount == 148) 
            _broker->submitOrder(
                1, 
                be::OrderSide::SELL, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::price(1100), 
                be::TPValue::price(900), 
                nullptr
            );

        if (candleCount == 208 || candleCount == 228) 
            _broker->submitOrder(
                1, 
                be::OrderSide::SELL, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::price(1030), 
                be::TPValue::price(970), 
                nullptr
            );
    }
};

