import numpy as np
import pandas as pd
import datetime
import os

def generate_synthetic_data(bars=500, initial_price=100.0, volatility=0.015, trend=0.0002):
    """
    Génère des données synthétiques identiques à celles de la version C++
    """
    dates = []
    open_prices = []
    high_prices = []
    low_prices = []
    close_prices = []
    volumes = []
    
    # Utiliser un seed fixe pour la reproductibilité
    np.random.seed(42)
    
    price = initial_price
    end_date = datetime.datetime.now()
    
    for i in range(bars):
        # Générer une date (un jour de moins à chaque barre)
        date = end_date - datetime.timedelta(days=(bars - i - 1))
        dates.append(date.strftime("%Y-%m-%d"))
        
        # Ajouter un bruit aléatoire au prix
        change = np.random.normal(0, volatility) + trend
        price *= (1 + change)
        
        # Générer les valeurs OHLC
        day_open = price * (1 + np.random.normal(0, volatility) * 0.5)
        day_close = price * (1 + np.random.normal(0, volatility) * 0.5)
        day_high = max(day_open, day_close) * (1 + abs(np.random.normal(0, volatility)) * 0.5)
        day_low = min(day_open, day_close) * (1 - abs(np.random.normal(0, volatility)) * 0.5)
        
        # Générer un volume aléatoire
        day_volume = 1000 + abs(np.random.normal(0, volatility)) * 500
        
        open_prices.append(day_open)
        high_prices.append(day_high)
        low_prices.append(day_low)
        close_prices.append(day_close)
        volumes.append(day_volume)
    
    # Créer un DataFrame
    df = pd.DataFrame({
        'Date': dates,
        'Open': open_prices,
        'High': high_prices,
        'Low': low_prices,
        'Close': close_prices,
        'Volume': volumes
    })
    
    return df

if __name__ == "__main__":
    # Générer les données
    data = generate_synthetic_data(bars=500, initial_price=100.0, volatility=0.015, trend=0.0002)
    
    # Créer le dossier de données s'il n'existe pas
    os.makedirs("data", exist_ok=True)
    
    # Sauvegarder les données dans un CSV
    data.to_csv("data/synthetic_data.csv", index=False)
    
    print(f"Données synthétiques générées et sauvegardées dans data/synthetic_data.csv")
    print(f"Nombre de barres: {len(data)}")
    print(f"Premier prix: {data['Open'].iloc[0]:.2f}")
    print(f"Dernier prix: {data['Close'].iloc[-1]:.2f}")