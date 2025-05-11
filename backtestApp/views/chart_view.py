import logging
import pandas as pd
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel
from PyQt5.QtCore import Qt
from lightweight_charts_esistjosh.widgets import QtChart
from ui_util import to_heikin_ashi

from views.base_view import ResultView

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
        chart = QtChart(chart_container, toolbox=True, inner_height=0.7)

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
        chart.set(data)
            
        # Ajouter le graphique au layout
        chart_layout.addWidget(chart.get_webview())
        
        # Stocker la référence au graphique
        self.current_chart = chart
        
        # Si des statistiques sont fournies, ajouter les indicateurs et les trades
        if stats is not None:
            self._add_equity_subchart(chart, data, stats)
            self._add_indicators(chart, data)
            self._add_trade_markers(chart, stats)
            
            # Fit the chart to show all data
            chart.fit()
        
        # Ajouter le conteneur du graphique au layout
        self.chart_layout.addWidget(chart_container)
    
    def _add_equity_subchart(self, chart, data, stats):
        """Ajoute le sous-graphique de l'équité."""
        equity_chart = chart.create_subchart(height=0.1, width=1, sync=True)
        equity_chart.layout(background_color='#f0f8ff')
        equity_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
        equity_chart.time_scale(visible=False, min_bar_spacing=0.0)
        equity_chart.price_scale(minimum_width=120)
        equity_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        
        # Create a realized PnL equity curve that only changes on trade exits
        all_timestamps = data['time'].copy()
        equity_df = pd.DataFrame({'time': all_timestamps})
        
        # Initial equity value (cash)
        general_params = self.parent.general_params_panel.get_values()
        initial_equity = general_params['cash']

        # Get trade data sorted by exit time
        trades_df = stats['_trades'].sort_values('ExitTime')
                    
        # Create a series mapping exit times to cumulative PnL
        current_equity = initial_equity
        equity_at_exit = {}
        for _, trade in trades_df.iterrows():
            current_equity += trade['PnL']
            equity_at_exit[trade['ExitTime']] = current_equity
        
        # Create a new column for equity value
        equity_df['Equity'] = initial_equity
        
        # Update equity values at trade exit times
        for i, row in equity_df.iterrows():
            # Convert row time to datetime for comparison
            row_time = pd.to_datetime(row['time'])
            
            # Find the most recent trade exit time that's not after current row time
            latest_equity = initial_equity
            for exit_time, equity_value in equity_at_exit.items():
                if exit_time <= row_time:
                    latest_equity = equity_value
            
            # Set the equity value for this timestamp
            equity_df.at[i, 'Equity'] = latest_equity

        # Add the main equity line
        equity_line = equity_chart.create_line(name='Equity', color='rgba(20,20,180,1)', width=1, price_line=False)
        equity_line.horizontal_line(price=initial_equity, color='black', width=1, style='dashed', text='Initial Equity')
        
        # Convert time to string and handle Timedelta objects
        equity_df['time'] = equity_df['time'].astype(str)
        
        # Set the data for the equity line
        equity_line.set(equity_df)
        
        return equity_chart
    
    def _add_indicators(self, chart, data):
        """Ajoute les indicateurs au graphique en les calculant directement avec talib."""
        try:
            import talib
        except ImportError:
            logging.error("talib n'est pas installé. Les indicateurs ne seront pas affichés.")
            return {}

        # Define indicator colors
        indicator_colors = {
            'EMA': ['blue', 'purple', 'red', 'green', 'cyan', 'magenta'],
            'SUPERTREND': ['orange', 'brown', 'gold'],
            'STOCH_K': ['blue'],
            'STOCH_D': ['red'],
            'ATR': ['green', 'teal']
        }
        
        # Dictionnaire pour stocker les sous-graphiques créés
        subcharts = {}
        
        # Déterminer les noms corrects des colonnes (majuscules ou minuscules)
        if 'close' in data.columns:
            close_col = 'close'
            high_col = 'high' 
            low_col = 'low'
        elif 'Close' in data.columns:
            close_col = 'Close'
            high_col = 'High'
            low_col = 'Low'
        else:
            logging.error("Colonnes OHLC introuvables dans les données")
            return {}
        
        # Récupérer les paramètres de la stratégie
        strategy_config = self.parent.get_strategy_config()
        strategy_name = strategy_config.get('strategy', '')
        
        # --------------------------------
        # EMAs
        # --------------------------------
        ema_periods = []
        
        # Si stratégie BuyHeikinGreen, utiliser ses paramètres spécifiques
        if 'BuyHeikinGreen' in strategy_name:
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
        
        for i, period in enumerate(ema_periods):
            # Calculer l'EMA avec talib
            ema_col = f'EMA_{period}'
            ema_values = talib.EMA(data[close_col].values, timeperiod=period)
            
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
        
        # --------------------------------
        # Stochastique
        # --------------------------------
        show_stoch = False
        stoch_k_period = 14  # valeur par défaut
        stoch_d_period = 3  # valeur par défaut
        stoch_slowing = 3  # valeur par défaut
        
        if 'BuyHeikinGreen' in strategy_name:
            show_stoch = strategy_config.get('use_stoch_filter', False)
            stoch_k_period = int(strategy_config.get('stoch_fastk', 14))
            stoch_slowing = int(strategy_config.get('stoch_slowk', 3))
            stoch_d_period = int(strategy_config.get('stoch_slowd', 3))
        
        if show_stoch:
            # Calculer Stochastique avec talib
            stoch_k, stoch_d = talib.STOCH(
                data[high_col].values,
                data[low_col].values,
                data[close_col].values,
                fastk_period=stoch_k_period,
                slowk_period=stoch_slowing,
                slowk_matype=0,
                slowd_period=stoch_d_period,
                slowd_matype=0
            )
            
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
                'STOCH_K': stoch_k
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
                'STOCH_D': stoch_d
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
        
        # --------------------------------
        # ATR
        # --------------------------------
        atr_period = int(strategy_config.get('atr_period', 14))
        
        # Calculer ATR avec talib
        atr_values = talib.ATR(
            data[high_col].values,
            data[low_col].values,
            data[close_col].values,
            timeperiod=atr_period
        )
        
        # Créer le sous-graphique pour l'ATR
        atr_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
        atr_chart.layout(background_color='#f0f8ff')
        atr_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
        atr_chart.time_scale(visible=False, min_bar_spacing=0.0)
        atr_chart.price_scale(minimum_width=120)
        atr_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        subcharts['atr_chart'] = atr_chart
        
        # Ajouter la ligne ATR
        atr_df = pd.DataFrame({
            'time': data['time'],
            f'ATR_{atr_period}': atr_values
        })
        atr_line = atr_chart.create_line(
            name=f'ATR_{atr_period}', 
            color=indicator_colors['ATR'][0], 
            width=1, 
            price_line=False
        )
        atr_line.set(atr_df)
        logging.debug(f"Added ATR indicator: ATR_{atr_period}")
        
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