#include "order.hpp"
#include "broker.hpp"
#include "trade.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace be {

Order::Order(std::shared_ptr<Broker> broker,
             double size,
             double limitPrice,
             double stopPrice,
             double slPrice,
             double tpPrice,
             std::shared_ptr<Trade> parentTrade,
             const std::string& tag,
             double slPoints,
             double tpPoints)
    : _broker(broker),
      _size(size),
      _limitPrice(limitPrice),
      _stopPrice(stopPrice),
      _slPrice(slPrice),
      _tpPrice(tpPrice),
      _parentTrade(parentTrade),
      _tag(tag),
      _slPoints(slPoints),
      _tpPoints(tpPoints) {
    // Vérifier que la taille n'est pas 0
    if (size == 0.0) {
        throw std::invalid_argument("Order size cannot be zero");
    }
}

bool Order::isContingent() const {
    if (!_parentTrade) {
        return false;
    }
    
    // Vérifier si cet ordre est un ordre SL ou TP du trade parent
    // Note: cette implémentation simplifiée sera à affiner lors de
    // l'implémentation complète de Trade
    return (_stopPrice > 0 && _parentTrade->sl() == _stopPrice) || 
           (_limitPrice > 0 && _parentTrade->tp() == _limitPrice);
}

// Méthode helper pour remplacer certains attributs de l'ordre
// Nécessaire pour le bon fonctionnement de broker.next()
void Order::_replace(const std::string& attr, double value) {
    if (attr == "stopPrice") {
        _stopPrice = value;
    } else if (attr == "limitPrice") {
        _limitPrice = value;
    } else if (attr == "size") {
        _size = value;
    } else if (attr == "slPrice") {
        _slPrice = value;
    } else if (attr == "tpPrice") {
        _tpPrice = value;
    } else {
        throw std::invalid_argument("Invalid attribute name in Order::_replace");
    }
}

// Surcharge pour remplacer plusieurs attributs simultanément
void Order::_replace(std::map<std::string, double> changes) {
    for (const auto& [attr, value] : changes) {
        _replace(attr, value);
    }
}

bool Order::operator==(const Order& other) const {
    // Deux ordres sont considérés égaux s'ils ont les mêmes propriétés essentielles
    return (_size == other._size &&
            _limitPrice == other._limitPrice &&
            _stopPrice == other._stopPrice &&
            _slPrice == other._slPrice &&
            _tpPrice == other._tpPrice &&
            _parentTrade == other._parentTrade &&
            _tag == other._tag);
}

std::string Order::toString() const {
    std::stringstream ss;
    ss << "<Order ";
    
    bool first = true;
    auto appendIfNotZero = [&](const std::string& name, double value, bool force = false) {
        if (value != 0.0 || force) {
            if (!first) ss << ", ";
            ss << name << "=" << std::fixed << std::setprecision(5) << value;
            first = false;
        }
    };

    appendIfNotZero("size", _size, true);
    appendIfNotZero("limit", _limitPrice);
    appendIfNotZero("stop", _stopPrice);
    appendIfNotZero("sl", _slPrice);
    appendIfNotZero("tp", _tpPrice);
    
    if (!first) ss << ", ";
    ss << "contingent=" << (isContingent() ? "true" : "false");
    first = false;
    
    if (!_tag.empty()) {
        ss << ", tag=" << _tag;
    }
    
    ss << ">";
    return ss.str();
}

} // namespace be