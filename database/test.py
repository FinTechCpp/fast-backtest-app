from operations import insert_market_data, get_data

btc_data = [
    ("2023-10-01 00:00:00", 50000, 51000, 49000, 50500, 1000),
    ("2023-10-02 00:00:00", 50500, 51500, 49500, 51000, 1200),
    ("2023-10-03 00:00:00", 51000, 52000, 50000, 51500, 1100),
    # Ajoutez d'autres données ici
]

insert_market_data("BTC", btc_data)

print(get_data("BTC", "2023-10-01 00:00:00", "2023-10-03 00:00:00"))