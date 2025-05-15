from PyQt5.QtWidgets import QWidget, QVBoxLayout, QTabWidget
import logging
from backtestApp.views.chart_view import ChartView
from backtestApp.views.stats_view import StatsView
from backtestApp.views.histogram_view_candle import HistogramViewCandle
from backtestApp.views.histogram_view import HistogramView
import time

class ResultManager:
    """Gestionnaire des vues de résultats du backtest."""
    
    def __init__(self, parent=None):
        """Initialise le gestionnaire avec une référence à l'application parent."""
        self.parent = parent
        self.views = {}
        
    def create(self):
        """Crée la zone d'affichage des résultats avec les différentes vues."""
        self.results_widget = QWidget()
        self.results_layout = QVBoxLayout(self.results_widget)
        
        # Créer les onglets pour séparer les différentes vues
        self.tab_widget = QTabWidget()
        
        # Initialiser les vues
        self.views['chart'] = ChartView(self.parent)
        self.views['stats'] = StatsView(self.parent)
        self.views['histogram'] = HistogramView(self.parent)
        # self.views['histogram_candle'] = HistogramViewCandle(self.parent)
        
        # Ajouter les vues aux onglets
        self.tab_widget.addTab(self.views['chart'].create(), "📈 Graphiques")
        self.tab_widget.addTab(self.views['stats'].create(), "📊 Statistiques")
        self.tab_widget.addTab(self.views['histogram'].create(), "📊 Histogramme PnL")
        # self.tab_widget.addTab(self.parent.histogram_view_candle.create(), "📊 Histogramme Bougies")
        
        # Ajouter le widget d'onglets au layout principal
        self.results_layout.addWidget(self.tab_widget)
        
        return self.results_widget
    
    def update_all(self, data=None, stats=None):
        """Met à jour toutes les vues avec les nouvelles données."""
        start = time.time()
        for name, view in self.views.items():
            start_time = time.time()
            view.update(data, stats)
            end_time = time.time()
            logging.info(f"Mise à jour de {name}: {(end_time - start_time) * 1000:.2f} ms")

        end = time.time()
        logging.info(f"Mise à jour de toutes les vues: {(end - start) * 1000:.2f} ms")
    
    def set_current_tab(self, index):
        """Définit l'onglet actif."""
        if 0 <= index < self.tab_widget.count():
            self.tab_widget.setCurrentIndex(index)
            
    def get_tab_widget(self):
        """Retourne le widget d'onglets."""
        return self.tab_widget