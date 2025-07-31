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
            // QMap<QString, QVariant> generalParams = app->getGeneralParamsPanel()->getValues();
            GeneralParamsConfig generalConfig = app->getGeneralParamsConfig();
            StrategyBaseConfig baseConfig = app->getStrategyBasePanel()->getConfig();
            baseConfig.cash = generalConfig.cash;
            baseConfig.leverage_limit = generalConfig.leverage_limit;
            BuyHeikinGreenConfig specificConfig = 
                dynamic_cast<BuyHeikinGreenPanel*>(app->getStrategySpecificPanel())->getConfig();

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
            // QMap<QString, QVariant> generalParams = app->getGeneralParamsPanel()->getValues();
            GeneralParamsConfig generalParams = app->getGeneralParamsConfig();
            StrategyBaseConfig baseConfig = app->getStrategyBasePanel()->getConfig();
            baseConfig.cash = generalParams.cash;
            baseConfig.leverage_limit = generalParams.leverage_limit;
            // Utiliser le panel spécifique pour récupérer la configuration
            SellHeikinRedConfig specificConfig = 
                dynamic_cast<SellHeikinRedPanel*>(app->getStrategySpecificPanel())->getConfig();

            std::cout << baseConfig << std::endl;
            std::cout << specificConfig << std::endl;
            
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