#include "strategies/strategy_registry.h"
#include "app.h"
#include "panels/strategy_specific_panels/strategy_base_panel.h"  // Ajoutez cette ligne
#include "panels/strategy_specific_panels/buy_heikin_green_panel.h"
#include "panels/strategy_specific_panels/sell_heikin_red_panel.h"
#include "BuyHeikinGreen.hpp"
#include "SellHeikinRed.hpp"

// Fonction d'initialisation des stratégies
void registerAllStrategies() {
    auto& registry = StrategyRegistry::getInstance();
    
    // Enregistrer BuyHeikinGreen
    registry.registerStrategy(
        "BuyHeikinGreenBA", 
        "Buy Heikin Ashi (Green)",
        [](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data, App* app) {
            // Récupérer les configurations typées directement depuis les panels
            StrategyBaseConfig baseConfig = app->getStrategyBasePanel()->getConfig();
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