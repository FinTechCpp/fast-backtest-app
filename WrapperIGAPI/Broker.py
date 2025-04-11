from trading_ig.rest import IGService
from trading_ig.config import config
import pandas as pd


class Broker:
    def __init__(self, epic=None, working_resolution='1Min'):
        self.ig_service = IGService(
        config.username, 
        config.password, 
        config.api_key, 
        config.acc_type,
        acc_number=config.acc_number)

        self.ig_service.create_session(version='3')
        print("Connexion à l'API du broker établie.")

        self.epic = epic
        self.working_resolution = working_resolution
        

    def fetch_historical_prices(self, numpoints=20):
        """
        Récupère les 'count' dernières bougies historiques depuis le broker.
        """

        data = self.ig_service.fetch_historical_prices_by_epic(
            epic=self.epic#,
            # resolution=self.working_resolution,
            # numpoints=numpoints
        )

        df = data['prices'][['bid']].copy()
        df.columns = df.columns.get_level_values(1)
        df.index.name = 'date'
        df = df[['Open', 'High', 'Low', 'Close']]
        df = df.astype(float)

        return df

    def get_live_candle(self):
        """
        Récupère la dernière bougie en live depuis l'API du broker.
        Ici, nous simulons une bougie live.
        """
        candle = {}
        return candle

    def place_order(self, signal):
        """
        Envoie l'ordre à l'API du broker en se basant sur le signal du bot.
        """
        return self.ig_service.create_open_position(
            currency_code='EUR',
            direction=signal['action'],
            epic=self.epic,
            expiry='-',
            force_open=True,
            guaranteed_stop=True,
            level=None,
            limit_distance=signal['take_profit'],
            limit_level=None,
            order_type='MARKET',
            quote_id=None,
            size=signal['quantity'],
            stop_distance=signal['stop_loss'],
            stop_level=None,
            trailing_stop=False,
            trailing_stop_increment=None
        )