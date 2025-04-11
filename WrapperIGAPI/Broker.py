from trading_ig.rest import IGService
from trading_ig.config import config
import pandas as pd
import datetime

EXPIRY = '-'
CURRENCY = 'EUR'
ORDER_TYPE = 'MARKET'

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
            epic=self.epic,
            resolution=self.working_resolution,
            numpoints=numpoints
        )

        df = data['prices'][['bid']].copy()
        df.columns = df.columns.get_level_values(1)
        df.index.name = 'date'
        df = df[['Open', 'High', 'Low', 'Close']]
        df = df.astype(float)

        return df

    def fetch_previous_and_current_candles(self):
        """
        Récupère la dernière bougie en live depuis l'API du broker.
        """
        results = self.ig_service.fetch_historical_prices_by_epic(
            epic=self.epic,
            resolution=self.working_resolution,
            numpoints=2)
        
        data = results['prices'][['bid']].copy()
        data.columns = data.columns.get_level_values(1)

        candle_previous = {
            'date': data.index[-2],
            'Open': data['Open'].iloc[-2],
            'High': data['High'].iloc[-2],
            'Low': data['Low'].iloc[-2],
            'Close': data['Close'].iloc[-2]
        }
        
        candle_current = {
            'date': data.index[-1],
            'Open': data['Open'].iloc[-1],
            'High': data['High'].iloc[-1],
            'Low': data['Low'].iloc[-1],
            'Close': data['Close'].iloc[-1]
        }

        return candle_previous, candle_current

    def execute_signal(self, signal):
        """
        Exécute le signal de trading (achat, vente ou liquidation) sur l'API du broker.
        """
        if signal is None:
            return

        # On ferme la position
        if signal['action'] == 'LIQUIDATE':
            return self.close_open_position()

        # On place l'ordre
        return self.place_order(signal)


    def close_open_position(self):
        """
        Ferme toutes les positions ouvertes sur l'EPIC spécifié.
        """

        # On recupère toutes les positions ouvertes
        open_positions = self.ig_service.fetch_open_positions()
        positions_to_close = open_positions[open_positions['epic'] == self.epic]

        if positions_to_close.empty:
            return

        # Pour chaque position, on appelle la méthode close_open_position de l'API IG
        results = []
        for idx, pos in positions_to_close.iterrows():
            deal_id = pos['dealId']
            if pos['direction'] == 'BUY':
                direction  = 'SELL'
            elif pos['direction'] == 'SELL':
                direction  = 'BUY'
            epic       = None
            expiry     = '-'  
            level      = None
            order_type = 'MARKET'
            quote_id   = None
            size       = pos['size']
            
            try:
                result = self.ig_service.close_open_position(
                    deal_id=deal_id,
                    direction=direction,
                    epic=epic,
                    expiry=expiry,
                    level=level,
                    order_type=order_type,
                    quote_id=quote_id,
                    size=size,
                    # Optionnel : vous pouvez ajouter time_in_force si nécessaire,
                    # et éventuellement gérer le paramètre "session"
                )
                print(f"Position fermée (dealId: {deal_id}) :", result)
                results.append(result)
            except Exception as e:
                print(f"Erreur lors de la fermeture de la position (dealId: {deal_id}) :", e)
                
        return results

    def place_order(self, signal):
        """
        Envoie l'ordre à l'API du broker en se basant sur le signal du bot.
        """

        if signal['action'] == 'LIQUIDATE':
            return self.close_open_position()


        tp_distance = abs(signal['take_profit']) if signal.get('take_profit') is not None else None
        sl_distance = abs(signal['stop_loss']) if signal.get('stop_loss') is not None else None

        # Si on n'a pas de distance de stop loss, on ne peut pas garantir l'arrêt
        guaranteed_stop = not sl_distance is None

        return self.ig_service.create_open_position(
            currency_code=CURRENCY,
            direction=signal['action'],
            epic=self.epic,
            expiry=EXPIRY,
            force_open=True,
            guaranteed_stop=guaranteed_stop,
            level=None,
            limit_distance=tp_distance,
            limit_level=None,
            order_type=ORDER_TYPE,
            quote_id=None,
            size=signal['quantity'],
            stop_distance=sl_distance,
            stop_level=None,
            trailing_stop=False,
            trailing_stop_increment=None
        )