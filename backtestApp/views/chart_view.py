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
    
    def clear_layout(self, layout):
        """Supprime tous les widgets d'un layout."""
        while layout.count():
            item = layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()
            elif item.layout() is not None:
                self.clear_layout(item.layout())
    
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
        equity_chart = chart.create_subchart(height=0.1, width=1, position="top", sync=True)
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
        """Ajoute les indicateurs au graphique."""
        # Define indicator colors
        indicator_colors = {
            'EMA': ['blue', 'purple', 'red', 'green', 'cyan', 'magenta'],
            'SUPERTREND': ['orange', 'brown', 'gold'],
            'STOCH_K': ['blue'],
            'STOCH_D': ['red'],
            'ATR': ['green', 'teal']
        }
        
        # Get indicator columns from data attributes
        indicator_columns = data.attrs.get('indicator_columns', {})
        
        logging.debug(f"data.attrs: {data.attrs}")
        logging.debug(f"Indicator columns: {indicator_columns}")
        
        # Dictionnaire pour stocker les sous-graphiques créés
        subcharts = {}
        
        # Add EMAs to main chart
        if 'EMA' in indicator_columns:
            for i, ema_col in enumerate(indicator_columns['EMA']):
                if ema_col in data.columns:
                    color_idx = i % len(indicator_colors['EMA'])
                    ema_line = chart.create_line(
                        name=ema_col, 
                        color=indicator_colors['EMA'][color_idx], 
                        width=1, 
                        price_line=False
                    )
                    ema_df = data[['time', ema_col]].copy()
                    ema_line.set(ema_df)
                    logging.debug(f"Added EMA indicator: {ema_col}")
        
        # Add SuperTrend to main chart
        if 'SUPERTREND' in indicator_columns:
            for i, st_col in enumerate(indicator_columns['SUPERTREND']):
                if st_col in data.columns:
                    color_idx = i % len(indicator_colors['SUPERTREND'])
                    st_line = chart.create_line(
                        name=st_col, 
                        color=indicator_colors['SUPERTREND'][color_idx], 
                        width=1, 
                        price_line=False
                    )
                    st_df = data[['time', st_col]].copy()
                    st_line.set(st_df)
                    logging.debug(f"Added SuperTrend indicator: {st_col}")
        
        # STOCHASTIC SUBCHART
        if 'STOCH' in indicator_columns:
            stoch_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
            stoch_chart.layout(background_color='#f0f8ff')
            stoch_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
            stoch_chart.time_scale(visible=False, min_bar_spacing=0.0)
            stoch_chart.price_scale(minimum_width=120)
            stoch_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
            subcharts['stoch_chart'] = stoch_chart
            
            stoch_lines = []
            for i, stoch_col in enumerate(indicator_columns['STOCH']):
                if stoch_col in data.columns:
                    color = indicator_colors['STOCH_K'][0] if 'K' in stoch_col else indicator_colors['STOCH_D'][0]
                    stoch_line = stoch_chart.create_line(name=stoch_col, color=color, width=1, price_line=False)
                    stoch_df = data[['time', stoch_col]].copy()
                    stoch_line.set(stoch_df)
                    stoch_lines.append(stoch_line)
                    logging.debug(f"Added Stochastic indicator: {stoch_col}")
            
            # Add reference lines if we created any stochastic lines
            if stoch_lines:
                stoch_lines[0].horizontal_line(price=80, color='green', width=1, style='dashed', text='Overbought(80)')
                stoch_lines[0].horizontal_line(price=20, color='red', width=1, style='dashed', text='Oversold(20)')
                stoch_lines[0].horizontal_line(price=50, color='blue', width=1, style='dashed', text='Neutral(50)')
        
        # ATR SUBCHART
        if 'ATR' in indicator_columns:
            atr_chart = chart.create_subchart(height=0.1, width=1, position="bottom", sync=True)
            atr_chart.layout(background_color='#f0f8ff')
            atr_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
            atr_chart.time_scale(visible=False, min_bar_spacing=0.0)
            atr_chart.price_scale(minimum_width=120)
            atr_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
            subcharts['atr_chart'] = atr_chart
            
            for i, atr_col in enumerate(indicator_columns['ATR']):
                if atr_col in data.columns:
                    color_idx = i % len(indicator_colors['ATR'])
                    atr_line = atr_chart.create_line(
                        name=atr_col, 
                        color=indicator_colors['ATR'][color_idx], 
                        width=1, 
                        price_line=False
                    )
                    atr_df = data[['time', atr_col]].copy()
                    atr_line.set(atr_df)
                    logging.debug(f"Added ATR indicator: {atr_col}")
        
        # Synchronize tooltips between charts if we have subcharts
        if subcharts:
            sync_charts = [chart]
            if 'equity_chart' in subcharts:
                sync_charts.append(subcharts['equity_chart'])
            if 'atr_chart' in subcharts:
                sync_charts.append(subcharts['atr_chart'])
            if 'stoch_chart' in subcharts:
                sync_charts.append(subcharts['stoch_chart'])
            
            chart.create_synchronized_tooltip(charts=sync_charts, options={
                "backgroundColor": "rgba(255, 255, 255, 0.9)",
                "textColor": "#333",
                "padding": "8px"}, 
                trigger_key="Shift",
                toggle_mode=False)
                
        return subcharts
    
    def _add_trade_markers(self, chart, stats):
        """Ajoute les marqueurs de trades au graphique."""
        trades = stats['_trades']
        for i, trade in trades.iterrows():
            entry_time = pd.to_datetime(trade['EntryTime'])
            exit_time = pd.to_datetime(trade['ExitTime'])
            entry_price = trade['EntryPrice']
            exit_price = trade['ExitPrice']
            
            entry_color = "blue" if trade['Size'] > 0 else "red"
            exit_color = "green" if trade['PnL'] > 0 else "red"
            
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