#pragma once

#include <memory>
#include <map>
#include <string>
#include <functional>
#include "data.hpp"
#include "broker.hpp"
#include "strategy.hpp"

/**
 * @brief Main backtesting engine
 */
class Backtest {
public:
    Backtest(std::shared_ptr<Data> data,
             std::function<std::shared_ptr<Strategy>(std::shared_ptr<Broker>, std::shared_ptr<Data>)> strategyFactory,
             double cash = 10000.0,
             double spread = 0.0,
             double commission = 0.0,
             double margin = 1.0,
             bool tradeOnClose = false,
             bool hedging = false,
             bool exclusiveOrders = false,
             bool finalizeTrades = false);
    
    /**
     * @brief Execute backtest with current parameters
     * 
     * @return Map of performance statistics
     */
    std::map<std::string, double> run();
    
    /**
     * @brief Optimize strategy parameters
     * 
     * @param params Parameter space to search
     * @param maximize Metric to maximize (e.g. "SQN" or "Return")
     * @return Best parameters and statistics
     */
    std::pair<std::map<std::string, double>, std::map<std::string, double>> 
    optimize(const std::map<std::string, std::vector<double>>& params,
             const std::string& maximize = "SQN");
    
private:
    std::shared_ptr<Data> _data;
    std::function<std::shared_ptr<Strategy>(std::shared_ptr<Broker>, std::shared_ptr<Data>)> _strategyFactory;
    double _cash;
    double _spread;
    double _commission;
    double _margin;
    bool _tradeOnClose;
    bool _hedging;
    bool _exclusiveOrders;
    bool _finalizeTrades;
    std::map<std::string, double> _lastResults;
};