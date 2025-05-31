#pragma once

#include "strategies/strategy_factory.h"
#include <QMap>
#include <QString>
#include <memory>
#include <vector>

class StrategyRegistry {
public:
    static StrategyRegistry& getInstance() {
        static StrategyRegistry instance;
        return instance;
    }
    
    // Enregistrer une nouvelle fabrique de stratégie
    void registerFactory(const QString& strategyId, std::shared_ptr<StrategyFactory> factory) {
        m_factories[strategyId] = factory;
    }
    
    // Obtenir une fabrique par ID de stratégie
    std::shared_ptr<StrategyFactory> getFactory(const QString& strategyId) const {
        auto it = m_factories.find(strategyId);
        if (it != m_factories.end()) {
            return it.value();
        }
        return nullptr;
    }
    
    // Obtenir toutes les fabriques enregistrées
    QMap<QString, std::shared_ptr<StrategyFactory>> getAllFactories() const {
        return m_factories;
    }
    
    // Obtenir la liste des noms de stratégies pour l'UI
    QMap<QString, QString> getStrategyMap() const {
        QMap<QString, QString> map;
        for (auto it = m_factories.begin(); it != m_factories.end(); ++it) {
            map[it.key()] = it.value()->getDisplayName();
        }
        return map;
    }

private:
    StrategyRegistry() {} // Constructeur privé (singleton)
    QMap<QString, std::shared_ptr<StrategyFactory>> m_factories;
};