from .profile_panel import ProfilePanel
from .general_params_panel import GeneralParamsPanel
from .strategy_base_panel import StrategyBasePanel

# Import des panels spécifiques aux stratégies
from .strategy_specific_panels import (
    BuyHeikinGreenPanel,
    CrossEMAPanel,
    # SellTrendFollowingPanel
)


# Dictionnaire pour associer chaque stratégie à son panel correspondant
STRATEGY_PANELS = {
    'BuyHeikinGreenBA': BuyHeikinGreenPanel,
    'CrossEMABA': CrossEMAPanel,
}

__all__ = [
    'BasePanel',
    'ProfilePanel', 
    'GeneralParamsPanel'
    'StrategyBasePanel',
    'BuyHeikinGreenPanel',
    'CrossEMAPanel',
    'STRATEGY_PANELS',
    ]