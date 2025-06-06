#include "position.hpp"
#include "broker.hpp"
#include "trade.hpp"
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace be {

Position::Position(std::shared_ptr<Broker> broker) 
    : _broker(broker) {
}

Position::operator bool() const {
    return size() != 0.0;
}

double Position::size() const {
    // Somme des tailles de tous les trades actifs
    //Utilise une réference constante pour éviter de copier les trades ()
    const auto& activeTrades = _broker->trades();
    double totalSize = 0.0;
    for (const auto& trade : activeTrades) {
        totalSize += trade->size();
    }
    return totalSize;
}

double Position::pl() const {
    // Somme des profits/pertes de tous les trades actifs
    double totalPL = 0.0;
    for (const auto& trade : _broker->trades()) {
        totalPL += trade->pl();
    }
    return totalPL;
}

double Position::plPercent() const {
    // Calcul du P/L en pourcentage, basé sur l'investissement total
    double totalInvested = 0.0;
    for (const auto& trade : _broker->trades()) {
        totalInvested += std::abs(trade->size()) * trade->entryPrice();
    }
    
    return totalInvested > 0.0 ? (pl() / totalInvested) * 100.0 : 0.0;
}

bool Position::isLong() const {
    return size() > 0.0;
}

bool Position::isShort() const {
    return size() < 0.0;
}

void Position::close(double portion) {
    // Valider la portion
    if (portion <= 0.0 || portion > 1.0) {
        throw std::invalid_argument("La portion doit être comprise entre 0 et 1");
    }
    
    // Faire une copie pour éviter les problèmes lors de la modification de _trades pendant l'itération
    auto trades = _broker->trades();
    for (const auto& trade : trades) {
        trade->close(portion);
    }
}

} // namespace be