#pragma once

#include <string>
#include <memory>
#include <chrono>

// Forward declarations
class Broker;
class Order;

/**
 * @brief Executed trade representation
 */
class Trade {
public:
    Trade(std::shared_ptr<Broker> broker,
          double size,
          double entryPrice,
          size_t entryBar,
          const std::string& tag = "");
    
    void close(double portion = 1.0);
    
    // Getters
    double size() const { return _size; }
    double entryPrice() const { return _entryPrice; }
    double exitPrice() const { return _exitPrice; }
    size_t entryBar() const { return _entryBar; }
    size_t exitBar() const { return _exitBar; }
    std::string tag() const { return _tag; }
    
    // PnL calculations
    double pl() const;
    double plPercent() const;
    double value() const;
    
    // Stop loss and take profit management
    double sl() const;
    void sl(double price);
    double tp() const;
    void tp(double price);
    
    bool isLong() const { return _size > 0; }
    bool isShort() const { return _size < 0; }
    bool isClosed() const { return _exitPrice > 0; }
    
    // Pour afficher le trade (debug/logging)
    std::string toString() const;
    
    // Accesseurs pour les ordres SL/TP
    std::shared_ptr<Order> slOrder() const { return _slOrder; }
    std::shared_ptr<Order> tpOrder() const { return _tpOrder; }
    
private:
    std::shared_ptr<Broker> _broker;
    double _size;
    double _entryPrice;
    double _exitPrice;
    size_t _entryBar;
    size_t _exitBar;
    std::string _tag;
    double _commissions;
    
    // SL/TP orders
    std::shared_ptr<Order> _slOrder;
    std::shared_ptr<Order> _tpOrder;
    
    // Méthodes auxiliaires pour que le broker puisse modifier les propriétés du trade
    void setExitPrice(double price);
    void setExitBar(size_t bar);
    void setSize(double size);
    void setCommissions(double commissions);
    void setSlOrder(Order order);
    void setTpOrder(Order order);
    
    friend class Broker;
};