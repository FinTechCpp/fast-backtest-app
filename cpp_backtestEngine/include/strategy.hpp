#pragma once

#include <memory>
#include <vector>
#include <string>
#include "order.hpp"

namespace be {

// Forward declarations
class Broker;
class Data;
class Position;
class Trade;

/**
 * @brief Abstract base class for trading strategies
 */
class Strategy {
public:
    Strategy(std::shared_ptr<Broker> broker, std::shared_ptr<Data> data);
    virtual ~Strategy() = default;

    /**
     * @brief Initialize indicators and strategy parameters
     * Must be implemented by derived classes
     */
    virtual void init() = 0;
    
    /**
     * @brief Execute strategy logic for current bar
     * Must be implemented by derived classes
     */
    virtual void next() = 0;
    
    /**
     * @brief Place a buy order
     */
    Order buy(double size = 1.0,
              double limit = 0.0,
              double stop = 0.0, 
              double sl = 0.0, 
              double tp = 0.0,
              double sl_points = 0.0,
              double tp_points = 0.0,
              const std::string& tag = "");
    
    /**
     * @brief Place a sell order
     */
    Order sell(double size = 1.0,
               double limit = 0.0,
               double stop = 0.0, 
               double sl = 0.0, 
               double tp = 0.0,
               double sl_points = 0.0,
               double tp_points = 0.0,
               const std::string& tag = "");
    
    // Access properties
    double getEquity() const;
    std::shared_ptr<Data> getData() const;
    Position getPosition() const;
    std::vector<Order> getOrders() const;
    std::vector<Trade> getTrades() const;
    std::vector<Trade> getClosedTrades() const;
    
protected:
    std::shared_ptr<Broker> _broker;
    std::shared_ptr<Data> _data;
};

} // namespace be