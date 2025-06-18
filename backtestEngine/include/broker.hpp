#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "order.hpp"
#include "trade.hpp"

namespace be {

// Forward declarations
class Data;

/**
 * @brief Order execution and position management
 */
class Broker : public std::enable_shared_from_this<Broker> {
public:
    Broker(std::shared_ptr<Data> data,
           double cash = 10000.0,
           double spread = 0.0,
           double commission = 0.0,
           double margin = 1.0,
           bool tradeOnClose = false,
           bool hedging = false,
           bool exclusiveOrders = false);
    
    void next();
    
    Order newOrder(double size,
                  double limit = 0.0,
                  double stop = 0.0,
                  double sl = 0.0,
                  double tp = 0.0,
                  const std::string& tag = "",
                  double slPoints = 0.0,
                  double tpPoints = 0.0,
                  std::shared_ptr<Trade> trade = nullptr);
    
    void cancelOrder(const Order& order);

    /**
     * @brief Finalise les trades sans avancer l'itérateur de données
     * 
     * Cette méthode est utilisée à la fin du backtest pour traiter les ordres
     * de clôture sans essayer d'avancer l'itérateur de données qui est déjà à la fin.
     */
    void finalizeOrders();
    
    // Broker state accessors
    double lastPrice() const;
    double cash() const { return _cash; }
    double equity() const;
    double marginAvailable() const;
    std::vector<Order> orders() const { return _orders; }
    const std::vector<std::shared_ptr<Trade>>& trades() const { return _trades; }
    const std::vector<std::shared_ptr<Trade>>& closedTrades() const { return _closedTrades; }
    const std::vector<double>& getEquityCurve() const { return _equityCurve; }

    
private:
    std::shared_ptr<Data> _data;
    double _cash;
    double _spread;
    double _commission;
    double _leverage;
    bool _tradeOnClose;
    bool _hedging;
    bool _exclusiveOrders;
    size_t _currentBar;
    
    std::vector<Order> _orders;
    std::vector<std::shared_ptr<Trade>> _trades;
    std::vector<std::shared_ptr<Trade>> _closedTrades;
    std::vector<double> _equityCurve;
    
    // Helper methods
    double adjustedPrice(double size, double price = 0.0) const;
    double calculateCommission(double size, double price) const;
    void processOrders();
    void openTrade(double price, double size, double sl, double tp, 
                  size_t barIndex, const std::string& tag, const Order& order);
    void closeTrade(std::shared_ptr<Trade> trade, double price, size_t barIndex);
    void reduceTrade(std::shared_ptr<Trade> trade, double price, double size, size_t barIndex);
    
    friend class Order;
    friend class Trade;
    friend class Position;
};

} // namespace be