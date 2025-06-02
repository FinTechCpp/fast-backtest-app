#include "strategy.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "position.hpp"
#include "trade.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <memory>

namespace be {

Strategy::Strategy(std::shared_ptr<Broker> broker, std::shared_ptr<Data> data)
    : _broker(broker), _data(data) {
    if (!broker) {
        throw std::invalid_argument("Broker cannot be null");
    }
    if (!data) {
        throw std::invalid_argument("Data cannot be null");
    }
}

Order Strategy::buy(double size, 
                   double limit, 
                   double stop, 
                   double sl, 
                   double tp,
                   double sl_points,
                   double tp_points,
                   const std::string& tag) {
    // Validation des paramètres
    if (size <= 0) {
        throw std::invalid_argument("Size must be positive for buy orders");
    }
    
    // Vérifier qu'on ne spécifie pas à la fois sl et sl_points
    if (sl != 0.0 && sl_points != 0.0) {
        throw std::invalid_argument("Cannot specify both sl and sl_points");
    }
    
    // Vérifier qu'on ne spécifie pas à la fois tp et tp_points
    if (tp != 0.0 && tp_points != 0.0) {
        throw std::invalid_argument("Cannot specify both tp and tp_points");
    }
    
    // Créer un ordre via le broker
    return _broker->newOrder(size, limit, stop, sl, tp, tag, sl_points, tp_points);
}

Order Strategy::sell(double size, 
                     double limit, 
                     double stop, 
                     double sl, 
                     double tp,
                     double sl_points,
                     double tp_points,
                     const std::string& tag) {
    // Validation des paramètres
    if (size <= 0) {
        throw std::invalid_argument("Size must be positive for sell orders");
    }
    
    // Vérifier qu'on ne spécifie pas à la fois sl et sl_points
    if (sl != 0.0 && sl_points != 0.0) {
        throw std::invalid_argument("Cannot specify both sl and sl_points");
    }
    
    // Vérifier qu'on ne spécifie pas à la fois tp et tp_points
    if (tp != 0.0 && tp_points != 0.0) {
        throw std::invalid_argument("Cannot specify both tp and tp_points");
    }
    
    // Créer un ordre de vente (taille négative) via le broker
    return _broker->newOrder(-size, limit, stop, sl, tp, tag, sl_points, tp_points);
}

// Méthodes d'accès aux propriétés

double Strategy::getEquity() const {
    return _broker->equity();
}

std::shared_ptr<Data> Strategy::getData() const {
    return _data;
}

Position Strategy::getPosition() const {
    return _broker->position();
}

std::vector<Order> Strategy::getOrders() const {
    return _broker->orders();
}

std::vector<Trade> Strategy::getTrades() const {
    // Le broker retourne des shared_ptr<Trade>, nous devons les convertir en Trade
    std::vector<Trade> trades;
    for (const auto& trade_ptr : _broker->trades()) {
        trades.push_back(*trade_ptr);
    }
    return trades;
}

std::vector<Trade> Strategy::getClosedTrades() const {
    // Le broker retourne des shared_ptr<Trade>, nous devons les convertir en Trade
    std::vector<Trade> closedTrades;
    
    // Pré-allouer la mémoire pour éviter les réallocations
    closedTrades.reserve(_broker->closedTrades().size());
    
    for (const auto& trade_ptr : _broker->closedTrades()) {
        closedTrades.push_back(*trade_ptr);
    }
    return closedTrades;
}

} // namespace be