#include "components/strategyRegistry.h"
#include "ui/app.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
#include "ui/panels/strategySpecificPanels/buyHeikinGreenPanel.h"
#include "ui/panels/strategySpecificPanels/sellHeikinRedPanel.h"
#include "components/adapters/buyHeikinGreen.hpp"
#include "components/adapters/sellHeikinRed.hpp"

// Fonction d'initialisation des stratégies
void registerAllStrategies() {
    auto& registry = StrategyRegistry::getInstance();
    
    // Enregistrer BuyHeikinGreen
    registry.registerStrategy(
        "BuyHeikinGreenBA", 
        "Buy Heikin Ashi (Green)",
        [](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, App* app) {
            // Récupérer les configurations typées directement depuis les panels
            QMap<QString, QVariant> generalParams = app->getGeneralParamsPanel()->getValues();
            StrategyBaseConfig baseConfig = app->getStrategyBasePanel()->getConfig();
            baseConfig.cash = generalParams.value("cash", -1.0).toDouble();
            baseConfig.leverage_limit = generalParams.value("leverage_limit", -1.0).toDouble();
            BuyHeikinGreenConfig specificConfig = 
                dynamic_cast<BuyHeikinGreenPanel*>(app->getStrategySpecificPanel())->getConfig();


            // plein de std::cout pour debut les paramettres
            std::cout << baseConfig << std::endl;
            std::cout << specificConfig << std::endl;
            
            // Création directe de la stratégie
            return std::make_shared<BuyHeikinGreenAdapter>(broker, data, baseConfig, specificConfig);
        }
    );

    // Enregistrer SellHeikinRed
    registry.registerStrategy(
        "SellHeikinRedBA", 
        "Sell Heikin Ashi (Red)",
        [](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, App* app) {
            // Récupérer les configurations typées directement depuis les panels
            StrategyBaseConfig baseConfig = app->getStrategyBasePanel()->getConfig();
            SellHeikinRedConfig specificConfig = 
                dynamic_cast<SellHeikinRedPanel*>(app->getStrategySpecificPanel())->getConfig();

            // plein de std::cout pour debut les paramettres
            std::cout << baseConfig << std::endl;
            // std::cout << specificConfig << std::endl;

            // Création directe de la stratégie
            return std::make_shared<SellHeikinRedAdapter>(broker, data, baseConfig, specificConfig);
        }
    );

    // Ajouter d'autres stratégies ici...
}

// Auto-enregistrement
namespace {
    class StrategyRegistrar {
    public:
        StrategyRegistrar() {
            registerAllStrategies();
        }
    };
    
    static StrategyRegistrar registrar;
}