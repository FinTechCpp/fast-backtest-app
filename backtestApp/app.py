# Python utils imports
import logging
from dotenv import load_dotenv
load_dotenv()

#Qt imports
from PyQt5.QtWidgets import (QMainWindow, QVBoxLayout, QHBoxLayout, QWidget,
                            QLabel, QScrollArea, QSplitter)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont

# Local imports
from backtestApp.config_manager import ConfigManager

# backtest_backend imports
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA  
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.BuyHeikinGreen import BuyHeikinGreenBA
from igtrader.Strategies.CrossEMA import CrossEMABA
from igtrader.Strategies.SellHeikinRed import SellHeikinRedBA

from backtestApp.views.result_manager import ResultManager

class BacktestApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Backtest Dashboard")
        self.resize(2100, 800)
        
        # Initialisation des variables
        self.strategy_map = {
            'BuyHeikinGreenBA': BuyHeikinGreenBA,
            'SellHeikinRedBA': SellHeikinRedBA,
            'BuyTrendFollowingBA': BuyTrendFollowingBA,
            'SellTrendFollowingBA': SellTrendFollowingBA,
            'CrossEMABA': CrossEMABA
        }
        self.default_indicators = {}
        self.current_chart = None
        self.current_strategy_widget = None
        
        # Initialiser le gestionnaire de configuration
        self.config_manager = ConfigManager()
        
        # Créer le layout principal
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QHBoxLayout(self.central_widget)
        
        # Créer le splitter entre le panneau de contrôle et les résultats
        self.splitter = QSplitter(Qt.Horizontal)
        self.main_layout.addWidget(self.splitter)
        
        # Créer et configurer le panneau de contrôle (sidebar)
        self.create_control_panel()
        
        # Créer et configurer la zone de résultats
        self.create_results_area()
        
        # Charger le profil DEFAULT
        self.config_manager.apply_profile_to_ui("DEFAULT", self)

    
    # Modifier la méthode create_control_panel pour ajouter la section des profils
    def create_control_panel(self):
        """Crée le panneau de contrôle avec des panels modulaires"""
        control_panel = QWidget()
        control_panel.setMaximumWidth(400)
        layout = QVBoxLayout(control_panel)
        layout.setSpacing(10)
        
        # Titre
        title_label = QLabel("Configuration du backtest")
        title_font = QFont()
        title_font.setPointSize(14)
        title_font.setBold(True)
        title_label.setFont(title_font)
        layout.addWidget(title_label)
        
        # Integration du panel de profils
        from backtestApp.panels import ProfilePanel
        self.profile_panel = ProfilePanel(self)
        layout.addWidget(self.profile_panel.create())
        
        # Utiliser la classe BacktestRunner pour gérer l'exécution du backtest
        from backtestApp.components.backtest_runner import BacktestRunner
        self.backtest_runner = BacktestRunner(self)
        layout.addLayout(self.backtest_runner.get_layout())
        
        # Integration du panel de paramètres généraux
        from backtestApp.panels import GeneralParamsPanel
        self.general_params_panel = GeneralParamsPanel(self)
        layout.addWidget(self.general_params_panel.create())

        # Panel de base de la stratégie (commun à toutes les stratégies)
        from backtestApp.panels import StrategyBasePanel
        self.strategy_base_panel = StrategyBasePanel(self)
        layout.addWidget(self.strategy_base_panel.create())

        # Ajouter le scroll area pour le panneau de contrôle
        scroll_area = QScrollArea()
        scroll_area.setWidget(control_panel)
        scroll_area.setWidgetResizable(True)
        self.splitter.addWidget(scroll_area)

        # Stocker une référence au layout du control panel
        self.control_panel_layout = layout

        # Panel spécifique à la stratégie (dynamique)
        self.strategy_specific_panel = None
        self.update_strategy_specific_panel()
        
        # Connecter le changement de stratégie pour mettre à jour le panel spécifique
        self.general_params_panel.widgets['strategy'].currentTextChanged.connect(
            self.update_strategy_specific_panel
        )

    def update_strategy_specific_panel(self):
        """Met à jour le panel spécifique en fonction de la stratégie sélectionnée."""
        # Obtenir la stratégie sélectionnée
        selected_strategy = self.general_params_panel.widgets['strategy'].currentText()
        
        # Supprimer l'ancien panel spécifique s'il existe
        if hasattr(self, 'current_strategy_widget') and self.current_strategy_widget:
            # Supprimer le widget courant du layout
            self.control_panel_layout.removeWidget(self.current_strategy_widget)
            # Détruire proprement le widget
            self.current_strategy_widget.setParent(None)
            self.current_strategy_widget.deleteLater()
            self.current_strategy_widget = None
        
        # Créer le nouveau panel spécifique
        from backtestApp.panels import STRATEGY_PANELS
        if selected_strategy in STRATEGY_PANELS:
            try:
                # Créer une nouvelle instance du panel
                panel_class = STRATEGY_PANELS[selected_strategy]
                self.strategy_specific_panel = panel_class(self)
                
                # Créer le widget et le conserver dans un attribut
                new_widget = self.strategy_specific_panel.create()
                self.current_strategy_widget = new_widget
                
                # Ajouter le widget au layout
                if hasattr(self, 'control_panel_layout'):
                    self.control_panel_layout.addWidget(new_widget)
                else:
                    logging.error("control_panel_layout n'est pas défini")
            except Exception as e:
                logging.error(f"Erreur lors de la création du panel spécifique: {str(e)}")

    def get_strategy_config(self):
        """Récupère la configuration complète de la stratégie."""
        config = {}
        
        # Paramètres généraux
        config.update(self.general_params_panel.get_values())
        
        # Paramètres de base de la stratégie
        config.update(self.strategy_base_panel.get_values())
        
        # Paramètres spécifiques à la stratégie
        if self.strategy_specific_panel:
            config.update(self.strategy_specific_panel.get_values())
        
        return config
    
    def create_results_area(self):
        """Crée la zone d'affichage des résultats"""
        # Initialiser le gestionnaire de résultats
        self.result_manager = ResultManager(self)
        result_widget = self.result_manager.create()
        
        # Ajouter la zone de résultats au splitter
        self.splitter.addWidget(result_widget)
        
        # Définir les tailles initiales du splitter
        self.splitter.setSizes([400, 1200])  # 400px pour le panneau de contrôle, le reste pour les résultats
    
    def get_indicator_config(self):
        """Récupère la configuration des indicateurs à partir des panels"""
        indicators = {}
        
        # Vérifier si nous avons un panel spécifique actif
        if not hasattr(self, 'strategy_specific_panel') or self.strategy_specific_panel is None:
            return indicators
        
        # Récupérer les valeurs du panel spécifique
        values = self.strategy_specific_panel.get_values()
        widgets = self.strategy_specific_panel.widgets
        
        # EMA
        ema_periods = []
        if 'ema_short_check' in widgets and widgets['ema_short_check'].isChecked():
            ema_periods.append([values.get('ema_short_period', 20)])
        if 'ema_long_check' in widgets and widgets['ema_long_check'].isChecked():
            ema_periods.append([values.get('ema_long_period', 200)])
        
        if ema_periods:
            indicators['EMA'] = ema_periods
        
        # ATR
        if 'atr_check' in widgets and widgets['atr_check'].isChecked():
            indicators['ATR'] = [[values.get('atr_period', 14)]]
        
        # Stochastic
        if 'stoch_check' in widgets and widgets['stoch_check'].isChecked():
            indicators['STOCH'] = [[
                values.get('stoch_fastk', 10),
                values.get('stoch_slowk', 7),
                values.get('stoch_slowd', 3)
            ]]
        
        # Ajouter des logs pour le débogage
        logging.debug(f"Indicateurs configurés: {indicators}")
        
        return indicators
    
