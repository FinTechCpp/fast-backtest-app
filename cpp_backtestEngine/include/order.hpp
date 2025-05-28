#pragma once

#include <string>
#include <memory>
#include <map>

// Forward declarations
class Broker;
class Trade;

/**
 * @brief Trading order representation
 */
class Order {
public:
    Order(std::shared_ptr<Broker> broker,
          double size,
          double limitPrice = 0.0,
          double stopPrice = 0.0,
          double slPrice = 0.0,
          double tpPrice = 0.0,
          std::shared_ptr<Trade> parentTrade = nullptr,
          const std::string& tag = "",
          double slPoints = 0.0,
          double tpPoints = 0.0);
    
    void cancel();
    
    // Getters
    double size() const { return _size; }
    double limit() const { return _limitPrice; }
    double stop() const { return _stopPrice; }
    double sl() const { return _slPrice; }
    double tp() const { return _tpPrice; }
    double slPoints() const { return _slPoints; }
    double tpPoints() const { return _tpPoints; }
    std::string tag() const { return _tag; }
    std::shared_ptr<Trade> parentTrade() const { return _parentTrade; }
    
    bool isLong() const { return _size > 0; }
    bool isShort() const { return _size < 0; }
    bool isContingent() const;
    
    // Helper methods pour la modification des attributs (utilisé dans broker.next)
    // TODO : Methode assez moches, ils yu a des string hardcodés, à améliorer
    void _replace(const std::string& attr, double value);
    void _replace(std::map<std::string, double> changes);
    
    // Opérateur d'égalité pour trouver un ordre dans un vecteur
    bool operator==(const Order& other) const;
    
    // Conversion en chaîne pour debug et logs
    std::string toString() const;
    
private:
    std::shared_ptr<Broker> _broker;
    double _size;
    double _limitPrice;
    double _stopPrice;
    double _slPrice;
    double _tpPrice;
    std::shared_ptr<Trade> _parentTrade;
    std::string _tag;
    double _slPoints;
    double _tpPoints;
    
    friend class Broker;
    friend class Trade;
};