from trading_ig.rest import IGService
from trading_ig.config import config
import time



ig_service = IGService(
    config.username, 
    config.password, 
    config.api_key, 
    config.acc_type,
    acc_number=config.acc_number)
ig = ig_service.create_session(version='3')


# Searching for a market
result = ig_service.search_markets("EUR/USD")
print(result)


# Get info about a market
# IX.D.NASDAQ.IFE.IP  <=> US Tech 100 au comptant (1€)
# CS.D.EURUSD.MINI.IP <=> EUR/USD Mini
# market  = ig_service.fetch_historical_prices_by_epic("CS.D.EURUSD.MINI.IP")
# print(market)


# Getting historic prices
# result = ig_service.fetch_historical_prices_by_epic(epic='IX.D.NASDAQ.IFE.IP')
# print(result['prices'])


# EPIC = 'IX.D.NASDAQ.IFE.IP'

# print("🔄 Récupération du prix EUR/USD en temps réel...")

# for i in range(100):
#     result = ig_service.fetch_market_by_epic(EPIC)
    
#     if "snapshot" in result:
#         bid = result["snapshot"]["bid"]  # Prix acheteur
#         ask = result["snapshot"]["offer"]  # Prix vendeur
#         print(f"⏱️ {i+1}/10 - Bid: {bid}, Ask: {ask}")
#     else:
#         print("⚠️ Impossible de récupérer les données.")

#     time.sleep(10)  # Pause d'une seconde

# print("✅ Fin de la récupération des prix.")

resp = ig_service.create_open_position(
    currency_code='EUR',
    direction='BUY',
    epic='IX.D.NASDAQ.IFE.IP',
    order_type='MARKET',
    expiry='DFB',
    force_open='false',
    guaranteed_stop='false',
    size=1, 
    level=None,
    limit_distance=None,
    limit_level=None,
    quote_id=None,
    stop_level=None,
    stop_distance=None,
    trailing_stop=None,
    trailing_stop_increment=None)
print(resp)