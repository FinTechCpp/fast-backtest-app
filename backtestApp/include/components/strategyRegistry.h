#pragma once

#include <functional>
#include <memory>
#include <QMap>
#include <QString>

#include "broker.hpp"
#include "data.hpp"
#include "strategy.hpp"
#include "ui/app.h"

// Définition du type pour la fonction de création de stratégie
using StrategyCreator = std::function<std::shared_ptr<be::Strategy>(
    std::shared_ptr<be::Broker> broker,
    std::shared_ptr<be::Data> data,
    App* app)>;

class StrategyRegistry {
public:
    static StrategyRegistry& getInstance() {
        static StrategyRegistry instance;
        return instance;
    }
    
    // Enregistrer une fonction de création
    void registerStrategy(const QString& strategyId, 
                         const QString& displayName,
                         StrategyCreator creator) {
        m_creators[strategyId] = creator;
        m_displayNames[strategyId] = displayName;
    }
    
    // Obtenir une fonction de création par ID de stratégie
    StrategyCreator getCreator(const QString& strategyId) const {
        auto it = m_creators.find(strategyId);
        if (it != m_creators.end()) {
            return it.value();
        }
        return nullptr;
    }
    
    // Obtenir la carte des noms de stratégies pour l'UI
    QMap<QString, QString> getStrategyMap() const {
        return m_displayNames;
    }

private:
    StrategyRegistry() {} // Constructeur privé (singleton)
    QMap<QString, StrategyCreator> m_creators;
    QMap<QString, QString> m_displayNames;
};