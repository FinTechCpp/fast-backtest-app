#pragma once

#include <memory>
#include <QMap>
#include <QVariant>
#include "broker.hpp"
#include "data.hpp"
#include "strategy.hpp"

class StrategyFactory {
public:
    virtual ~StrategyFactory() = default;
    
    // Méthode pour créer une stratégie à partir des paramètres UI
    virtual std::shared_ptr<be::Strategy> createStrategy(
        std::shared_ptr<be::Broker> broker,
        std::shared_ptr<be::Data> data,
        const QMap<QString, QVariant>& params) const = 0;
        
    // Méthode pour obtenir le nom d'affichage de la stratégie
    virtual QString getDisplayName() const = 0;
    
    // Méthode pour vérifier si la stratégie est applicable aux paramètres donnés
    virtual bool isApplicable(const QMap<QString, QVariant>& params) const = 0;
};