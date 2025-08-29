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
      _slOrder(nullptr),
      _tpOrder(nullptr) {

    _data.size = size;
    _data.entryPrice = entryPrice;
    _data.entryBar = entryBar;
    _data.entryDate = entryDate;
    _data.tag = tag;
}

void Trade::close(double portion) {
    if (portion <= 0 || portion > 1) {
        throw std::invalid_argument("portion must be between 0 and 1");
    }
    
    // Calculer la taille à fermer
    double closeSize = std::copysign(
        std::abs(_data.size) * portion,
        -_data.size  // Signe opposé à la position actuelle
    );
    
    // Créer un ordre pour fermer la position
    // Notez que nous passons this comme trade parent, le broker gèrera l'association
    Order order = _broker->newOrder(closeSize, 0.0, 0.0, 0.0, 0.0, _data.tag, 0.0, 0.0, shared_from_this());
}

bool Trade::setBreakEven(double price, double triggerPrice) {
    // Ne rien faire si déjà en break-even
    if (_data.isBreakEven)
        return true;
    
    // Sauvegarder le SL initial s'il existe
    if (_slOrder) {
        _data.initialSlPrice = _slOrder->stop();
    } else {
        // Aucun stop loss défini, impossible de passer en break-even
        return false;
    }

    // Enregistrer le prix de déclenchement du break-even
    if (triggerPrice > 0) {
        _data.breakEvenTriggerPrice = triggerPrice;
    }
        
    // Vérifier que le nouveau SL est valide (ne déclenche pas immédiatement)
    double currentPrice = _broker->lastPrice();
    if ((isLong() && currentPrice <= price) || 
        (isShort() && currentPrice >= price)) {
        // Le prix actuel déclencherait immédiatement le SL, abandon
        return false;
    }
    
    // Appliquer le nouveau SL
    sl(price);
    
    // Marquer comme break-even
    _data.isBreakEven = true;
    return true;
}

double Trade::pl() const {
    double price = _data.exitPrice > 0 ? _data.exitPrice : _broker->lastPrice();
    return _data.size * (price - _data.entryPrice);
}

double Trade::plPercent() const {
    double price = _data.exitPrice > 0 ? _data.exitPrice : _broker->lastPrice();
    return std::copysign(1.0, _data.size) * (price / _data.entryPrice - 1.0) * 100.f;
}

// double Trade::value() const {
//     double price = _data.exitPrice > 0 ? _data.exitPrice : _broker->lastPrice();
//     return std::abs(_data.size) * price;
// }

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

    // Sauvegarder le prix initial du SL si c'est le premier placement
    if (!_slOrder && _data.initialSlPrice == 0.0) {
        _data.initialSlPrice = price;
    }
    
    // Annuler l'ordre existant s'il y en a un
    if (_slOrder) {
        _broker->cancelOrder(*_slOrder);  // Méthode à ajouter au broker
        _slOrder = nullptr;
    }
    
    // Créer un nouvel ordre SL
    // Comme le broker est ami (friend) de la classe Trade, il peut accéder à _slOrder
    // directement et le modifier
    Order slOrder = _broker->newOrder(-_data.size, 0.0, price, 0.0, 0.0, _data.tag, 0.0, 0.0, shared_from_this());
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
    if ((isLong() && price <= _data.entryPrice) || (isShort() && price >= _data.entryPrice)) {
        throw std::invalid_argument("TP price must be above entry for long trades and below entry for short trades");
    }
    
    // Annuler l'ordre existant s'il y en a un
    if (_tpOrder) {
        _broker->cancelOrder(*_tpOrder);  // Méthode à ajouter au broker
        _tpOrder = nullptr;
    }
    
    // Créer un nouvel ordre TP avec shared_from_this() comme parent
    Order tpOrder = _broker->newOrder(-_data.size, price, 0.0, 0.0, 0.0, _data.tag, 0.0, 0.0, shared_from_this());
    _tpOrder = std::make_shared<Order>(tpOrder);
}

// Méthodes auxiliaires pour que le broker puisse modifier les propriétés du trade
void Trade::setExitPrice(double price) {
    _data.exitPrice = price;
}

void Trade::setExitBar(size_t bar) {
    _data.exitBar = bar;
}

void Trade::setExitDate(Date date)
{
    _data.exitDate = date;
}

void Trade::setSize(double size) {
    _data.size = size;
}

void Trade::setCommissions(double commissions) {
    _data.commissions = commissions;
}

void Trade::setSlOrder(Order order) {
    // Si c'est le premier SL, enregistrer le prix initial
    if (!_slOrder && _data.initialSlPrice == 0.0) {
        _data.initialSlPrice = order.sl();
    }

    _slOrder = std::make_shared<Order>(order);
}

void Trade::setTpOrder(Order order) {
    _tpOrder = std::make_shared<Order>(order);
}

std::string Trade::toString() const {
    std::stringstream ss;
    ss << "<Trade size=" << _data.size << " time=" << _data.entryBar << "-";
    if (_data.exitBar > 0) {
        ss << _data.exitBar;
    }
    ss << " price=" << _data.entryPrice << "-";
    if (_data.exitPrice > 0) {
        ss << _data.exitPrice;
    }
    ss << " pl=" << pl();

    if (!_data.tag.empty()) {
        ss << " tag=" << _data.tag;
    }
    ss << ">";
    return ss.str();
}

} // namespace be
