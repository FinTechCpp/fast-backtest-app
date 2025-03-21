from trading_ig.rest import IGService
from trading_ig.config import config


ig_service = IGService(config.username, config.password, config.api_key, config.acc_type)
ig = ig_service.create_session()


# Searching for a market
result = ig_service.search_markets("Tech 100")
# print(result)


# Get info about a market
market  = ig_service.fetch_historical_prices_by_epic("IX.D.NASDAQ.IFE.IP") # US Tech 100 au comptant (1€)
# print(market)


# Getting historic prices
result = ig_service.fetch_historical_prices_by_epic(epic='IX.D.NASDAQ.IFE.IP')
print(result['prices'])