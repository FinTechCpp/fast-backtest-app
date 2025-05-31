#pragma once

#include "strategies/strategy_factory.h"
#include "BuyHeikinGreen.hpp" // Votre adaptateur de stratégie
#include "panels/strategy_specific_panels/buy_heikin_green_panel.h"

class BuyHeikinGreenFactory : public StrategyFactory {
public:
    std::shared_ptr<be::Strategy> createStrategy(
        std::shared_ptr<be::Broker> broker,
        std::shared_ptr<be::Data> data,
        const QMap<QString, QVariant>& params) const override {
        
        // Convertir les paramètres en configurations typées
        StrategyBaseConfig baseConfig;
        BuyHeikinGreenConfig specificConfig;
        
        // Utiliser les méthodes de conversion (à implémenter ou utiliser convertToConfig)
        baseConfig = convertToStrategyBaseConfig(params);
        specificConfig = convertToBuyHeikinGreenConfig(params);
        
        return std::make_shared<BuyHeikinGreenAdapter>(broker, data, baseConfig, specificConfig);
    }
    
    QString getDisplayName() const override {
        return "BuyHeikinGreen";
    }
    
    bool isApplicable(const QMap<QString, QVariant>& params) const override {
        return params.value("strategy").toString() == "BuyHeikinGreenBA";
    }

private:
    // Méthodes helpers de conversion (peuvent être déplacées ailleurs)
    StrategyBaseConfig convertToStrategyBaseConfig(const QMap<QString, QVariant>& params) const {
        // Implémentation similaire à celle de BasePanel::convertToConfig
    }
    
    BuyHeikinGreenConfig convertToBuyHeikinGreenConfig(const QMap<QString, QVariant>& params) const {
        // Implémentation similaire à celle que vous avez pour BuyHeikinGreenPanel::getConfig
    }
};