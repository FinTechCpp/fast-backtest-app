#include "components/strategyRegistry.h"
#include "ui/app.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
#include "ui/panels/strategySpecificPanels/buyHeikinGreenPanel.h"
#include "ui/panels/strategySpecificPanels/sellHeikinRedPanel.h"
#include "components/strategiesAdapters/buyHeikinGreen.hpp"
#include "components/strategiesAdapters/sellHeikinRed.hpp"
#include "components/strategiesAdapters/genericStrategyAdapter.hpp"

// Fonction d'initialisation des stratégies
void registerAllStrategies() {
    auto& registry = StrategyRegistry::getInstance();
    
    // Enregistrer BuyHeikinGreen
    registry.registerStrategy(
        "BuyHeikinGreenBA", 
        "Buy Heikin Ashi (Green)",
        [](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, App* app) {
            GeneralParamsConfig generalConfig = app->getGeneralParamsConfig();
            StrategyBaseConfig baseConfig = app->getStrategyBaseConfig();
            baseConfig.cash = generalConfig.cash;
            baseConfig.leverage_limit = generalConfig.leverage_limit;
            BuyHeikinGreenConfig buyConfig = app->getBuyHeikinGreenConfig();

            std::cout << generalConfig << std::endl;
            std::cout << baseConfig << std::endl;
            std::cout << buyConfig << std::endl;

            // Création directe de la stratégie
            return std::make_shared<BuyHeikinGreenAdapter>(broker, data, baseConfig, buyConfig);
        }
    );

    // Enregistrer SellHeikinRed
    registry.registerStrategy(
        "SellHeikinRedBA", 
        "Sell Heikin Ashi (Red)",
        [](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, App* app) {
            GeneralParamsConfig generalParams = app->getGeneralParamsConfig();
            StrategyBaseConfig baseConfig = app->getStrategyBaseConfig();
            baseConfig.cash = generalParams.cash;
            baseConfig.leverage_limit = generalParams.leverage_limit;
            SellHeikinRedConfig sellConfig = app->getSellHeikinRedConfig();

            std::cout << generalParams << std::endl;
            std::cout << baseConfig << std::endl;
            std::cout << sellConfig << std::endl;

            // Création directe de la stratégie
            return std::make_shared<SellHeikinRedAdapter>(broker, data, baseConfig, sellConfig);
        }
    );

    // Enregistrer GenericStrategy
    registry.registerStrategy(
        "GenericStrategy", 
        "Generic Strategy (Configurable)",
        [](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, App* app) {
            GeneralParamsConfig generalParams = app->getGeneralParamsConfig();
            StrategyBaseConfig baseConfig = app->getStrategyBaseConfig();
            baseConfig.cash = generalParams.cash;
            baseConfig.leverage_limit = generalParams.leverage_limit;
            GenericStrategyConfig genericConfig = app->getGenericStrategyConfig();

            std::cout << generalParams << std::endl;
            std::cout << baseConfig << std::endl;
            std::cout << genericConfig << std::endl;

            // Création directe de la stratégie
            return std::make_shared<GenericStrategyAdapter>(broker, data, baseConfig, genericConfig);
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