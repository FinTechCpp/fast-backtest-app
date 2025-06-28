"""
markets.py
Ce fichier contient les détails des marchés disponibles pour le trading.
Le dictionnaire epics_dict contient les détails des marchés, y compris le nom du marché,
le type de marché, le pays, la taille de transaction minimale, et les heures d'ouvertures.
"""
from typing import Dict, Any

# Dictionnaire contenant les détails des marchés
epics_dict: Dict[str, Dict[str, Any]] = {
    "CS.D.EURUSD.MINI.IP": {
        "name": "EUR/USD Mini",
        "market": "Forex",
        "country": None,
        "min_trad_size": 0.1,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }        
    },
    "IX.D.NASDAQ.IFE.IP": {
        "name": "NASDAQ Index (1€)",
        "market": "Indices",
        "country": "US",
        "min_trad_size": 0.5,
        "trading_hours": {
            "Monday": [("15:30", "22:00")],
            "Tuesday": [("15:30", "22:00")],
            "Wednesday": [("15:30", "22:00")],
            "Thursday": [("15:30", "22:00")],
            "Friday": [("15:30", "22:00")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "IX.D.SPTRD.IFE.IP": {
        "name": "S&P 500 Index (1€)",
        "market": "Indices",
        "country": "US",
        "min_trad_size": 1,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "IX.D.DAX.IFMM.IP": {
        "name": "DAX Index (1€)",
        "market": "Indices",
        "country": "Germany",
        "min_trad_size": 1,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "IX.D.CAC.IMF.IP": {
        "name": "CAC 40 Index (1€)",
        "market": "Indices",
        "country": "France",
        "min_trad_size": 1,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "CS.D.BITCOIN.CFE.IP": {
        "name": "Bitcoin (CFE)",
        "market": "Cryptocurrencies",
        "country": None,
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "CS.D.XRPUSD.CFD.IP": {
        "name": "Ripple (CFD)",
        "market": "Cryptocurrencies",
        "country": None,
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "UC.D.NVDA.CASH.IP": {
        "name": "NVIDIA (USD)",
        "market": "Stocks",
        "country": "US",
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "UA.D.AAPL.CASH.IP": {
        "name": "Apple (USD)",
        "market": "Stocks",
        "country": "US",
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "UA.D.AMZN.CASH.IP": {
        "name": "Amazon (USD)",
        "market": "Stocks",
        "country": "US",
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "EC.D.HOFP.CASH.IP": {
        "name": "Thales (EUR)",
        "market": "Stocks",
        "country": "France",
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "EC.D.RENA.CASH.IP": {
        "name": "Renault (EUR)",
        "market": "Stocks",
        "country": "France",
        "min_trad_size": None,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    },
    "CS.D.CFEGOLD.CFE.IP": {
        "name": "Gold (1€)",
        "market": "Commodities",
        "country": None,
        "min_trad_size": 1,
        "trading_hours": {
            "Monday": [("00:00", "23:59")],
            "Tuesday": [("00:00", "23:59")],
            "Wednesday": [("00:00", "23:59")],
            "Thursday": [("00:00", "23:59")],
            "Friday": [("00:00", "23:59")],
            "Saturday": [],
            "Sunday": []
        }
    }
}