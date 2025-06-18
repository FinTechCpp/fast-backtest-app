from dataclasses import dataclass
from datetime import datetime
from lightstreamer.client import Subscription
from .objects import nan, StreamObject

class CandleSubscription(Subscription):
    """Represents a subscription for candle data at SECOND resolution"""

    # Utilisation des noms de champs exacts selon la documentation IG
    CANDLE_FIELDS = [
        "BID_OPEN", "BID_HIGH", "BID_LOW", "BID_CLOSE",
        "OFR_OPEN", "OFR_HIGH", "OFR_LOW", "OFR_CLOSE",
        "LTV", "TTV", "UTM", 
        "CONS_END", "CONS_TICK_COUNT"  # Ajout des champs supplémentaires
    ]

    def __init__(self, epic: str, resolution: str = "SECOND"):
        """
        Create a candle subscription
        
        :param epic: The instrument epic code
        :param resolution: The candle resolution (SECOND, 1MINUTE, 5MINUTE, HOUR)
        """
        if resolution not in ["SECOND", "1MINUTE", "5MINUTE", "HOUR"]:
            raise ValueError(f"Invalid resolution: {resolution}")
            
        super().__init__(
            mode="MERGE",
            items=[f"CHART:{epic}:{resolution}"],
            fields=self.CANDLE_FIELDS
        )
        
    def __repr__(self) -> str:
        return f"CandleSubscription with {len(self.item_names)} epics"


@dataclass
class CandleData(StreamObject):
    epic: str
    timestamp: datetime = None
    resolution: str = "SECOND"
    bid_open: float = nan
    bid_high: float = nan
    bid_low: float = nan
    bid_close: float = nan
    ofr_open: float = nan
    ofr_high: float = nan
    ofr_low: float = nan
    ofr_close: float = nan
    volume: int = 0
    incr_volume: int = 0

    def __init__(self, epic, resolution="SECOND"):
        self.epic = epic
        self.resolution = resolution

    def __repr__(self) -> str:
        if hasattr(self, 'timestamp') and self.timestamp:
            ts_str = self.timestamp.strftime('%Y-%m-%d %H:%M:%S')
        else:
            ts_str = "No timestamp"
        return (
            f"{self.epic} {self.resolution} {ts_str} "
            f"O:{self.bid_open:.5f} H:{self.bid_high:.5f} L:{self.bid_low:.5f} C:{self.bid_close:.5f}"
        )

    def populate(self, values):
        self.set_timestamp_by_name("timestamp", values, "UTM")
        # Utiliser les noms de champs avec underscore
        self.set_by_name("bid_open", values, "BID_OPEN", float)
        self.set_by_name("bid_high", values, "BID_HIGH", float)
        self.set_by_name("bid_low", values, "BID_LOW", float)
        self.set_by_name("bid_close", values, "BID_CLOSE", float)
        self.set_by_name("ofr_open", values, "OFR_OPEN", float)
        self.set_by_name("ofr_high", values, "OFR_HIGH", float)
        self.set_by_name("ofr_low", values, "OFR_LOW", float)
        self.set_by_name("ofr_close", values, "OFR_CLOSE", float)
        self.set_by_name("volume", values, "LTV", int)
        self.set_by_name("incr_volume", values, "TTV", int)

    @classmethod
    def identifier(cls, name):
        parts = name.split(":")
        if len(parts) >= 2:
            return parts[1]
        return name