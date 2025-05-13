import matplotlib.pyplot as plt
import numpy as np
from matplotlib.figure import Figure
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox
from PyQt5.QtCore import Qt

from backtestApp.views.base_view import ResultView

class HistogramView(ResultView):
    """Vue pour afficher l'histogramme des gains et pertes."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.stats = None
    
    def create(self):
        """Crée le widget principal pour l'histogramme."""
        self.histogram_tab = QWidget()
        histogram_layout = QVBoxLayout(self.histogram_tab)
        
        # Créer les contrôles pour sélectionner l'unité de temps
        controls_widget = QWidget()
        controls_layout = QHBoxLayout(controls_widget)
        controls_layout.setContentsMargins(0, 0, 0, 10)
        
        # Label pour l'unité de temps
        controls_layout.addWidget(QLabel("Unité de temps:"))
        
        # Combobox pour sélectionner l'unité de temps
        self.time_unit_combo = QComboBox()
        self.time_unit_combo.addItems(["Jour", "Semaine", "Mois", "Trimestre", "Année"])
        self.time_unit_combo.setCurrentIndex(0)  # Jour par défaut
        self.time_unit_combo.currentIndexChanged.connect(self.update_histogram)
        controls_layout.addWidget(self.time_unit_combo)
        
        controls_layout.addStretch()
        
        # Ajouter les contrôles au layout principal
        histogram_layout.addWidget(controls_widget)
        
        # Créer le widget pour le graphique matplotlib
        self.histogram_figure = Figure(figsize=(8, 6), dpi=100)
        self.histogram_canvas = FigureCanvas(self.histogram_figure)
        histogram_layout.addWidget(self.histogram_canvas)
        
        # Message placeholder initial
        self.histogram_figure.clear()
        ax = self.histogram_figure.add_subplot(111)
        ax.text(0.5, 0.5, "Exécutez le backtest pour afficher l'histogramme des gains/pertes", 
                horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
        ax.axis('off')
        self.histogram_canvas.draw()
        
        return self.histogram_tab
    
    def update(self, data=None, stats=None):
        """Met à jour l'histogramme avec les nouvelles données."""
        if stats is None:
            return
            
        self.stats = stats
        self.update_histogram()
    
    def update_histogram(self):
        """Met à jour l'histogramme en fonction de l'unité de temps sélectionnée."""
        # Vérifier si on a des données de trades
        if self.stats is None or '_trades' not in self.stats:
            return
        
        # Récupérer les données
        trades = self.stats['_trades'].copy()
        
        # Vérifier si les trades sont vides
        if trades.empty:
            self.histogram_figure.clear()
            ax = self.histogram_figure.add_subplot(111)
            ax.text(0.5, 0.5, "Aucun trade pour générer un histogramme", 
                    horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            ax.axis('off')
            self.histogram_canvas.draw()
            return
        
        # Définir le regroupement en fonction de l'unité de temps sélectionnée
        unit = self.time_unit_combo.currentText()
        if unit == "Jour":
            # Regrouper par jour
            trades['TimeGroup'] = trades['ExitTime'].dt.date
            title = "Gains et pertes quotidiens"
        elif unit == "Semaine":
            # Regrouper par semaine (année-numéro de semaine)
            trades['TimeGroup'] = trades['ExitTime'].dt.to_period('W').apply(lambda x: x.start_time.date())
            title = "Gains et pertes hebdomadaires"
        elif unit == "Mois":
            # Regrouper par mois (année-mois)
            trades['TimeGroup'] = trades['ExitTime'].dt.to_period('M').apply(lambda x: x.start_time.date())
            title = "Gains et pertes mensuels"
        elif unit == "Trimestre":
            # Regrouper par trimestre (année-trimestre)
            trades['TimeGroup'] = trades['ExitTime'].dt.to_period('Q').apply(lambda x: x.start_time.date())
            title = "Gains et pertes trimestriels"
        else:  # Année
            # Regrouper par année
            trades['TimeGroup'] = trades['ExitTime'].dt.year
            title = "Gains et pertes annuels"
        
        # Regrouper les PnL par la période définie
        grouped = trades.groupby('TimeGroup')['PnL'].sum().reset_index()
        grouped = grouped.sort_values('TimeGroup')
        
        # Créer l'histogramme
        self.histogram_figure.clear()
        ax = self.histogram_figure.add_subplot(111)
        
        # Définir les couleurs pour les barres positives et négatives
        colors = ['green' if pnl >= 0 else 'red' for pnl in grouped['PnL']]
        
        # Créer l'histogramme avec les barres colorées
        bars = ax.bar(grouped['TimeGroup'].astype(str), grouped['PnL'], color=colors)
        
        # Ajouter des annotations pour les valeurs sur chaque barre
        for bar in bars:
            height = bar.get_height()
            value = height if height > 0 else bar.get_y() + height  # position y différente si négatif
            sign = '+' if height > 0 else ''
            ax.annotate(f'{sign}{height:.2f}',
                        xy=(bar.get_x() + bar.get_width() / 2, value),
                        xytext=(0, 3 if height > 0 else -12),  # 3 points au-dessus ou 12 points en-dessous
                        textcoords="offset points",
                        ha='center', va='bottom' if height > 0 else 'top',
                        fontsize=8)
        
        # Configurer les axes et les titres
        ax.set_title(title)
        ax.set_xlabel('Période')
        ax.set_ylabel('Profit/Perte ($)')
        
        # Ajuster l'axe y pour avoir le zéro au milieu si nécessaire
        max_abs_y = max(abs(grouped['PnL'].max()) if not grouped['PnL'].empty and not np.isnan(grouped['PnL'].max()) else 1, 
                        abs(grouped['PnL'].min()) if not grouped['PnL'].empty and not np.isnan(grouped['PnL'].min()) else 1)
        ax.set_ylim(-max_abs_y * 1.1, max_abs_y * 1.1)  # 10% de marge
        
        # Ajouter une ligne horizontale à zéro
        ax.axhline(y=0, color='black', linestyle='-', alpha=0.3)
        
        # Rotation des étiquettes de l'axe x pour une meilleure lisibilité
        plt.setp(ax.get_xticklabels(), rotation=45, ha='right')
        
        # Ajuster la mise en page
        self.histogram_figure.tight_layout()
        
        # Rafraîchir le canvas
        self.histogram_canvas.draw()