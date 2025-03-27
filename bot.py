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
    
    # Debug: Print the structure of the returned data
    #print("DEBUG: Fetched prices:", prices)
    
    if 'prices' not in prices:
        raise KeyError("'prices' key not found in the API response.")
    
    # Convert the 'prices' data into a DataFrame
    df = pd.DataFrame(prices['prices'])
    
    # Check if the 'bid' column exists and contains the 'Close' sub-column
    if 'bid' not in df.columns or 'Close' not in df['bid']:
        raise KeyError("'Close' sub-column not found in the 'bid' column.")
    
    # Extract the 'Close' prices from the 'bid' column
    df['close'] = df['bid']['Close']
    
    if df['close'].isnull().all():
        raise ValueError("No valid 'Close' prices found in the 'bid' column.")
    
    return df[['close']]

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
        df = fetch_prices(ig_service, EPIC)
        df['EMA_10'] = calculate_ema(df['close'], 10)
        df['EMA_30'] = calculate_ema(df['close'], 30)
        df['RSI'] = calculate_rsi(df['close'])
        
        last_price = df['close'].iloc[-1]
        ema_10 = df['EMA_10'].iloc[-1]
        ema_30 = df['EMA_30'].iloc[-1]
        rsi = df['RSI'].iloc[-1]
        
        print(f"💹 Dernier prix: {last_price}, EMA10: {ema_10}, EMA30: {ema_30}, RSI: {rsi}")
        # Calculate stop and limit distances
        stop_distance = round(last_price * SL, 2)  # Ensure valid precision
        limit_distance = round(last_price * TP, 2)  # Ensure valid precision
        
        if ema_10 > ema_30 and rsi < 70 and (last_open_price is None or abs(last_price - last_open_price) > MIN_PRICE_DIFF):
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
                 
        elif ema_10 < ema_30 and rsi > 30 and (last_open_price is None or abs(last_price - last_open_price) > MIN_PRICE_DIFF):
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
        
        time.sleep(60)  # Rafraîchir toutes les minutes

if __name__ == "__main__":
    main()
