#include "trade.hpp"
#include "broker.hpp"
#include "order.hpp"
#include <cmath>
#include <stdexcept>
#include <limits>
#include <iostream>
#include <sstream>

namespace be {

Trade::Trade(std::shared_ptr<Broker> broker,
             double size,
             double entryPrice,
             size_t entryBar,
             const Date& entryDate,
             const std::string& tag)
    : _broker(broker),
      _size(size),
      _entryPrice(entryPrice),
      _exitPrice(0.0),  // Pas encore fermé
      _entryBar(entryBar),
      _exitBar(0),      // Pas encore fermé
      _entryDate(entryDate),
      _exitDate(Date()), // Date de sortie (initialisée à une date par défaut)
      _tag(tag),
      _commissions(0.0),
      _slOrder(nullptr),
      _tpOrder(nullptr) {
}

void Trade::close(double portion) {
    if (portion <= 0 || portion > 1) {
        throw std::invalid_argument("portion must be between 0 and 1");
    }
    
    // Calculer la taille à fermer
    double closeSize = std::copysign(
        std::abs(_size) * portion,
        -_size  // Signe opposé à la position actuelle
    );
    
    // Créer un ordre pour fermer la position
    // Notez que nous passons this comme trade parent, le broker gèrera l'association
    Order order = _broker->newOrder(closeSize, 0.0, 0.0, 0.0, 0.0, _tag, 0.0, 0.0, shared_from_this());
}

bool Trade::setBreakEven(double offset) {
    // Ne rien faire si déjà en break-even
    if (_isBreakEven)
        return true;
    
    // Sauvegarder le SL initial s'il existe
    if (_slOrder) {
        _initialSlPrice = _slOrder->stop();
    } else {
        // Aucun stop loss défini, impossible de passer en break-even
        return false;
    }
    
    // Calculer le nouveau prix du SL (prix d'entrée avec offset éventuel)
    double newSlPrice = _entryPrice;
    
    // Si un offset est fourni, l'appliquer dans la direction appropriée
    if (offset != 0.0) {
        // Pour un long: prix d'entrée + offset positif = légère sécurité
        // Pour un short: prix d'entrée - offset positif = légère sécurité
        newSlPrice += isLong() ? offset : -offset;
    }
    
    // Vérifier que le nouveau SL est valide (ne déclenche pas immédiatement)
    double currentPrice = _broker->lastPrice();
    if ((isLong() && currentPrice <= newSlPrice) || 
        (isShort() && currentPrice >= newSlPrice)) {
        // Le prix actuel déclencherait immédiatement le SL, abandon
        return false;
    }
    
    // Appliquer le nouveau SL
    sl(newSlPrice);
    
    // Marquer comme break-even
    _isBreakEven = true;
    return true;
}

double Trade::pl() const {
    double price = _exitPrice > 0 ? _exitPrice : _broker->lastPrice();
    return _size * (price - _entryPrice);
}

double Trade::plPercent() const {
    double price = _exitPrice > 0 ? _exitPrice : _broker->lastPrice();
    return std::copysign(1.0, _size) * (price / _entryPrice - 1.0);
}

double Trade::value() const {
    double price = _exitPrice > 0 ? _exitPrice : _broker->lastPrice();
    return std::abs(_size) * price;
}

double Trade::sl() const {
    return _slOrder ? _slOrder->stop() : 0.0;
}

void Trade::sl(double price) {
    // Si price est 0, on annule l'ordre SL existant
    if (price == 0.0) {
        if (_slOrder) {
            _broker->cancelOrder(*_slOrder);
            _slOrder = nullptr;
        }
        return;
    }
    
    // Vérifier la validité du prix
    if (!(0.0 < price && price < std::numeric_limits<double>::infinity())) {
        throw std::invalid_argument("Make sure 0 < price < inf");
    }
    
    // Vérifier que le prix est cohérent avec la direction du trade
    // if ((isLong() && price >= _entryPrice) || (isShort() && price <= _entryPrice)) {
    //     throw std::invalid_argument("SL price must be below entry for long trades and above entry for short trades");
    // }
    
    // Annuler l'ordre existant s'il y en a un
    if (_slOrder) {
        _broker->cancelOrder(*_slOrder);  // Méthode à ajouter au broker
        _slOrder = nullptr;
    }
    
    // Créer un nouvel ordre SL
    // Comme le broker est ami (friend) de la classe Trade, il peut accéder à _slOrder
    // directement et le modifier
    Order slOrder = _broker->newOrder(-_size, 0.0, price, 0.0, 0.0, _tag, 0.0, 0.0, shared_from_this());
    _slOrder = std::make_shared<Order>(slOrder);
}

double Trade::tp() const {
    return _tpOrder ? _tpOrder->limit() : 0.0;
}

void Trade::tp(double price) {
    // Si price est 0, on annule l'ordre TP existant
    if (price == 0.0) {
        if (_tpOrder) {
            // Au lieu de cancel(), on annule directement l'association
            _broker->cancelOrder(*_tpOrder);  // Méthode à ajouter au broker
            _tpOrder = nullptr;
        }
        return;
    }
    
    // Vérifier la validité du prix
    if (!(0.0 < price && price < std::numeric_limits<double>::infinity())) {
        throw std::invalid_argument("Make sure 0 < price < inf");
    }
    
    // Vérifier que le prix est cohérent avec la direction du trade
    if ((isLong() && price <= _entryPrice) || (isShort() && price >= _entryPrice)) {
        throw std::invalid_argument("TP price must be above entry for long trades and below entry for short trades");
    }
    
    // Annuler l'ordre existant s'il y en a un
    if (_tpOrder) {
        _broker->cancelOrder(*_tpOrder);  // Méthode à ajouter au broker
        _tpOrder = nullptr;
    }
    
    // Créer un nouvel ordre TP avec shared_from_this() comme parent
    Order tpOrder = _broker->newOrder(-_size, price, 0.0, 0.0, 0.0, _tag, 0.0, 0.0, shared_from_this());
    _tpOrder = std::make_shared<Order>(tpOrder);
}

// Méthodes auxiliaires pour que le broker puisse modifier les propriétés du trade
void Trade::setExitPrice(double price) {
    _exitPrice = price;
}

void Trade::setExitBar(size_t bar) {
    _exitBar = bar;
}

void Trade::setExitDate(Date date)
{
    _exitDate = date;
}

void Trade::setSize(double size) {
    _size = size;
}

void Trade::setCommissions(double commissions) {
    _commissions = commissions;
}

void Trade::setSlOrder(Order order) {
    _slOrder = std::make_shared<Order>(order);
}

void Trade::setTpOrder(Order order) {
    _tpOrder = std::make_shared<Order>(order);
}

std::string Trade::toString() const {
    std::stringstream ss;
    ss << "<Trade size=" << _size << " time=" << _entryBar << "-";
    if (_exitBar > 0) {
        ss << _exitBar;
    }
    ss << " price=" << _entryPrice << "-";
    if (_exitPrice > 0) {
        ss << _exitPrice;
    }
    ss << " pl=" << pl();
    
    if (!_tag.empty()) {
        ss << " tag=" << _tag;
    }
    ss << ">";
    return ss.str();
}

} // namespace be
