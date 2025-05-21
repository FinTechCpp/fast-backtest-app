import logging
import pandas as pd
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel, QHBoxLayout, QCheckBox
from PyQt5.QtCore import Qt
from lightweight_charts_esistjosh.widgets import QtChart
from backtestApp.ui_util import to_heikin_ashi
import time
import numpy as np
from cpp_strategies import (
    CppEMA, 
    CppSTOCH, 
    CppATR,
    CppRSI,
    CppATRC
)

from backtestApp.views.base_view import ResultView

class CandleConverter:
    """Utilitaire pour convertir des données pandas/numpy en objets BasicCandle pour les indicateurs C++"""
    
    @staticmethod
    def create_cpp_candle(timestamp, open_val, high_val, low_val, close_val):
        """Crée un objet BasicCandle C++ à partir des données d'une bougie"""
        from cpp_strategies import CppBasicCandle, CppDateTime, CppTime
        
        # Convertir le timestamp en composants DateTime pour C++
        dt = pd.to_datetime(timestamp)
        
        # Créer un objet Time C++
        time_obj = CppTime()
        time_obj.hour = dt.hour
        time_obj.minute = dt.minute
        time_obj.second = dt.second
        
        # Créer un objet DateTime C++
        date_obj = CppDateTime()
        date_obj.year = dt.year
        date_obj.month = dt.month
        date_obj.day = dt.day
        date_obj.time = time_obj
        
        # Créer l'objet CppBasicCandle C++
        cppCandle = CppBasicCandle()
        cppCandle.date = date_obj
        cppCandle.open = float(open_val)
        cppCandle.high = float(high_val)
        cppCandle.low = float(low_val)
        cppCandle.close = float(close_val)

        return cppCandle

    
    @staticmethod
    def create_cpp_candles_from_dataframe(df, timestamp_col='time'):
        """Crée une liste d'objets BasicCandle C++ à partir d'un DataFrame pandas"""
        candles = []
        
        # Identifier les noms de colonnes OHLC
        if 'close' in df.columns:
            open_col, high_col, low_col, close_col = 'open', 'high', 'low', 'close'
        elif 'Close' in df.columns:
            open_col, high_col, low_col, close_col = 'Open', 'High', 'Low', 'Close'
        else:
            raise ValueError("Colonnes OHLC introuvables dans les données")
        
        # Créer les objets candle
        for i, row in df.iterrows():
            candle = CandleConverter.create_cpp_candle(
                row[timestamp_col],
                row[open_col],
                row[high_col],
                row[low_col],
                row[close_col]
            )
            candles.append(candle)
        
        return candles
    

class ChartView(ResultView):
    """Vue pour afficher les graphiques de prix et d'indicateurs."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.current_chart = None

        
    def create(self):
        """Crée le widget principal pour les graphiques."""
        self.chart_container = QWidget()
        self.chart_layout = QVBoxLayout(self.chart_container)
        
        # Placeholder pour le graphique (sera remplacé lors de l'exécution)
        self.chart_placeholder = QLabel("Exécutez le backtest pour afficher les graphiques")
        self.chart_placeholder.setAlignment(Qt.AlignCenter)
        self.chart_layout.addWidget(self.chart_placeholder)
        
        return self.chart_container
    
    def update(self, data=None, stats=None):
        """Met à jour le graphique avec les nouvelles données."""
        if data is None:
            return
        
        # Supprimer l'ancien graphique s'il existe
        self.clear_layout(self.chart_layout)
        
        # Création du conteneur pour le graphique
        chart_container = QWidget()
        chart_layout = QVBoxLayout(chart_container)
        chart_layout.setContentsMargins(0, 0, 0, 0)
        
        # Créer le graphique principal
        chart = QtChart(chart_container, toolbox=True, inner_height=0.6)

        # Configurer l'apparence du graphique
        chart.layout(background_color='#f0f8ff', text_color='black')
        chart.grid(color='rgba(1,1,1,0.1)', vert_enabled=False, horz_enabled=False, style='solid')
        chart.price_scale(minimum_width=120, auto_scale=True, mode='normal', scale_margin_bottom=0.1, scale_margin_top=0.1)
        chart.time_scale(visible=True, seconds_visible=True, border_color='black', min_bar_spacing=0.0)
        chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        chart.legend(visible=True, color_based_on_candle=False, color='rgba(1,1,1,1)', font_size=12, font_family='Arial')
        
        # Appliquer les paramètres généraux
        general_params = self.parent.general_params_panel.get_values()
        if general_params['candle_type'] == "Heikin Ashi":
            data = to_heikin_ashi(data)
        
        start_time = time.time()
        chart.set(data)
        logging.info(f"Chart set time: {(time.time() - start_time) * 1000:.2f} ms")            
            
        # Ajouter le graphique au layout
        chart_layout.addWidget(chart.get_webview())
        
        # Stocker la référence au graphique
        self.current_chart = chart
        
        # Si des statistiques sont fournies, ajouter les indicateurs et les trades
        if stats is not None:
            start_time2 = time.time()
            equity_chart = self._add_equity_subchart(chart, data, stats)
                
            end_time2 = time.time()
            logging.info(f"_add_equity_subchart time: {(end_time2- start_time2) * 1000:.2f} ms")
            
            self._add_indicators(chart, data)
            
            end_time3 = time.time()
            logging.info(f"_add_indicators time: {(end_time3 - end_time2) * 1000:.2f} ms")
            
            self._add_trade_markers(chart, stats)
            end_time4 = time.time()
            logging.info(f"_add_trade_markers time: {(end_time4 - end_time3) * 1000:.2f} ms")
            
            # Fit the chart to show all data
            chart.fit()
        
        self.chart_layout.addWidget(chart_container)
    
    def _add_equity_subchart(self, chart: QtChart, data, stats):
        """Ajoute le sous-graphique de l'équité - version hautement optimisée."""
        import numpy as np
        
        # Créer le sous-graphique
        equity_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
        equity_chart.layout(background_color='#f0f8ff')
        equity_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
        equity_chart.time_scale(visible=False, min_bar_spacing=0.0)
        equity_chart.price_scale(minimum_width=120)
        equity_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        
        # Récupérer les paramètres généraux
        general_params = self.parent.general_params_panel.get_values()
        initial_equity = general_params['cash']
        
        # Obtenir les trades triés par date de sortie
        if '_trades' not in stats or len(stats['_trades']) == 0:
            # Cas simple: pas de trades, ligne plate
            equity_df = pd.DataFrame({
                'time': data['time'],
                'Equity': initial_equity
            })
        else:
            # Obtenir et préparer les timestamps de toutes les bougies
            all_timestamps = pd.to_datetime(data['time'])
            
            # Préparer les données de trades - conversion efficace
            trades_df = stats['_trades'].copy()
            exit_times = pd.to_datetime(trades_df['ExitTime']).values
            pnl_values = trades_df['PnL'].values
            
            # Création optimisée du DataFrame des changements d'équité en un seul appel
            equity_changes = pd.DataFrame({
                'time': exit_times,
                'Equity': initial_equity + np.cumsum(pnl_values)
            })
            
            # Créer le DataFrame final avec merge_asof (opération vectorisée efficace)
            equity_df = pd.DataFrame({'time': all_timestamps})
            
            # Effectuer le merge_asof sans tri préalable qui crée des copies (plus efficace)
            # On trie directement dans le merge_asof
            equity_df = pd.merge_asof(
                equity_df.sort_values('time'),
                equity_changes.sort_values('time'),
                on='time',
                direction='backward'
            )
            
            # Remplacer les NaN sans utiliser inplace=True (correction du warning)
            equity_df = equity_df.assign(Equity=lambda x: x['Equity'].fillna(initial_equity))
        
        # Convertir efficacement les timestamps en strings
        equity_df['time'] = equity_df['time'].dt.strftime('%Y-%m-%d %H:%M:%S')
        
        # Ajouter la ligne d'équité
        equity_line = equity_chart.create_line(
            name='Equity', 
            color='rgba(20,20,180,1)', 
            width=1, 
            price_line=False
        )
        
        # Ajouter la ligne horizontale de l'équité initiale
        equity_line.horizontal_line(
            price=initial_equity, 
            color='black', 
            width=1, 
            style='dashed', 
            text='Initial Equity'
        )
        
        # Définir les données
        equity_line.set(equity_df)
        
        return equity_chart
    
    def _add_indicators(self, chart: QtChart, data):
        """Ajoute les indicateurs au graphique en les calculant avec les implémentations C++."""        
        start_time = time.time()
        
        # Define indicator colors
        indicator_colors = {
            'EMA': ['blue', 'purple', 'red', 'green', 'cyan', 'magenta'],
            'STOCH_K': ['blue'],
            'STOCH_D': ['red'],
            'ATR': ['green', 'teal'],
            'RSI': ['purple']  # Définir une couleur pour le RSI
        }
        
        # Dictionnaire pour stocker les sous-graphiques créés
        subcharts = {}
        
        # Déterminer les noms corrects des colonnes (majuscules ou minuscules)
        if 'close' in data.columns:
            open_col = 'open'
            close_col = 'close'
            high_col = 'high' 
            low_col = 'low'
        elif 'Close' in data.columns:
            open_col = 'Open'
            close_col = 'Close'
            high_col = 'High'
            low_col = 'Low'
        else:
            logging.error("Colonnes OHLC introuvables dans les données")
            return {}
        
        # Convertir les colonnes en tableaux NumPy pour des performances optimales
        open_values = data[open_col].values
        close_values = data[close_col].values
        high_values = data[high_col].values
        low_values = data[low_col].values
        
        # Récupérer les paramètres de la stratégie
        strategy_config = self.parent.get_strategy_config()
        strategy_name = strategy_config.get('strategy', '')

        # --------------------------
        # EMAs
        # --------------------------
        ema_periods = []
        
        # Si stratégie BuyHeikinGreen, utiliser ses paramètres spécifiques
        if 'BuyHeikinGreen' in strategy_name or 'SellHeikinRed' in strategy_name:
            ema_short_period = int(strategy_config.get('ema_short_period', 9))
            ema_long_period = int(strategy_config.get('ema_long_period', 21))
            if strategy_config.get('use_ema_short_filter', True):
                ema_periods.append(ema_short_period)
            if strategy_config.get('use_ema_long_filter', True):
                ema_periods.append(ema_long_period)
        else:
            # Pour les autres stratégies, vérifier les paramètres génériques
            if strategy_config.get('use_ema', False):
                ema_periods = [9, 21, 50, 200]  # Valeurs par défaut
        
        # Calculer les EMAs avec les implémentations C++
        for i, period in enumerate(ema_periods):
            ema_col = f'EMA_{period}'
            
            # Créer et initialiser l'indicateur EMA C++
            ema = CppEMA(period)

            # Créer un sous-ensemble de données pour l'initialisation
            init_data = data.iloc[:period*2]

            # Convertir les données en objets BasicCandle
            candles = CandleConverter.create_cpp_candles_from_dataframe(init_data)
            
            # Initialiser avec l'historique de bougies
            ema.initialize_with_history(candles)

            # Calculer les valeurs EMA pour toutes les bougies
            ema_values = np.zeros(len(close_values))

            # Calculer pour les bougies initiales
            for j in range(period*2):
                if j < len(init_data):
                    ema_values[j] = np.nan  # Ces valeurs sont déjà calculées par initialize_with_history
            
            # Calculer pour le reste des bougies
            for j in range(period*2, len(close_values)):
                # Créer un objet BasicCandle pour cette mise à jour
                candle = CandleConverter.create_cpp_candle(
                    data['time'].iloc[j],
                    data[open_col].iloc[j],
                    data[high_col].iloc[j],
                    data[low_col].iloc[j],
                    data[close_col].iloc[j]
                )
                ema_values[j] = ema.update(candle)

            # Créer un DataFrame pour la ligne
            ema_df = pd.DataFrame({
                'time': data['time'],
                ema_col: ema_values
            })
            
            # Ajouter la ligne au graphique
            color_idx = i % len(indicator_colors['EMA'])
            ema_line = chart.create_line(
                name=ema_col, 
                color=indicator_colors['EMA'][color_idx], 
                width=1.5, 
                price_line=False
            )
            ema_line.set(ema_df)
            logging.debug(f"Added EMA indicator: {ema_col}")
        
        # --------------------------
        # Stochastique
        # --------------------------
        show_stoch = True
        stoch_k_period = 14  # valeur par défaut
        stoch_d_period = 3  # valeur par défaut
        stoch_slowing = 3  # valeur par défaut
        
        if 'BuyHeikinGreen' in strategy_name or 'SellHeikinRed' in strategy_name:
            show_stoch = strategy_config.get('use_stoch_filter', False)
            stoch_k_period = int(strategy_config.get('stoch_fastk', 14))
            stoch_slowing = int(strategy_config.get('stoch_slowk', 3))
            stoch_d_period = int(strategy_config.get('stoch_slowd', 3))
        
        if show_stoch:
            # Créer et initialiser l'indicateur STOCH C++
            stoch = CppSTOCH(stoch_k_period, stoch_slowing, stoch_d_period)
            
            # Calcul du nombre de bougies nécessaires pour l'initialisation
            hist_size = stoch_k_period + max(stoch_slowing, stoch_d_period)
            
            # Créer un sous-ensemble de données pour l'initialisation
            init_data = data.iloc[:hist_size]
            
            # Convertir les données en objets BasicCandle
            candles = CandleConverter.create_cpp_candles_from_dataframe(init_data)
            
            # Initialiser avec l'historique de bougies
            if len(candles) > 0:
                stoch.initialize_with_history(candles)
            
            # Calculer les valeurs stochastiques pour toutes les bougies
            stoch_k_values = np.zeros(len(close_values))
            stoch_d_values = np.zeros(len(close_values))
            
            # Calculer pour les bougies initiales
            for j in range(hist_size):
                if j < len(init_data):
                    stoch_k_values[j] = np.nan
                    stoch_d_values[j] = np.nan
            
            # Calculer pour le reste des bougies
            for j in range(hist_size, len(close_values)):
                # Créer un objet BasicCandle pour cette mise à jour
                candle = CandleConverter.create_cpp_candle(
                    data['time'].iloc[j],
                    data[open_col].iloc[j],
                    data[high_col].iloc[j],
                    data[low_col].iloc[j],
                    data[close_col].iloc[j]
                )
                
                # Mise à jour avec la bougie
                k_d_pair = stoch.update(candle)
                stoch_k_values[j] = k_d_pair[0]  # K value
                stoch_d_values[j] = k_d_pair[1]  # D value
            
            # Créer le sous-graphique pour le stochastique
            stoch_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
            stoch_chart.layout(background_color='#f0f8ff')
            stoch_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
            stoch_chart.time_scale(visible=False, min_bar_spacing=0.0)
            stoch_chart.price_scale(minimum_width=120)
            stoch_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
            subcharts['stoch_chart'] = stoch_chart
            
            # Ajouter les lignes K et D
            stoch_lines = []
            
            # Ligne K
            k_df = pd.DataFrame({
                'time': data['time'],
                'STOCH_K': stoch_k_values
            })
            k_line = stoch_chart.create_line(
                name='STOCH_K', 
                color=indicator_colors['STOCH_K'][0], 
                width=1, 
                price_line=False
            )
            k_line.set(k_df)
            stoch_lines.append(k_line)
            logging.debug("Added Stochastic K indicator")
            
            # Ligne D
            d_df = pd.DataFrame({
                'time': data['time'],
                'STOCH_D': stoch_d_values
            })
            d_line = stoch_chart.create_line(
                name='STOCH_D', 
                color=indicator_colors['STOCH_D'][0], 
                width=1, 
                price_line=False
            )
            d_line.set(d_df.dropna())
            stoch_lines.append(d_line)
            logging.debug("Added Stochastic D indicator")
            
            # Ajouter les lignes de référence
            if stoch_lines:
                stoch_lines[0].horizontal_line(price=80, color='green', width=1, style='dashed', text='Overbought(80)')
                stoch_lines[0].horizontal_line(price=20, color='red', width=1, style='dashed', text='Oversold(20)')
                stoch_lines[0].horizontal_line(price=50, color='blue', width=1, style='dashed', text='Neutral(50)')
        
        # --------------------------
        # ATR (log)
        # --------------------------
        atr_period = int(strategy_config.get('atr_period', 14))
        
        # Créer et initialiser l'indicateur ATR C++
        atr = CppATR(atr_period)
        atrc = CppATRC(atr_period)
        
        init_data = data.iloc[:atr_period*2]
        candles = CandleConverter.create_cpp_candles_from_dataframe(init_data)

        if candles:
            atr.initialize_with_history(candles)
            atrc.initialize_with_history(candles)

        # Calculer les valeurs ATR pour toutes les bougies
        atr_values = np.zeros(len(close_values))
        atrc_values = np.zeros(len(close_values))

        # Marquer les premières valeurs comme non-initialisées
        atr_values[:atr_period*2] = np.nan
        atrc_values[:atr_period*2] = np.nan

        for j in range(atr_period*2, len(close_values)):
            # Créer un objet BasicCandle pour cette mise à jour
            candle = CandleConverter.create_cpp_candle(
                data['time'].iloc[j],
                data[open_col].iloc[j],
                data[high_col].iloc[j],
                data[low_col].iloc[j],
                data[close_col].iloc[j]
            )
            
            atr_values[j] = atr.update(candle)
            atrc_values[j] = atrc.update(candle)

        # Appliquer le log (logarithme népérien) en évitant les valeurs <= 0
        log_atr_values = np.where(atr_values > 0, np.log(atr_values), np.nan)
        log_atrc_values = np.where(atrc_values > 0, np.log(atrc_values), np.nan)
        
        # Créer le sous-graphique pour l'ATR (log)
        atr_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
        atr_chart.layout(background_color='#f0f8ff')
        atr_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
        atr_chart.time_scale(visible=False, min_bar_spacing=0.0)
        atr_chart.price_scale(minimum_width=120)
        atr_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        subcharts['atr_chart'] = atr_chart
        
        # Ajouter la ligne ATR (log)
        atr_df = pd.DataFrame({
            'time': data['time'],
            f'log_ATR_{atr_period}': log_atr_values,
        })
        atr_line = atr_chart.create_line(
            name=f'log_ATR_{atr_period}', 
            color=indicator_colors['ATR'][0], 
            width=1, 
            price_line=False
        )
        atr_line.set(atr_df)
        logging.debug(f"Added log ATR indicator: log_ATR_{atr_period}")

        # Ajouter la ligne ATRC (log)
        atrc_df = pd.DataFrame({
            'time': data['time'],
            f'log_ATRC_{atr_period}': log_atrc_values,
        })
        atrc_line = atr_chart.create_line(
            name=f'log_ATRC_{atr_period}', 
            color=indicator_colors['ATR'][1], 
            width=1, 
            price_line=False
        )
        atrc_line.set(atrc_df)
        logging.debug(f"Added log ATRC indicator: log_ATRC_{atr_period}")
        
        # --------------------------
        # RSI
        # --------------------------
        # Vérifier si le RSI doit être affiché
        show_rsi = strategy_config.get('use_rsi_filter', False)
        rsi_period = int(strategy_config.get('rsi_period', 14))

        if show_rsi:
            # Créer et initialiser l'indicateur RSI C++
            # Si la classe CppRSI n'existe pas, vous devrez l'implémenter ou utiliser une autre approche
            rsi = CppRSI(rsi_period)
            
            # Initialiser avec un historique suffisant
            init_data = data.iloc[:hist_size]
            candles = CandleConverter.create_cpp_candles_from_dataframe(init_data)

            if candles:
                rsi.initialize_with_history(candles)
            
            # Calculer les valeurs RSI pour toutes les bougies
            rsi_values = np.zeros(len(close_values))

            # Marquer les premières valeurs comme non-initialisées
            rsi_values[:hist_size] = np.nan

            for j in range(hist_size, len(close_values)):
                # Créer un objet BasicCandle pour cette mise à jour
                candle = CandleConverter.create_cpp_candle(
                    data['time'].iloc[j],
                    data[open_col].iloc[j],
                    data[high_col].iloc[j],
                    data[low_col].iloc[j],
                    data[close_col].iloc[j]
                )
                
                rsi_values[j] = rsi.update(candle)
            
            # Créer le sous-graphique pour le RSI
            rsi_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
            rsi_chart.layout(background_color='#f0f8ff')
            rsi_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
            rsi_chart.time_scale(visible=False, min_bar_spacing=0.0)
            rsi_chart.price_scale(minimum_width=120)
            rsi_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
            subcharts['rsi_chart'] = rsi_chart
            
            # Créer un DataFrame pour le RSI
            rsi_df = pd.DataFrame({
                'time': data['time'],
                f'RSI_{rsi_period}': rsi_values
            })
            # Ajouter la ligne RSI
            rsi_line = rsi_chart.create_line(
                name=f'RSI_{rsi_period}', 
                color=indicator_colors['RSI'][0], 
                width=1.5, 
                price_line=False
            )
            rsi_line.set(rsi_df)
            
            # Ajouter les lignes de référence pour le RSI
            rsi_line.horizontal_line(price=70, color='red', width=1, style='dashed', text='Overbought(70)')
            rsi_line.horizontal_line(price=30, color='green', width=1, style='dashed', text='Oversold(30)')
            rsi_line.horizontal_line(price=50, color='blue', width=1, style='dashed', text='Neutral(50)')
            
            logging.debug(f"Added RSI indicator: RSI_{rsi_period}")
        
        # Synchroniser les tooltips entre les graphiques si nous avons des sous-graphiques
        if subcharts:
            sync_charts = [chart]
            for subchart_name, subchart in subcharts.items():
                sync_charts.append(subchart)
            
            chart.create_synchronized_tooltip(charts=sync_charts, options={
                "backgroundColor": "rgba(255, 255, 255, 0.9)",
                "textColor": "#333",
                "padding": "8px",
                "showOHLC": True,
                "showDateTime": False}, 
                trigger_key="Shift",
                toggle_mode=False)
        
        logging.info(f"C++ indicators calculation time: {(time.time() - start_time) * 1000:.2f} ms")
        return subcharts
    
    def _add_trade_markers(self, chart: QtChart, stats):
        """Ajoute les marqueurs de trades au graphique."""
        trades = stats['_trades']
        for i, trade in trades.iterrows():
            entry_time = pd.to_datetime(trade['EntryTime'])
            exit_time = pd.to_datetime(trade['ExitTime'])
            entry_price = trade['EntryPrice']
            exit_price = trade['ExitPrice']
            
            entry_color = "blue" if trade['Size'] > 0 else "red" if trade['Size'] < 0 else "black"
            exit_color = "green" if trade['PnL'] > 0 else "red" if trade['PnL'] < 0 else "black"
            
            # Entry marker
            chart.marker(
                time=entry_time,
                position="above",
                color=entry_color,
                text=f"Entry: {entry_price:.2f}",
                shape="arrow_down",
                size=1)
            chart.point_marker(time=entry_time,
                            price=entry_price,
                            fill_color='black',
                            line_color='black',
                            width=3,
                            radius=5)
                            
            # Exit marker
            chart.marker(
                time=exit_time,
                position="below",
                color=exit_color,
                text=f"Exit: {exit_price:.2f}\n"
                     f"(P/L: {trade['PnL']:.2f} $ / {round(trade['ReturnPct'], 5)}%)\n",
                shape="arrow_up",
                size=1)
            chart.point_marker(time=exit_time,
                            price=exit_price,
                            fill_color='black',
                            line_color='black',
                            width=3,
                            radius=5)