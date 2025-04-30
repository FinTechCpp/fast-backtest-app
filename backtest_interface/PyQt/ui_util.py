import pandas as pd
from PyQt5.QtCore import QThread, pyqtSignal
from igtrader.backtestingpy.backtesting.backtesting import Backtest

class BacktestWorker(QThread):
    finished = pyqtSignal(object, object)  # Signaux pour renvoyer les résultats
    error = pyqtSignal(str)
    progress = pyqtSignal(int)
    
    def __init__(self, data, strategy, cash, spread, strategy_kwargs):
        super().__init__()
        self.data = data
        self.strategy = strategy
        self.cash = cash
        self.spread = spread
        self.strategy_kwargs = strategy_kwargs
    
    def run(self):
        try:
            maximum_leverage = self.strategy_kwargs.get('maximal_leverage', 20.0)
            margin = 1 / maximum_leverage
            
            # Exécuter le backtest
            # margin = 1 / leverage donc pour IG avec un levier max de 20 : levier = 20 = 1/0.05
            bt = Backtest(self.data, self.strategy, cash=self.cash, commission=.00, 
                          spread=self.spread, exclusive_orders=False, 
                          strategy_kwargs=self.strategy_kwargs, margin=margin)
            stats = bt.run()
            
            # Émettre le signal avec les résultats
            self.finished.emit(self.data, stats)
        
        except Exception as e:
            self.error.emit(str(e))

class ChartWorker(QThread):
    progress = pyqtSignal(str)
    error = pyqtSignal(str)
    chunk_ready = pyqtSignal(object, int, int)  # data_chunk, start_idx, end_idx
    finished_setup = pyqtSignal(object, object, dict)
    
    def __init__(self, data, stats, candle_type_combo, to_heikin_ashi_func):
        super().__init__()
        self.data = data
        self.stats = stats
        self.candle_type = candle_type_combo.currentText()
        self.to_heikin_ashi_func = to_heikin_ashi_func
        
    def run(self):
        try:
            # Préparation des données
            self.progress.emit("Préparation des données...")
            
            # Convertir à Heikin Ashi si nécessaire
            if self.candle_type == "Heikin Ashi":
                chart_data = self.to_heikin_ashi_func(self.data.copy())
            else:
                chart_data = self.data.copy()
                
            # Émettre signal avec les données nécessaires
            self.finished_setup.emit(chart_data, self.stats, {
                "indicator_columns": chart_data.attrs.get('indicator_columns', {})
            })
            
        except Exception as e:
            self.error.emit(str(e))

def to_heikin_ashi(df):
    data = df.copy()
    # Detect case
    o, h, l, c = (
        ('Open', 'High', 'Low', 'Close')
        if {'Open', 'High', 'Low', 'Close'}.issubset(data.columns)
        else ('open', 'high', 'low', 'close')
    )
    # HA close
    ha_close = (data[o] + data[h] + data[l] + data[c]) / 4.0
    # HA open
    ha_open = pd.Series(index=data.index, dtype=float)
    ha_open.iloc[0] = (data[o].iloc[0] + data[c].iloc[0]) / 2.0
    for i in range(1, len(data)):
        ha_open.iloc[i] = (ha_open.iloc[i - 1] + ha_close.iloc[i - 1]) / 2.0
    # HA high & low
    ha_high = pd.concat([data[h], ha_open, ha_close], axis=1).max(axis=1)
    ha_low = pd.concat([data[l], ha_open, ha_close], axis=1).min(axis=1)
    # Assemble result
    ha = pd.DataFrame({
        'open': ha_open,
        'high': ha_high,
        'low': ha_low,
        'close': ha_close
    }, index=data.index)
    
    # Préserver les colonnes d'indicateurs et autres métadonnées du DataFrame original
    if hasattr(data, 'attrs'):
        ha.attrs = data.attrs.copy()
    for col in data.columns:
        if col not in [o, h, l, c] and col not in ha.columns:
            ha[col] = data[col]
    
    return ha