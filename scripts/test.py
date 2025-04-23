# import talib
# import numpy as np



# close_prices = np.array([100 + i for i in range(20)], dtype='float64')

# ema_period = 5
# ema = talib.EMA(np.array([np.nan]), timeperiod=ema_period)

# print("Close Prices:", close_prices)
# print("EMA:", ema)
# print("EMA Length:", len(ema))
# print("EMA Type:", type(ema))

import talib
import numpy as np
import pandas as pd

# Paramètres
fastk_period = 10
slowk_period = 7
slowd_period = 3

# Générer des données fictives
n = 1
high = np.array([100 + np.sin(i) for i in range(n)], dtype='float64')
low = high - 2
close = (high + low) / 2

# Calculer le stochastique
k, d = talib.STOCH(high, low, close,
                   fastk_period=fastk_period,
                   slowk_period=slowk_period,
                   slowk_matype=0,
                   slowd_period=slowd_period,
                   slowd_matype=0)

# Affichage
df = pd.DataFrame({
    'High': high,
    'Low': low,
    'Close': close,
    f'%K (fastk={fastk_period}, slowk={slowk_period})': k,
    f'%D (slowd={slowd_period})': d
})

# Le nombre de NaN au début de %D est égal à `fastk + slowk + slowd - 3`, car chaque étape de lissage ajoute un délai.

print(df.to_string(index=False))
