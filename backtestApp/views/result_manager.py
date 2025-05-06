from PyQt5.QtWidgets import QWidget, QVBoxLayout, QTabWidget

from views.chart_view import ChartView
from views.stats_view import StatsView
from views.histogram_view import HistogramView

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
        
        # Ajouter les vues aux onglets
        self.tab_widget.addTab(self.views['chart'].create(), "📈 Graphiques")
        self.tab_widget.addTab(self.views['stats'].create(), "📊 Statistiques")
        self.tab_widget.addTab(self.views['histogram'].create(), "📊 Histogramme PnL")
        
        # Ajouter le widget d'onglets au layout principal
        self.results_layout.addWidget(self.tab_widget)
        
        return self.results_widget
    
    def update_all(self, data=None, stats=None):
        """Met à jour toutes les vues avec les nouvelles données."""
        for view in self.views.values():
            view.update(data, stats)
    
    def set_current_tab(self, index):
        """Définit l'onglet actif."""
        if 0 <= index < self.tab_widget.count():
            self.tab_widget.setCurrentIndex(index)
            
    def get_tab_widget(self):
        """Retourne le widget d'onglets."""
        return self.tab_widget