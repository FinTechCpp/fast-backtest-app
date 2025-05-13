import pandas as pd
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QApplication
from PyQt5.QtCore import Qt
from lightweight_charts_esistjosh.widgets import QtChart
from backtestApp.views.base_view import ResultView
from enum import Enum


class DataGranularity(Enum):
    D = "D"  # Jour
    W = "W"  # Semaine
    M = "ME"  # Mois
    Q = "QE"  # Trimestre
    Y = "YE"  # Année


class HistogramViewCandle(ResultView):
    """Vue pour afficher l'évolution de l'équité en chandeliers."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.stats = None
        self.data = None
        self.current_chart = None
        self.equity_df = None  # Pour stocker les données d'équité brutes
        self.time_unit = DataGranularity.D  # Période par défaut (jour)
        self.candle_series = None  # Pour stocker la référence à la série de chandeliers
    
    def create(self):
        """Crée le widget principal pour le graphique en chandeliers."""
        self.main_tab = QWidget()
        main_layout = QVBoxLayout(self.main_tab)
        
        # Créer le conteneur pour le graphique
        self.chart_container = QWidget()
        self.chart_layout = QVBoxLayout(self.chart_container)
        self.chart_layout.setContentsMargins(0, 0, 0, 0)
        
        # Placeholder pour le graphique (sera remplacé lors de l'exécution)
        self.chart_placeholder = QLabel("Exécutez le backtest pour afficher le graphique d'équité")
        self.chart_placeholder.setAlignment(Qt.AlignCenter)
        self.chart_layout.addWidget(self.chart_placeholder)
        
        main_layout.addWidget(self.chart_container)
        
        return self.main_tab

    def update(self, data=None, stats=None):
        """Met à jour le graphique avec les nouvelles données."""
        if stats is None:
            return
            
        self.stats = stats
        self.data = data
        
        # Préparer les données d'équité brutes
        self.prepare_equity_data()
        
        # Créer ou mettre à jour le graphique
        self.update_chart()
    
    def prepare_equity_data(self):
        """Prépare les données d'équité brutes à partir des trades."""
        # Vérifier si on a des données de trades
        if self.stats is None or '_trades' not in self.stats:
            return
        
        # Récupérer les données
        trades = self.stats['_trades'].copy()
        
        # Vérifier si les trades sont vides
        if trades.empty:
            self.equity_df = None
            return
        
        # Récupérer les paramètres généraux pour obtenir le capital initial
        general_params = self.parent.general_params_panel.get_values()
        initial_equity = general_params['cash']
        
        # Créer une série temporelle complète pour l'équité
        if self.data is not None:
            # Utiliser toutes les dates des données de prix
            all_dates = pd.DataFrame({'time': self.data['time']})
        else:
            # Si pas de données de prix, utiliser seulement les dates des trades
            min_date = trades['EntryTime'].min()
            max_date = trades['ExitTime'].max()
            all_dates = pd.DataFrame({'time': pd.date_range(min_date, max_date, freq='D')})
        
        # Créer une liste pour stocker les transactions cumulatives
        equity_transactions = []
        
        # Ajouter le cash initial
        equity_transactions.append({
            'time': trades['EntryTime'].min() - pd.Timedelta(days=1),  # Jour avant le premier trade
            'amount': initial_equity
        })
        
        # Ajouter toutes les transactions
        for _, trade in trades.iterrows():
            equity_transactions.append({
                'time': trade['ExitTime'],
                'amount': trade['PnL']  # on ajoute le P&L à chaque sortie de trade
            })
        
        # Convertir en DataFrame
        equity_trans_df = pd.DataFrame(equity_transactions)
        equity_trans_df = equity_trans_df.sort_values('time')
        
        # Calculer l'équité cumulative
        equity_trans_df['cumulative'] = equity_trans_df['amount'].cumsum()
        
        # Fusionner avec toutes les dates
        equity_df = pd.merge_asof(
            all_dates.sort_values('time'), 
            equity_trans_df.sort_values('time'), 
            on='time', 
            direction='backward'
        )
        
        equity_df['cumulative'] = equity_df['cumulative'].ffill()
        
        # Stocker les données d'équité brutes
        self.equity_df = equity_df
    
    def resample_equity_data(self, granularity: DataGranularity):
        """Resampler les données d'équité brutes selon la fréquence demandée."""
        if self.equity_df is None:
            return None
        
        # Utiliser la valeur de l'énumération pour le resampling
        freq = granularity.value
        
        # Copier les données d'équité
        df = self.equity_df.copy()
        
        # S'assurer que la colonne time est au format datetime
        df['time'] = pd.to_datetime(df['time'])

        # Identifier les jours où il y a des données réelles (pas des valeurs remplies)
        # On sauvegarde les dates avant le resampling
        original_dates = set(df['time'].dt.strftime('%Y-%m-%d'))
        
        # Définir la colonne time comme index pour faciliter le resampling
        df.set_index('time', inplace=True)
        
        # Resampler les données selon la fréquence demandée
        resampled = df['cumulative'].resample(freq).ohlc()
        
        # Réinitialiser l'index pour avoir time comme colonne
        resampled.reset_index(inplace=True)
        
        # Remplir les valeurs manquantes (NaN) si nécessaire
        resampled = resampled.ffill()
        
        # Préparer le format pour lightweight-charts et filtrer les week-ends sans données
        ohlc_data = []
        for _, row in resampled.iterrows():
            # Vérifier que la ligne contient des données valides
            if pd.notna(row['time']) and pd.notna(row['open']) and pd.notna(row['high']) and pd.notna(row['low']) and pd.notna(row['close']):
                # Pour le niveau jour, vérifier si c'est une date avec des données réelles
                date_str = row['time'].strftime('%Y-%m-%d')
                
                # Si c'est une période journalière, on filtre les dates sans données
                if granularity == DataGranularity.D:
                    # Vérifier si cette date existe dans les données originales
                    if date_str in original_dates:
                        ohlc_data.append({
                            'time': date_str,
                            'open': float(row['open']),
                            'high': float(row['high']),
                            'low': float(row['low']),
                            'close': float(row['close'])
                        })
                else:
                    # Pour les autres périodes, on garde toutes les données
                    ohlc_data.append({
                        'time': date_str,
                        'open': float(row['open']),
                        'high': float(row['high']),
                        'low': float(row['low']),
                        'close': float(row['close'])
                    })
        
        # Si aucune donnée valide, retourner un DataFrame vide avec les colonnes appropriées
        if not ohlc_data:
            return pd.DataFrame(columns=['time', 'open', 'high', 'low', 'close'])
        
        return pd.DataFrame(ohlc_data)
    
    def on_timeframe_change(self, chart: QtChart):
        """Callback pour le changement d'unité de temps dans la topbar."""
        period_str = chart.topbar['period'].value
        
        # Convertir la période sélectionnée en valeur d'énumération
        period_mapping = {
            "Jour": DataGranularity.D,
            "Semaine": DataGranularity.W,
            "Mois": DataGranularity.M,
            "Trimestre": DataGranularity.Q,
            "Année": DataGranularity.Y
        }
        
        self.time_unit = period_mapping.get(period_str, DataGranularity.D)
        
        # Mettre à jour le graphique sans recréer tout
        if self.equity_df is not None and self.candle_series is not None:
            # Resampler les données
            ohlc_df = self.resample_equity_data(self.time_unit)
            
            # Mettre à jour les données
            self.candle_series.set(ohlc_df)
            
            # Ajuster l'affichage
            chart.fit()
    
    def update_chart(self):
        """Crée ou met à jour le graphique."""
        # Vérifier si on a des données d'équité
        if self.equity_df is None:
            self.clear_layout(self.chart_layout)
            self.chart_placeholder = QLabel("Aucun trade pour générer un graphique d'équité")
            self.chart_placeholder.setAlignment(Qt.AlignCenter)
            self.chart_layout.addWidget(self.chart_placeholder)
            return
        
        # Récupérer les paramètres généraux pour obtenir le capital initial
        general_params = self.parent.general_params_panel.get_values()
        initial_equity = general_params['cash']
        
        # Supprimer l'ancien graphique s'il existe
        self.clear_layout(self.chart_layout)
        
        # Création du conteneur pour le graphique
        chart_container = QWidget()
        chart_container.setMinimumHeight(300)  # Forcer une hauteur minimale
        chart_layout = QVBoxLayout(chart_container)
        chart_layout.setContentsMargins(0, 0, 0, 0)
        
        # Ajouter d'abord le conteneur au layout
        self.chart_layout.addWidget(chart_container)
        
        # Forcer le traitement des événements pour que le widget soit correctement dimensionné
        QApplication.processEvents()

        # Créer le graphique principal
        chart = QtChart(chart_container, toolbox=True)
        
        # Configurer l'apparence du graphique
        chart.layout(background_color='#f0f8ff', text_color='black')
        chart.grid(color='rgba(1,1,1,0.1)', vert_enabled=False, horz_enabled=False, style='solid')
        chart.price_scale(minimum_width=120, auto_scale=True, mode='normal', scale_margin_bottom=0.1, scale_margin_top=0.1)
        chart.time_scale(visible=True, seconds_visible=True, border_color='black', min_bar_spacing=0.0)
        chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        chart.legend(visible=True, color_based_on_candle=False, color='rgba(1,1,1,1)', font_size=12, font_family='Arial')
        
        # Définir les noms d'affichage correspondant à chaque granularité
        period_display_names = {
            DataGranularity.D: "Jour",
            DataGranularity.W: "Semaine",
            DataGranularity.M: "Mois", 
            DataGranularity.Q: "Trimestre",
            DataGranularity.Y: "Année"
        }
        
        # Récupérer le nom d'affichage pour la période actuelle
        current_period_display = period_display_names.get(self.time_unit, "Jour")
        
        # Ajouter le sélecteur de période dans la topbar
        chart.topbar.switcher('period', 
                              tuple(period_display_names.values()), 
                              default=current_period_display, 
                              func=self.on_timeframe_change)
        
        # Obtenir les données OHLC resamplées pour la période actuelle
        ohlc_df = self.resample_equity_data(self.time_unit)
        
        # Titre fixe pour le graphique d'équité
        chart_title = "Évolution de l'Équité"
        
        # Créer le candlestick chart
        self.candle_series = chart.create_custom_candle(
            name=chart_title,
            up_color='rgba(0, 150, 0, 0.8)',
            down_color='rgba(220, 0, 0, 0.8)',
            border_up_color='rgba(0, 150, 0, 1.0)',
            border_down_color='rgba(220, 0, 0, 1.0)',
            wick_up_color='rgba(0, 150, 0, 1.0)',
            wick_down_color='rgba(220, 0, 0, 1.0)'
        )

        self.candle_series.precision(2)
        
        # Ajouter l'equity initiale comme ligne horizontale
        self.candle_series.horizontal_line(
            price=initial_equity,
            color='rgba(0, 0, 0, 0.5)',
            width=1,
            style='dashed',
            text='Capital initial'
        )
        
        # Définir les données pour les chandeliers
        self.candle_series.set(ohlc_df)
        
        # Ajouter des marqueurs pour les trades
        trades = self.stats['_trades'].copy()
        for i, trade in trades.iterrows():
            entry_time = pd.to_datetime(trade['EntryTime'])
            exit_time = pd.to_datetime(trade['ExitTime'])
            
            entry_color = "blue" if trade['Size'] > 0 else "red"
            exit_color = "green" if trade['PnL'] > 0 else "red"
            
            # Entry marker
            chart.marker(
                time=entry_time,
                position="below",
                color=entry_color,
                text=f"Trade #{i+1}",
                shape="arrow_up",
                size=1)
                
            # Exit marker
            chart.marker(
                time=exit_time,
                position="above",
                color=exit_color,
                text=f"P/L: {trade['PnL']:.2f}$",
                shape="arrow_down",
                size=1)
        
        # Fit the chart to show all data
        chart.fit()
        
        # Ajouter le graphique au layout
        chart_layout.addWidget(chart.get_webview())
        
        # Stocker la référence au graphique
        self.current_chart = chart
        
        # Ajouter le conteneur du graphique au layout
        self.chart_layout.addWidget(chart_container)