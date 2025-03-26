import time
import pandas as pd
import numpy as np
from trading_ig.rest import IGService
from trading_ig.config import config
import utils
import markets

def calculate_ema(data, period):
    return data.ewm(span=period, adjust=False).mean()

def calculate_rsi(data, period=14):
    delta = data.diff()
    gain = (delta.where(delta > 0, 0)).rolling(window=period).mean()
    loss = (-delta.where(delta < 0, 0)).rolling(window=period).mean()
    rs = gain / loss
    return 100 - (100 / (1 + rs))

def fetch_prices(ig_service, epic, resolution='1Min', num_points=50):
    prices = ig_service.fetch_historical_prices_by_epic(epic, resolution=resolution, numpoints=num_points)
    df = pd.DataFrame(prices['prices'])
    df['close'] = df['closePrice'].apply(lambda x: x['bid'])  # Prendre le prix "bid"
    return df[['close']]

def main():
    ig_service = utils.initialize_service()
    
    EPIC, SELECTED_DATA = utils.select_from_dict(markets.epics_dict)
    TRADE_SIZE = 1  # Taille de la position
    SL = 0.002  # Stop Loss
    TP = 0.004  # Take Profit
    
    print("🔄 Démarrage du bot de trading...")
    
    while True:
        df = fetch_prices(ig_service, EPIC)
        df['EMA_10'] = calculate_ema(df['close'], 10)
        df['EMA_30'] = calculate_ema(df['close'], 30)
        df['RSI'] = calculate_rsi(df['close'])
        
        last_price = df['close'].iloc[-1]
        ema_10 = df['EMA_10'].iloc[-1]
        ema_30 = df['EMA_30'].iloc[-1]
        rsi = df['RSI'].iloc[-1]
        
        print(f"💹 Dernier prix: {last_price}, EMA10: {ema_10}, EMA30: {ema_30}, RSI: {rsi}")
        
        if ema_10 > ema_30 and rsi < 70:
            print("📈 Signal d'achat détecté!")
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
                stop_distance=SL, #Distance du stop loss    
                limit_distance=TP) #Distance du take profit
            utils.postion_output(response)
                 
        elif ema_10 < ema_30 and rsi > 30:
            print("📉 Signal de vente détecté!")
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
                stop_distance=SL,
                limit_distance=TP)
            utils.postion_output(response)
        
        time.sleep(60)  # Rafraîchir toutes les minutes

if __name__ == "__main__":
    main()
