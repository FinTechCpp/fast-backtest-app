import time
import pandas as pd
import numpy as np
from trading_ig.rest import IGService
from trading_ig.config import config
import scripts.utils as utils
import scripts.markets as markets

def main():
    ig_service = utils.initialize_service()
    
    # Paramètres de trading
    EPIC, SELECTED_DATA = utils.select_from_dict(markets.epics_dict)
    TRADE_SIZE = 0.5  # Taille de la position
    SL = 0.002  # Stop Loss
    TP = 0.004  # Take Profit
    last_open_price = None
    MIN_PRICE_DIFF = 5 # Différence minimale de prix pour éviter les faux signaux
    
    print("🔄 Démarrage du bot de trading...")
    print(f"📊 Analyse des données pour l'EPIC: {EPIC}...")
    
    while True:
        df = utils.fetch_prices(ig_service, EPIC)
        df['EMA_10'] = utils.calculate_ema(df['close'], 10)
        df['EMA_30'] = utils.calculate_ema(df['close'], 30)
        df['RSI'] = utils.calculate_rsi(df['close'])
        df['ATR'] = utils.calculate_atr(df, period=14)
        
        last_price = df['close'].iloc[-1]
        ema_10 = df['EMA_10'].iloc[-1]
        ema_30 = df['EMA_30'].iloc[-1]
        rsi = df['RSI'].iloc[-1]
        atr = df['ATR'].iloc[-1]
        
        MIN_PRICE_DIFF = atr * 2  # Dynamically adjust min price diff based on ATR
        
        print(f"💹 Dernier prix: {last_price}, EMA10: {ema_10}, EMA30: {ema_30}, RSI: {rsi}")
        print(f"MIN_PRICE_DIFF dynamique: {MIN_PRICE_DIFF}")
        # Calculate stop and limit distances
        stop_distance = round(last_price * SL, 2)  # Ensure valid precision
        limit_distance = round(last_price * TP, 2)  # Ensure valid precision
        
        if utils.should_open_position('BUY', ema_10, ema_30, rsi, last_price, last_open_price, MIN_PRICE_DIFF):
            print("\n------------------------------------")
            print("📈 Signal d'achat détecté!")
            print("------------------------------------")
            response = ig_service.create_open_position(
                epic=EPIC, #epic sélectionné
                direction='BUY', #ACHAT
                size=TRADE_SIZE, #Quantité de la position
                currency_code='EUR', #Devise
                expiry='-', #Expiration
                order_type='MARKET', #Achat au prix du marché ou prix limite
                guaranteed_stop=False, #Stop garanti
                force_open=True, #Ouverture forcée
                trailing_stop=False, #Stop suiveur
                time_in_force='FILL_OR_KILL', #Durée de vie de l'ordre
                stop_distance=stop_distance, #Distance du stop loss    
                limit_distance=limit_distance,
                level=None,
                limit_level=None,
                quote_id=None,
                stop_level=None,
                trailing_stop_increment=None) #Distance du take profit
            utils.position_output(response)
            last_open_price = last_price
        elif utils.should_open_position('SELL', ema_10, ema_30, rsi, last_price, last_open_price, MIN_PRICE_DIFF):
            print("\n------------------------------------")
            print("📉 Signal de vente détecté!")
            print("------------------------------------")
            response = ig_service.create_open_position(
                epic=EPIC,
                direction='SELL',
                size=TRADE_SIZE,
                currency_code='EUR',
                expiry='-',
                order_type='MARKET',
                guaranteed_stop=False,
                force_open=True,
                trailing_stop=False,
                time_in_force='FILL_OR_KILL',
                stop_distance=stop_distance,
                limit_distance=limit_distance,
                level=None,
                limit_level=None,
                quote_id=None,
                stop_level=None,
                trailing_stop_increment=None)
            utils.position_output(response)
            last_open_price = last_price
        time.sleep(60)  # Rafraîchir toutes les minutes

if __name__ == "__main__":
    main()
