# Python utils imports
import logging
import pandas as pd
from datetime import datetime, time
from dotenv import load_dotenv
load_dotenv()

#Qt imports
from PyQt5.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget, 
                            QPushButton, QComboBox, QDoubleSpinBox, QDateEdit, QLineEdit, 
                            QCheckBox, QLabel, QTabWidget, QScrollArea, QSplitter, QTableWidget, 
                            QTableWidgetItem, QGroupBox, QGridLayout, QFormLayout, QSpinBox, QProgressBar, QToolBar)
from PyQt5.QtCore import Qt, QDate, QTimer
from PyQt5.QtGui import QFont, QColor

# Lightweight Charts imports
from lightweight_charts_esistjosh.widgets import QtChart

# Local imports
from config_manager import ConfigManager
from metric_widget import MetricWidget
from ui_util import to_heikin_ashi, BacktestWorker

# backtest_backend imports
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA  
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.BuyHeikinGreen import BuyHeikinGreenBA
from igtrader.Strategies.Helpers import load_data

class BacktestApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Backtest Dashboard")
        self.resize(2100, 800)
        
        # Initialisation des variables
        self.strategy_map = {
            'BuyHeikinGreenBA': BuyHeikinGreenBA,
            'BuyTrendFollowingBA': BuyTrendFollowingBA,
            'SellTrendFollowingBA': SellTrendFollowingBA,
        }
        self.default_indicators = {}
        self.current_chart = None
        
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
        
        # Connecter les signaux
        self.connect_signals()
        
        # Charger le profil DEFAULT
        self.config_manager.apply_profile_to_ui("DEFAULT", self)
    
    def load_selected_profile(self, profile_name):
        """Charge le profil sélectionné dans le menu déroulant."""
        if profile_name != self.config_manager.current_profile:
            success = self.config_manager.apply_profile_to_ui(profile_name, self)
            if not success:
                # En cas d'échec, rétablir le profil précédent dans le combobox
                index = self.profile_combo.findText(self.config_manager.current_profile)
                if index >= 0:
                    self.profile_combo.setCurrentIndex(index)
    
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
        from panels import ProfilePanel
        self.profile_panel = ProfilePanel(self)
        layout.addWidget(self.profile_panel.create())
        
        # Bouton de lancement du backtest avec indicateur de chargement
        button_layout = QHBoxLayout()
        
        self.run_button = QPushButton("Lancer le backtest")
        self.run_button.setMinimumHeight(40)
        self.run_button.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;")
        
        self.loading_indicator = QProgressBar()
        self.loading_indicator.setMaximum(0)  # Mode indéterminé
        self.loading_indicator.setMinimum(0)
        self.loading_indicator.setTextVisible(False)  # Pas de texte de pourcentage
        self.loading_indicator.setMaximumHeight(10)
        self.loading_indicator.setVisible(False)  # Caché par défaut
        
        button_layout.addWidget(self.run_button)
        button_layout.addWidget(self.loading_indicator)
        layout.addLayout(button_layout)
        

        #Integration du panel de paramètres généraux
        from panels import GeneralParamsPanel
        self.general_params_panel = GeneralParamsPanel(self)
        layout.addWidget(self.general_params_panel.create())


        # Panel de base de la stratégie (commun à toutes les stratégies)
        from panels import StrategyBasePanel
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
        self.general_params_panel.widgets['strategy_combo'].currentTextChanged.connect(
            self.update_strategy_specific_panel
        )


        
        # # ATR
        # atr_widget = QWidget()
        # atr_layout = QHBoxLayout(atr_widget)
        # atr_layout.setContentsMargins(0, 0, 0, 0)
        
        # self.atr_check = QCheckBox("Activer ATR")
        # self.atr_check.setChecked(True)
        # self.atr_period_spin = QSpinBox()
        # self.atr_period_spin.setRange(1, 100)
        # self.atr_period_spin.setValue(14)
        
        # atr_layout.addWidget(self.atr_check)
        # atr_layout.addWidget(QLabel("Période:"))
        # atr_layout.addWidget(self.atr_period_spin)
        # atr_layout.addStretch()
        
        # indicators_layout.addWidget(atr_widget)
        
    

        
    

    def update_strategy_specific_panel(self):
        """Met à jour le panel spécifique en fonction de la stratégie sélectionnée."""
        # Obtenir la stratégie sélectionnée
        selected_strategy = self.general_params_panel.widgets['strategy_combo'].currentText()
        
        # Supprimer l'ancien panel spécifique s'il existe
        if self.strategy_specific_panel:
            old_panel_widget = self.strategy_specific_panel.create().parent()
            if old_panel_widget:
                old_panel_widget.setParent(None)
                old_panel_widget.deleteLater()
        
        # Créer le nouveau panel spécifique
        from panels import STRATEGY_PANELS
        if selected_strategy in STRATEGY_PANELS:
            panel_class = STRATEGY_PANELS[selected_strategy]
            self.strategy_specific_panel = panel_class(self)
            
            # Utiliser directement la référence au layout stockée
            if hasattr(self, 'control_panel_layout'):
                self.control_panel_layout.addWidget(self.strategy_specific_panel.create())
            else:
                # Fallback à l'ancienne méthode (ne devrait plus être nécessaire)
                try:
                    control_panel = self.splitter.widget(0).findChild(QScrollArea).widget()
                    if control_panel:
                        layout = control_panel.layout()
                        if layout:
                            layout.addWidget(self.strategy_specific_panel.create())
                        else:
                            logging.error("Le layout du panneau de contrôle est None")
                    else:
                        logging.error("Le widget du panneau de contrôle est None")
                except Exception as e:
                    logging.error(f"Erreur lors de la mise à jour du panel spécifique: {str(e)}")

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
        self.results_widget = QWidget()
        self.results_layout = QVBoxLayout(self.results_widget)
        
        # Créer les onglets pour séparer les graphiques et les statistiques
        self.tab_widget = QTabWidget()
        
        # Onglet des graphiques
        self.charts_tab = QWidget()
        self.charts_layout = QVBoxLayout(self.charts_tab)
        
        # Placeholder pour le graphique (sera remplacé lors de l'exécution)
        self.chart_placeholder = QLabel("Exécutez le backtest pour afficher les graphiques")
        self.chart_placeholder.setAlignment(Qt.AlignCenter)
        self.charts_layout.addWidget(self.chart_placeholder)
        
        # Onglet des statistiques
        self.stats_tab = QWidget()
        self.stats_layout = QVBoxLayout(self.stats_tab)
        self.scroll_stats = QScrollArea()
        self.scroll_stats.setWidgetResizable(True)
        self.stats_content = QWidget()
        self.stats_content_layout = QVBoxLayout(self.stats_content)
        
        self.stats_placeholder = QLabel("Exécutez le backtest pour afficher les statistiques")
        self.stats_placeholder.setAlignment(Qt.AlignCenter)
        self.stats_content_layout.addWidget(self.stats_placeholder)
        
        self.scroll_stats.setWidget(self.stats_content)
        self.stats_layout.addWidget(self.scroll_stats)
        
        # Ajouter les onglets au widget d'onglets
        self.tab_widget.addTab(self.charts_tab, "📈 Graphiques")
        self.tab_widget.addTab(self.stats_tab, "📊 Statistiques")
        
        self.results_layout.addWidget(self.tab_widget)
        
        # Ajouter la zone de résultats au splitter
        self.splitter.addWidget(self.results_widget)
        
        # Définir les tailles initiales du splitter
        self.splitter.setSizes([400, 1200])  # 400px pour le panneau de contrôle, le reste pour les résultats
    
    def connect_signals(self):
        """Connecter les signaux des widgets aux fonctions correspondantes"""
        self.run_button.clicked.connect(self.run_backtest)
        
        # # État des widgets en fonction des checkboxes
        # self.ema_short_check.toggled.connect(lambda checked: self.ema_short_spin.setEnabled(checked))
        # self.ema_medium_check.toggled.connect(lambda checked: self.ema_medium_spin.setEnabled(checked))
        # self.ema_long_check.toggled.connect(lambda checked: self.ema_long_spin.setEnabled(checked))
        # self.atr_check.toggled.connect(lambda checked: self.atr_period_spin.setEnabled(checked))
        # self.stoch_check.toggled.connect(self.toggle_stoch_widgets)
        # self.supertrend_check.toggled.connect(self.toggle_supertrend_widgets)
        # self.use_atr_check.toggled.connect(self.toggle_atr_controls)
        
    def toggle_atr_controls(self, checked):
        """Active ou désactive les contrôles pour les paramètres ATR"""
        self.sl_atr_multiplier.setEnabled(checked)
        self.tp_atr_multiplier.setEnabled(checked)
        self.min_sl.setEnabled(checked)
        self.min_tp.setEnabled(checked)
        # Désactiver les contrôles classiques de SL/TP quand ATR est activé
        self.stop_loss.setEnabled(not checked)
        self.take_profit.setEnabled(not checked)
        
    def toggle_risk_controls(self, checked):
        """Active ou désactive les contrôles pour le risk-based sizing"""
        self.risk_percentage.setEnabled(checked)
    
    def toggle_stoch_widgets(self, checked):
        """Active/désactive les widgets Stochastic en fonction de la checkbox"""
        self.fastk_spin.setEnabled(checked)
        self.slowk_spin.setEnabled(checked)
        self.slowd_spin.setEnabled(checked)
        self.stoch_threshold_spin.setEnabled(checked)  # Ajouter cette ligne
    
    def toggle_supertrend_widgets(self, checked):
        """Active/désactive les widgets SuperTrend en fonction de la checkbox"""
        self.st_atr_period_spin.setEnabled(checked)
        self.st_multiplier_spin.setEnabled(checked)
    
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
        if 'ema_medium_check' in widgets and widgets['ema_medium_check'].isChecked():
            ema_periods.append([values.get('ema_medium_period', 50)])
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
        
        # SuperTrend
        if 'supertrend_check' in widgets and widgets['supertrend_check'].isChecked():
            indicators['SUPERTREND'] = [[
                values.get('st_atr_period', 14),
                values.get('st_multiplier', 3.0)
            ]]
        
        # Ajouter des logs pour le débogage
        logging.debug(f"Indicateurs configurés: {indicators}")
        
        return indicators
    
    def clear_layout(self, layout):
        """Supprime tous les widgets d'un layout"""
        while layout.count():
            item = layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()
            elif item.layout() is not None:  # Check if item.layout() is not None
                self.clear_layout(item.layout())
            # If neither widget nor layout, just continue
    
    def create_stats_widgets(self, stats):
        """Crée les widgets pour afficher les statistiques du backtest"""
        # Supprimer les anciens widgets
        self.clear_layout(self.stats_content_layout)
        
        # Ajouter un titre
        title = QLabel("Statistiques du backtest")
        title.setStyleSheet("font-size: 18px; font-weight: bold; color: #0066cc;")
        self.stats_content_layout.addWidget(title)
        
        # Créer les sections pour les statistiques
        main_stats_widget = QWidget()
        main_stats_layout = QHBoxLayout(main_stats_widget)
        
        # Colonne 1: Période et Performance
        col1_widget = QWidget()
        col1_layout = QVBoxLayout(col1_widget)
        
        # Période
        period_group = QGroupBox("⏱️ Période")
        period_layout = QGridLayout()
        
        # Date de début et durée
        period_layout.addWidget(MetricWidget("Date de début", stats["Start"].strftime("%Y-%m-%d %H:%M")), 0, 0)
        period_layout.addWidget(MetricWidget("Durée", str(stats["Duration"])), 1, 0)
        
        # Date de fin et exposition
        period_layout.addWidget(MetricWidget("Date de fin", stats["End"].strftime("%Y-%m-%d %H:%M")), 0, 1)
        period_layout.addWidget(MetricWidget("Exposition [%]", f"{stats['Exposure Time [%]']:.2f}%"), 1, 1)
        
        period_group.setLayout(period_layout)
        col1_layout.addWidget(period_group)
        
        # Performance
        perf_group = QGroupBox("📊 Performance")
        perf_layout = QGridLayout()
        
        # Rendement et rendement annualisé
        delta = f"{stats['Return [%]'] - stats['Buy & Hold Return [%]']:.2f}% vs B&H"
        perf_layout.addWidget(MetricWidget("Rendement [%]", f"{stats['Return [%]']:.2f}%", delta), 0, 0)
        perf_layout.addWidget(MetricWidget("Rendement annualisé [%]", f"{stats['Return (Ann.) [%]']:.2f}%"), 1, 0)
        perf_layout.addWidget(MetricWidget("Volatilité annualisée [%]", f"{stats['Volatility (Ann.) [%]']:.2f}%"), 2, 0)
        perf_layout.addWidget(MetricWidget("CAGR [%]", f"{stats['CAGR [%]']:.2f}%"), 3, 0)
        
        # Capital final, max et Buy & Hold
        perf_layout.addWidget(MetricWidget("Capital final [$]", f"${stats['Equity Final [$]']:,.3f}"), 0, 1)
        perf_layout.addWidget(MetricWidget("Capital max [$]", f"${stats['Equity Peak [$]']:,.3f}"), 1, 1)
        perf_layout.addWidget(MetricWidget("Buy & Hold [%]", f"{stats['Buy & Hold Return [%]']:.3f}%"), 2, 1)
        perf_layout.addWidget(MetricWidget("Alpha [%]", f"{stats['Alpha [%]']:.4f}%"), 3, 1)
        
        perf_group.setLayout(perf_layout)
        col1_layout.addWidget(perf_group)
        
        main_stats_layout.addWidget(col1_widget)
        
        # Colonne 2: Métriques de risque et statistiques des trades
        col2_widget = QWidget()
        col2_layout = QVBoxLayout(col2_widget)
        
        # Métriques de risque
        risk_group = QGroupBox("⚠️ Métriques de Risque")
        risk_layout = QGridLayout()
        
        sharpe_color = "normal" if stats['Sharpe Ratio'] >= 0 else "inverse"
        risk_layout.addWidget(MetricWidget("Sharpe Ratio", f"{stats['Sharpe Ratio']:.2f}", delta_color=sharpe_color), 0, 0)
        risk_layout.addWidget(MetricWidget("Sortino Ratio", f"{stats['Sortino Ratio']:.2f}"), 1, 0)
        risk_layout.addWidget(MetricWidget("Drawdown max [%]", f"{stats['Max. Drawdown [%]']:.2f}%", delta_color="inverse"), 2, 0)
        risk_layout.addWidget(MetricWidget("Drawdown moyen [%]", f"{stats['Avg. Drawdown [%]']:.2f}%", delta_color="inverse"), 3, 0)
        
        risk_layout.addWidget(MetricWidget("Calmar Ratio", f"{stats['Calmar Ratio']:.2f}"), 0, 1)
        risk_layout.addWidget(MetricWidget("Beta", f"{stats['Beta']:.4f}"), 1, 1)
        risk_layout.addWidget(MetricWidget("Durée max drawdown", str(stats["Max. Drawdown Duration"])), 2, 1)
        risk_layout.addWidget(MetricWidget("Durée moyenne drawdown", str(stats["Avg. Drawdown Duration"])), 3, 1)
        
        risk_group.setLayout(risk_layout)
        col2_layout.addWidget(risk_group)
        
        # Statistiques des trades
        trade_group = QGroupBox("💹 Statistiques des Trades")
        trade_layout = QGridLayout()
        
        trade_layout.addWidget(MetricWidget("Nombre de trades", f"{stats['# Trades']}"), 0, 0)
        trade_layout.addWidget(MetricWidget("Taux de réussite [%]", f"{stats['Win Rate [%]']:.2f}%"), 1, 0)
        trade_layout.addWidget(MetricWidget("Meilleur trade [%]", f"{stats['Best Trade [%]']:.2f}%"), 2, 0)
        trade_layout.addWidget(MetricWidget("Pire trade [%]", f"{stats['Worst Trade [%]']:.2f}%"), 3, 0)
        
        trade_layout.addWidget(MetricWidget("Trade moyen [%]", f"{stats['Avg. Trade [%]']:.4f}%"), 0, 1)
        trade_layout.addWidget(MetricWidget("Durée max trade", str(stats["Max. Trade Duration"])), 1, 1)
        trade_layout.addWidget(MetricWidget("Durée moyenne trade", str(stats["Avg. Trade Duration"])), 2, 1)
        trade_layout.addWidget(MetricWidget("Profit Factor", f"{stats['Profit Factor']:.2f}"), 3, 1)
        
        trade_group.setLayout(trade_layout)
        col2_layout.addWidget(trade_group)
        
        main_stats_layout.addWidget(col2_widget)
        self.stats_content_layout.addWidget(main_stats_widget)
        
        # Métriques avancées
        adv_group = QGroupBox("Métriques avancées")
        adv_layout = QGridLayout()
        
        adv_layout.addWidget(MetricWidget("Expectancy [%]", f"{stats['Expectancy [%]']:.4f}%"), 0, 0)
        adv_layout.addWidget(MetricWidget("SQN", f"{stats['SQN']:.4f}"), 0, 1)
        adv_layout.addWidget(MetricWidget("Kelly Criterion", f"{stats['Kelly Criterion']:.4f}"), 0, 2)
        
        adv_group.setLayout(adv_layout)
        self.stats_content_layout.addWidget(adv_group)
        
        # Historique des trades
        trades_label = QLabel("Historique des trades")
        trades_label.setStyleSheet("font-weight: bold; color: #0066cc;")
        self.stats_content_layout.addWidget(trades_label)
        
        trades_table = QTableWidget()
        trades = stats['_trades'].iloc[:, :12]
        
        trades_table.setRowCount(len(trades))
        trades_table.setColumnCount(len(trades.columns))
        trades_table.setHorizontalHeaderLabels(list(trades.columns))
        
        for i, (_, row) in enumerate(trades.iterrows()):
            for j, value in enumerate(row):
                item = QTableWidgetItem(str(value))
                if j == trades.columns.get_loc('PnL') and float(value) > 0:
                    item.setForeground(QColor('green'))
                elif j == trades.columns.get_loc('PnL') and float(value) < 0:
                    item.setForeground(QColor('red'))
                trades_table.setItem(i, j, item)
        trades_table.setMinimumHeight(500)
        trades_table.resizeColumnsToContents()
        self.stats_content_layout.addWidget(trades_table)
        
        # Statistiques de performance
        perf_label = QLabel("Statistiques de performance")
        perf_label.setStyleSheet("font-weight: bold; color: #0066cc;")
        self.stats_content_layout.addWidget(perf_label)
        
        equity_table = QTableWidget()
        equity_curve = stats['_equity_curve'].copy()
        
        equity_table.setRowCount(len(equity_curve))
        equity_table.setColumnCount(len(equity_curve.columns))
        equity_table.setHorizontalHeaderLabels(list(equity_curve.columns))
        
        for i, (index, row) in enumerate(equity_curve.iterrows()):
            # Ajouter l'index (date) comme première colonne
            index_item = QTableWidgetItem(str(index))
            equity_table.setItem(i, 0, index_item)
            
            # Ajouter les autres valeurs
            for j, value in enumerate(row):
                item = QTableWidgetItem(str(value))
                equity_table.setItem(i, j + 1, item)  # +1 pour tenir compte de la colonne d'index
        equity_table.setMinimumHeight(500)
        equity_table.resizeColumnsToContents()
        self.stats_content_layout.addWidget(equity_table)
        
        # Ajouter un espacement en bas
        self.stats_content_layout.addStretch()
    
    def setup_chart(self, data, stats=None):
        """Configure et affiche le graphique principal et les sous-graphiques"""
        # Supprimer l'ancien graphique s'il existe
        self.clear_layout(self.charts_layout)
        
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
        
        general_params = self.general_params_panel.get_values()

        if general_params['candle_type'] == "Heikin Ashi":
            data = to_heikin_ashi(data)
        chart.set(data)
            
        # Ajouter le graphique au layout
        chart_layout.addWidget(chart.get_webview())
        
        # Stocker la référence au graphique
        self.current_chart = chart
        
        # Si des statistiques sont fournies, ajouter les indicateurs et les trades
        if stats is not None:
            # EQUITY SUBCHART
            equity_chart = chart.create_subchart(height=0.1, width=1, position="top", sync=True)
            equity_chart.layout(background_color='#f0f8ff')
            equity_chart.grid(color='lightgray', vert_enabled=False, horz_enabled=False, style='solid')
            equity_chart.time_scale(visible=False, min_bar_spacing=0.0)  # Hide time scale for equity curve
            equity_chart.price_scale(minimum_width=120)
            equity_chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
            
            # Prepare equity data
            equity_df = stats['_equity_curve'].copy()
            equity_df.reset_index(inplace=True)
            
            # Handle possible column names - check what's actually there
            if 'index' in equity_df.columns:
                equity_df.rename(columns={'index': 'time'}, inplace=True)
            elif 'date' in equity_df.columns:
                equity_df.rename(columns={'date': 'time'}, inplace=True)
            elif equity_df.index.name is None:
                equity_df.rename(columns={equity_df.columns[0]: 'time'}, inplace=True)
            
            general_params = self.general_params_panel.get_values()

            # Add the main equity line
            equity_line = equity_chart.create_line(name='Equity', color='rgba(20,20,180,1)', width=1, price_line=False)
            equity_line.horizontal_line(price=general_params['cash'], color='black', width=1, style='dashed', text='Initial Equity')
            
            # Convert time to string and handle Timedelta objects
            equity_df['time'] = equity_df['time'].astype(str)
            for column in equity_df.columns:
                if pd.api.types.is_timedelta64_dtype(equity_df[column]):
                    equity_df[column] = equity_df[column].dt.total_seconds()
                elif isinstance(equity_df[column].iloc[0], pd.Timedelta):
                    equity_df[column] = equity_df[column].apply(lambda x: x.total_seconds() if isinstance(x, pd.Timedelta) else x)
            
            # Set the data for the equity line
            equity_line.set(equity_df)
            
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
            
            # Add trade markers
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
                                
                # Exit marker
                chart.marker(
                    time=exit_time,
                    position="below",
                    color=exit_color,
                    text=f"Exit: {exit_price:.2f}\n"
                         f"(P/L: {trade['PnL']:.2f} $ / {round(trade['ReturnPct'], 5)}%)\n",
                    shape="arrow_up",
                    size=1)
            
            # Fit the chart to show all data
            chart.fit()
            
            chart.create_synchronized_tooltip(charts=[equity_chart, atr_chart, stoch_chart], options={
                "backgroundColor": "rgba(255, 255, 255, 0.9)",
                "textColor": "#333",
                "padding": "8px"}, 
                trigger_key="Shift",
                toggle_mode=False)
        
        # Ajouter le conteneur du graphique au layout
        self.charts_layout.addWidget(chart_container)
        
    def run_backtest(self):
        """Exécute le backtest avec les paramètres sélectionnés et met à jour l'interface"""
        try:
            self.loading_indicator.setVisible(True)
            self.run_button.setEnabled(False)
            self.run_button.setText("Exécution du backtest en cours...")
            QApplication.processEvents()
            
            # Récupérer tous les paramètres de configuration
            config = self.get_strategy_config()

            print(config)

            # # Récupérer les paramètres du backtest
            # symbol = general_params['symbol']
            # period = general_params['period']
            # interval = general_params['interval']
            # end_date = general_params['end_date']
            # timezone = general_params['timezone']
            # spread = general_params['spread']
            # cash = general_params['cash']
            # strategy_name = general_params['strategy']
            # stop_loss = general_params['stop_loss']
            # take_profit = general_params['take_profit']
            
            
            # Charger les données (potentiellement long aussi, mais gérable)
            logging.debug("Chargement des données...")
            data = load_data(
                symbol=config['symbol'],
                period=config['period'],
                interval=config['interval'],
                end_date=config['end_date'],
                timezone=config['timezone'],
                indicators=self.get_indicator_config())
            
            # Vérifier la stratégie
            strategy_name = config['strategy']
            if strategy_name not in self.strategy_map:
                logging.error(f"Strategy {strategy_name} not found. Available strategies: {list(self.strategy_map.keys())}")
                return
            
            # # Get trading hours from UI
            # trading_from = time(
            #     self.trading_from_hour.value(),
            #     self.trading_from_minute.value()
            # )
            # trading_to = time(
            #     self.trading_to_hour.value(),
            #     self.trading_to_minute.value()
            # )

            # Get trading days from UI
            # trading_days = [i for i, check in enumerate(self.trading_days_check) if check.isChecked()]
            
            # # Préparer les arguments pour la stratégie
            # strategy_kwargs = {
            #     "trading_from": trading_from,
            #     "trading_to": trading_to,
            #     "trading_days": trading_days,
            #     "take_profit_distance": take_profit,
            #     "stop_loss_distance": stop_loss,
                
            #     # ATR parameters
            #     "use_atr_for_sl_tp": self.use_atr_check.isChecked(),
            #     "atr_period": self.atr_period_spin.value(),
            #     "stop_loss_atr_multiplier": self.sl_atr_multiplier.value(),
            #     "take_profit_atr_multiplier": self.tp_atr_multiplier.value(),
            #     "min_stop_loss_distance": self.min_sl.value(),
            #     "min_take_profit_distance": self.min_tp.value(),
                
            #     "ema_short_period": self.ema_short_spin.value(),
            #     "ema_long_period": self.ema_long_spin.value(),
            #     "stoch_fastk": self.fastk_spin.value(),
            #     "stoch_slowk": self.slowk_spin.value(),
            #     "stoch_slowd": self.slowd_spin.value(),
            #     "stoch_threshold": self.stoch_threshold_spin.value(),

            #     # Activation des filtres
            #     "use_ema_short_filter": self.ema_short_filter_check.isChecked(),
            #     "use_ema_long_filter": self.ema_long_filter_check.isChecked(),
            #     "use_stoch_filter": self.stoch_filter_check.isChecked(),
            #     "use_previous_ha_candle_red_filter": self.previous_ha_candle_red_check.isChecked(),
                
            #     # Paramètres de gestion du risque
            #     "use_risk_based_sizing": self.use_risk_based_sizing.isChecked(),
            #     "risk_percentage": self.risk_percentage.value(),
            #     "risk_capital": cash,
            #     "break_even_threshold": self.break_even_threshold.value(),
            #     "maximal_leverage": self.maximal_leverage.value(),
            # }
                       

            
            
            # Définir l'onglet à afficher à la fin de l'exécution du backtest
            # 0 = Graphiques, 1 = Statistiques
            self.tab_widget.setCurrentIndex(1)
            
            # Créer et exécuter le thread de backtest
            self.backtest_thread = BacktestWorker(
                data=data,
                strategy=self.strategy_map[strategy_name],
                cash=config['cash'],
                spread=config['spread'],
                strategy_kwargs=config,
            )
            
            # Connecter les signaux
            self.backtest_thread.finished.connect(self.on_backtest_finished)
            self.backtest_thread.error.connect(self.on_backtest_error)
            
            # Démarrer le thread
            self.backtest_thread.start()
            
        except Exception as e:
            logging.exception(f"Erreur lors de l'exécution du backtest: {str(e)}")
            self.loading_indicator.setVisible(False)
            self.run_button.setEnabled(True)
            self.run_button.setText("Lancer le backtest")
            
    def on_backtest_finished(self, data, stats):
        """Fonction appelée lorsque le backtest est terminé avec succès"""
        try:
            general_params = self.general_params_panel.get_values()
            timezone = general_params['timezone']
            
            # Traiter les résultats
            stats['_trades']['EntryTime'] = stats['_trades']['EntryTime'].dt.tz_convert(timezone).dt.tz_localize(None)
            stats['_trades']['ExitTime'] = stats['_trades']['ExitTime'].dt.tz_convert(timezone).dt.tz_localize(None)
            stats['_equity_curve'].index = stats['_equity_curve'].index.tz_convert(timezone).tz_localize(None)
            
            # Afficher les statistiques (généralement plus léger)
            self.create_stats_widgets(stats)
            
            # Préparer les données pour le graphique
            chart_data = data.reset_index()
            
            # Renommer les colonnes pour lightweight-charts
            if 'index' in chart_data.columns:
                chart_data.rename(columns={'index': 'time'}, inplace=True)
            elif 'date' in chart_data.columns:
                chart_data.rename(columns={'date': 'time'}, inplace=True)
            
            # Convertir les noms des colonnes OHLC
            column_map = {
                'Open': 'open',
                'High': 'high',
                'Low': 'low',
                'Close': 'close',
                'Volume': 'volume'
            }
            chart_data.rename(columns=column_map, inplace=True)
            
            # Traiter la colonne de temps
            chart_data['time'] = pd.to_datetime(chart_data['time'])
            if chart_data['time'].dt.tz is None:
                chart_data['time'] = chart_data['time'].dt.tz_localize(timezone)
            chart_data['time'] = chart_data['time'].dt.tz_convert(timezone).dt.tz_localize(None)
            
            # Trier les données
            chart_data.sort_values('time', inplace=True)

            self.setup_chart(chart_data, stats=stats)    
            
            logging.info("Backtest exécuté avec succès")
        
        finally:
            self.loading_indicator.setVisible(False)
            self.run_button.setEnabled(True)
            self.run_button.setText("Lancer le backtest")

    def on_backtest_error(self, error_msg):
        """Fonction appelée en cas d'erreur pendant le backtest"""
        logging.error(f"Erreur pendant le backtest: {error_msg}")
        self.loading_indicator.setVisible(False)
        self.run_button.setEnabled(True)
        self.run_button.setText("Lancer le backtest")
