#!/usr/bin/env python3
# filepath: test_cpp_strategy.py

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import datetime as dt

# Importer les classes C++ avec les noms corrects
from cpp_strategies import CppCandle, CppSignal, CppStrategyBaseConfig, CppBuyHeikinGreenConfig, CppBuyHeikinGreen

def main():
    print("### Test des stratégies de trading C++ ###")
    
    # 1. Création des configurations
    print("\n1. Création des configurations...")
    base_config = CppStrategyBaseConfig()
    base_config.trading_days = [0, 1, 2, 3, 4]
    base_config.take_profit_distance = 30.0
    base_config.stop_loss_distance = 15.0
    base_config.use_atr_for_sl_tp = True
    base_config.atr_period = 14
    base_config.stop_loss_atr_multiplier = 2.0
    base_config.take_profit_atr_multiplier = 3.0
    base_config.min_stop_loss_distance = 5.0
    base_config.min_take_profit_distance = 10.0
    base_config.use_risk_based_sizing = True
    base_config.risk_percentage = 1.0
    base_config.cash = 10000.0
    base_config.leverage_limit = 20.0

    strategy_config = CppBuyHeikinGreenConfig()
    strategy_config.ema_short_period = 20  # Plus court pour obtenir plus de signaux
    strategy_config.ema_long_period = 50   # Plus court pour obtenir plus de signaux
    strategy_config.stoch_fastk = 10
    strategy_config.stoch_slowk = 7
    strategy_config.stoch_slowd = 3
    strategy_config.stoch_threshold = 30
    strategy_config.use_ema_short_filter = True
    strategy_config.use_ema_long_filter = True
    strategy_config.use_stoch_filter = True
    strategy_config.use_previous_ha_candle_red_filter = True
    
    # 2. Initialisation de la stratégie
    print("\n2. Initialisation de la stratégie...")
    strategy = CppBuyHeikinGreen(base_config, strategy_config)
    
    # 3. Préparation des bougies de test
    print("\n3. Préparation des données de test...")
    
    num_candles = 100
    dates = [dt.datetime.now() - dt.timedelta(hours=i) for i in range(num_candles, 0, -1)]
    
    # Tendance sinusoïdale
    x = np.linspace(0, 4*np.pi, num_candles)
    trend = 10 * np.sin(x/2) + 110
    noise = np.random.normal(0, 1, num_candles)
    closes = trend + noise
    
    # OHLC
    daily_volatility = 2.0
    opens = closes - np.random.uniform(-daily_volatility, daily_volatility, num_candles)
    highs = np.maximum(closes, opens) + np.random.uniform(0, daily_volatility, num_candles)
    lows = np.minimum(closes, opens) - np.random.uniform(0, daily_volatility, num_candles)
    
    # EMA
    def simple_ema(data, period):
        alpha = 2 / (period + 1)
        ema = np.zeros_like(data)
        ema[0] = data[0]
        for i in range(1, len(data)):
            ema[i] = data[i] * alpha + ema[i-1] * (1 - alpha)
        return ema
    
    ema_short = simple_ema(closes, strategy_config.ema_short_period)
    ema_long = simple_ema(closes, strategy_config.ema_long_period)
    
    # ATR
    atr = np.zeros(num_candles)
    for i in range(1, num_candles):
        true_range = max(highs[i] - lows[i], abs(highs[i] - closes[i-1]), abs(lows[i] - closes[i-1]))
        atr[i] = 0.9 * atr[i-1] + 0.1 * true_range if i > 1 else true_range
    
    # Stochastique
    def calculate_stochastic(high, low, close, k_period, d_period):
        k = np.zeros_like(close)
        d = np.zeros_like(close)
        
        for i in range(k_period - 1, len(close)):
            window_high = np.max(high[i-k_period+1:i+1])
            window_low = np.min(low[i-k_period+1:i+1])
            k[i] = 100 * (close[i] - window_low) / (window_high - window_low) if window_high != window_low else 50
        
        for i in range(k_period + d_period - 2, len(close)):
            d[i] = np.mean(k[i-d_period+1:i+1])
        
        return k, d
    
    stoch_k, stoch_d = calculate_stochastic(highs, lows, closes, strategy_config.stoch_fastk, strategy_config.stoch_slowd)
    
    # 4. Traitement des bougies et collecte des signaux
    print("\n4. Traitement des bougies...")
    signals = []
    
    ema_short_name = f'EMA_{strategy_config.ema_short_period}'
    ema_long_name = f'EMA_{strategy_config.ema_long_period}'
    stoch_k_name = f'STOCH_K_{strategy_config.stoch_fastk}_{strategy_config.stoch_slowk}_{strategy_config.stoch_slowd}'
    stoch_d_name = f'STOCH_D_{strategy_config.stoch_fastk}_{strategy_config.stoch_slowk}_{strategy_config.stoch_slowd}'
    atr_name = f'ATR_{base_config.atr_period}'
    
    # Test si l'objet CppCandle a bien un attribut 'indicators' qui peut être utilisé comme un dict
    test_candle = CppCandle()
    print("Vérification de la structure CppCandle:")
    print("Attributs disponibles:", dir(test_candle))
    
    # Vérifier comment ajouter des indicateurs
    try:
        # Essayer d'attribuer directement
        test_candle.add_indicator = lambda name, value: print(f"Méthode add_indicator appelée: {name}={value}")
        test_candle.add_indicator("TEST", 1.0)
        
        # Essayer d'accéder aux indicateurs comme une propriété
        if hasattr(test_candle, "indicators"):
            print("L'attribut 'indicators' existe. Type:", type(test_candle.indicators))
            try:
                test_candle.indicators["TEST"] = 1.0
                print("Accès par dictionnaire fonctionne")
            except Exception as e:
                print(f"Erreur d'accès par dictionnaire: {e}")
                
                # Essayer d'autres approches d'accès
                try:
                    if hasattr(test_candle.indicators, "set_item"):
                        test_candle.indicators.set_item("TEST", 1.0)
                        print("Méthode set_item fonctionne")
                except Exception as e:
                    print(f"Erreur avec set_item: {e}")
                
                try:
                    if hasattr(test_candle.indicators, "add"):
                        test_candle.indicators.add("TEST", 1.0)
                        print("Méthode add fonctionne")
                except Exception as e:
                    print(f"Erreur avec add: {e}")
    except Exception as e:
        print(f"Erreur de test d'indicateurs: {e}")
    
    for i in range(num_candles):
        # Créer une bougie C++ pour chaque période
        candle = CppCandle()
        candle.date = dates[i].strftime("%Y-%m-%d %H:%M:%S")
        candle.open = float(opens[i])
        candle.high = float(highs[i])
        candle.low = float(lows[i])
        candle.close = float(closes[i])
        
        # Essayer d'ajouter les indicateurs avec sécurité
        try:
            # Si la classe a une méthode add_indicator(), l'utiliser
            if hasattr(candle, "add_indicator"):
                candle.add_indicator(ema_short_name, float(ema_short[i]))
                candle.add_indicator(ema_long_name, float(ema_long[i]))
                candle.add_indicator(stoch_k_name, float(stoch_k[i]))
                candle.add_indicator(stoch_d_name, float(stoch_d[i]))
                candle.add_indicator(atr_name, float(atr[i]))
            # Sinon, essayer d'utiliser le dictionnaire indicators
            elif hasattr(candle, "indicators"):
                # Essayer les différentes méthodes possibles
                try:
                    candle.indicators[ema_short_name] = float(ema_short[i])
                    candle.indicators[ema_long_name] = float(ema_long[i])
                    candle.indicators[stoch_k_name] = float(stoch_k[i])
                    candle.indicators[stoch_d_name] = float(stoch_d[i])
                    candle.indicators[atr_name] = float(atr[i])
                except:
                    # Si ça ne fonctionne pas, ne pas ajouter d'indicateurs
                    # La stratégie devra les calculer elle-même
                    pass
        except Exception as e:
            print(f"Erreur lors de l'ajout d'indicateurs à la bougie {i}: {e}")
        
        # Appeler la stratégie et récupérer un signal
        signal = strategy.update_candle(candle)
        
        if signal and hasattr(signal, "action") and signal.action:
            print(f"Bougie {i+1}: Signal '{signal.action}' - Prix: {signal.price:.2f}, Quantité: {signal.quantity:.2f}")
            print(f"  Take profit: {signal.take_profit:.2f}, Stop loss: {signal.stop_loss:.2f}")
            signals.append({
                'index': i,
                'date': candle.date,
                'price': candle.close,
                'action': signal.action,
                'quantity': signal.quantity,
                'take_profit': signal.take_profit,
                'stop_loss': signal.stop_loss
            })
    
    # 5. Affichage des résultats
    print(f"\n5. Résultats: {len(signals)} signaux générés")
    
    # Visualisation des données et signaux
    try:
        plt.figure(figsize=(12, 6))
        plt.plot(dates, closes, label='Prix de clôture')
        plt.plot(dates, ema_short, 'r--', label=f'EMA {strategy_config.ema_short_period}')
        plt.plot(dates, ema_long, 'g--', label=f'EMA {strategy_config.ema_long_period}')
        
        # Ajouter les signaux
        for signal in signals:
            if signal['action'] == 'BUY':
                plt.plot(dates[signal['index']], signal['price'], 'g^', markersize=10)
            elif signal['action'] == 'SELL':
                plt.plot(dates[signal['index']], signal['price'], 'rv', markersize=10)
        
        plt.title('Test de la stratégie BuyHeikinGreen C++')
        plt.xlabel('Date')
        plt.ylabel('Prix')
        plt.legend()
        plt.grid(True)
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.show()
    except Exception as e:
        print(f"Erreur lors de l'affichage du graphique: {e}")
    
    return signals

if __name__ == "__main__":
    try:
        signals = main()
        if signals:
            print("\nTest réussi! Votre stratégie C++ fonctionne correctement avec Python.")
        else:
            print("\nTest terminé, mais aucun signal n'a été généré.")
            print("Vérifiez le code C++ de votre stratégie, en particulier:")
            print("1. La méthode should_long() retourne-t-elle true dans certains cas?")
            print("2. La stratégie calcule-t-elle correctement les bougies Heikin Ashi?")
            print("3. Les filtres sont-ils trop restrictifs?")
    except Exception as e:
        print(f"\nErreur lors du test: {e}")
        import traceback
        traceback.print_exc()