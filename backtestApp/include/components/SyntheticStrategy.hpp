// Future fichier de test pour le backtest engine avec des données synthétiques et une stratégie qui provoque des situations précises

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

        if (candleCount == 8 || candleCount == 28) 
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

        if (candleCount == 48) 
            _broker->submitOrder(
                1, 
                be::OrderSide::BUY, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::price(920), 
                be::TPValue::price(1100), 
                nullptr
            );

        if (candleCount == 68) 
            _broker->submitOrder(
                1, 
                be::OrderSide::BUY, 
                be::OrderType::MARKET, 
                std::nullopt, 
                std::nullopt, 
                be::SLValue::price(900), 
                be::TPValue::price(1062), 
                nullptr
            );

        // if (candleCount % 10 == 8) {
        //     // _broker->cancelAllTrades();
        //     // _broker->cancelAllOrders();
        // }
        // else if (candleCount % 20 == 10) {
        //     _broker->submitOrder(
        //         1, 
        //         be::OrderSide::SELL, 
        //         be::OrderType::MARKET, 
        //         std::nullopt, 
        //         std::nullopt, 
        //         be::SLValue::price(1100), 
        //         be::TPValue::price(900), 
        //         nullptr
        //     );
        // }
        // else if (candleCount % 20 == 0) {
        //     _broker->submitOrder(
        //         1, 
        //         be::OrderSide::BUY, 
        //         be::OrderType::MARKET, 
        //         std::nullopt, 
        //         std::nullopt, 
        //         be::SLValue::price(900), 
        //         be::TPValue::price(1100), 
        //         nullptr
        //     );
        // }
    }
};

