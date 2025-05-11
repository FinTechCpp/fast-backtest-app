from .profile_panel import ProfilePanel
from .general_params_panel import GeneralParamsPanel
from .strategy_base_panel import StrategyBasePanel

# Import des panels spécifiques aux stratégies
from .strategy_specific_panels import (
    BuyHeikinGreenPanel,
    SellHeikinRedPanel,
    CrossEMAPanel,
    # SellTrendFollowingPanel
)


# Dictionnaire pour associer chaque stratégie à son panel correspondant
STRATEGY_PANELS = {
    'BuyHeikinGreenBA': BuyHeikinGreenPanel,
    'SellHeikinRedBA': SellHeikinRedPanel,
    'CrossEMABA': CrossEMAPanel,
}

__all__ = [
    'BasePanel',
    'ProfilePanel', 
    'GeneralParamsPanel'
    'StrategyBasePanel',
    'BuyHeikinGreenPanel',
    'SellHeikinRedPanel',
    'CrossEMAPanel',
    'STRATEGY_PANELS',
    ]