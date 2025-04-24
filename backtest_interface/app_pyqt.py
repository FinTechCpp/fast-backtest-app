import sys
import logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
import os
import pandas as pd
from datetime import datetime, time
from PyQt5.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget, 
                            QPushButton, QComboBox, QDoubleSpinBox, QDateEdit, QLineEdit, 
                            QCheckBox, QLabel, QTabWidget, QScrollArea, QSplitter, QTableWidget, 
                            QTableWidgetItem, QGroupBox, QGridLayout, QFormLayout, QSpinBox,
                            QFrame, QSizePolicy)
from PyQt5.QtCore import Qt, QDate
from PyQt5.QtGui import QFont, QColor
from dotenv import load_dotenv

from lightweight_charts.widgets import QtChart

# Import your strategy classes and helpers
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA  
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.BuyHeikinGreen import BuyHeikinGreenBA
from igtrader.Strategies.Helpers import load_data, to_heikin_ashi
from igtrader.backtestingpy.backtesting.backtesting import Backtest

# Configure logging
load_dotenv()

class MetricWidget(QFrame):
    """Widget pour afficher une métrique avec un titre, une valeur et une variation optionnelle"""
    def __init__(self, title, value="", delta="", delta_color="normal", parent=None):
        super().__init__(parent)
        self.setFrameShape(QFrame.StyledPanel)
        self.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Titre
        self.title_label = QLabel(title)
        title_font = QFont()
        title_font.setPointSize(10)
        self.title_label.setFont(title_font)
        
        # Valeur
        self.value_label = QLabel(value)
        value_font = QFont()
        value_font.setPointSize(12)
        value_font.setBold(True)
        self.value_label.setFont(value_font)
        
        # Delta
        self.delta_label = QLabel(delta)
        delta_font = QFont()
        delta_font.setPointSize(10)
        self.delta_label.setFont(delta_font)
        if delta_color == "normal":
            self.delta_label.setStyleSheet("color: green")
        elif delta_color == "inverse":
            self.delta_label.setStyleSheet("color: red")
        
        layout.addWidget(self.title_label)
        layout.addWidget(self.value_label)
        if delta:
            layout.addWidget(self.delta_label)
        
        self.setLayout(layout)
    
    def update_values(self, value, delta="", delta_color="normal"):
        self.value_label.setText(value)
        if delta:
            self.delta_label.setText(delta)
            if delta_color == "normal":
                self.delta_label.setStyleSheet("color: green")
            elif delta_color == "inverse":
                self.delta_label.setStyleSheet("color: red")


class BacktestApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Backtest Dashboard")
        self.resize(1900, 1200)
        
        # Initialisation des variables
        self.strategy_map = {
            'BuyHeikinGreenBA': BuyHeikinGreenBA,
            'BuyTrendFollowingBA': BuyTrendFollowingBA,
            'SellTrendFollowingBA': SellTrendFollowingBA,
        }
        self.default_indicators = {}
        self.current_chart = None
        
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
    
    def create_control_panel(self):
        """Crée le panneau de contrôle (équivalent de la sidebar Streamlit)"""
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
        
        # Bouton de lancement du backtest
        self.run_button = QPushButton("Lancer le backtest")
        self.run_button.setMinimumHeight(40)
        layout.addWidget(self.run_button)
        
        # Paramètres généraux
        params_group = QGroupBox("Paramètres généraux")
        params_layout = QFormLayout()
        
        # Symbole
        self.symbol_combo = QComboBox()
        self.symbol_combo.addItems(["NDX", "SPX", "EURUSD"])
        params_layout.addRow(QLabel("Symbole:"), self.symbol_combo)
        
        # Période
        self.period_combo = QComboBox()
        self.period_combo.addItems(["5d", "10d", "30d", "1m", "3m", "6m", "1y", "2y"])
        self.period_combo.setCurrentIndex(1)
        params_layout.addRow(QLabel("Période de données:"), self.period_combo)
        
        # Intervalle
        self.interval_combo = QComboBox()
        self.interval_combo.addItems(["10secs", "20secs", "30secs", "1min", "2min", "3min", "5min", "10min", "15min", "30min", "1h", "2h", "4h", "1d"])
        self.interval_combo.setCurrentIndex(1)
        params_layout.addRow(QLabel("Intervalle:"), self.interval_combo)
        
        # Date de fin
        self.end_date = QDateEdit()
        self.end_date.setDate(QDate(2025, 2, 10))
        self.end_date.setCalendarPopup(True)
        params_layout.addRow(QLabel("Date de fin:"), self.end_date)
        
        # Fuseau horaire
        self.timezone = QLineEdit("Europe/Paris")
        params_layout.addRow(QLabel("Fuseau horaire:"), self.timezone)
        
        # Spread
        self.spread = QDoubleSpinBox()
        self.spread.setDecimals(4)
        self.spread.setRange(0, 1)
        self.spread.setSingleStep(0.0001)
        self.spread.setValue(0.0002)
        params_layout.addRow(QLabel("Spread:"), self.spread)
        
        # Cash initial
        self.cash = QDoubleSpinBox()
        self.cash.setDecimals(2)
        self.cash.setRange(0, 10000000)
        self.cash.setSingleStep(1000)
        self.cash.setValue(100000)
        params_layout.addRow(QLabel("Cash initial:"), self.cash)
        
        # Stratégie
        self.strategy_combo = QComboBox()
        self.strategy_combo.addItems(list(self.strategy_map.keys()))
        params_layout.addRow(QLabel("Stratégie:"), self.strategy_combo)
        
        # Type de bougie
        self.candle_type_combo = QComboBox()
        self.candle_type_combo.addItems(["Heikin Ashi", "Standard"])
        params_layout.addRow(QLabel("Type de bougie:"), self.candle_type_combo)
        
        # Stop Loss
        self.stop_loss = QDoubleSpinBox()
        self.stop_loss.setDecimals(2)
        self.stop_loss.setRange(0, 1000)
        self.stop_loss.setSingleStep(1)
        self.stop_loss.setValue(20)
        params_layout.addRow(QLabel("Stop Loss [pts]:"), self.stop_loss)
        
        # Take Profit
        self.take_profit = QDoubleSpinBox()
        self.take_profit.setDecimals(2)
        self.take_profit.setRange(0, 1000)
        self.take_profit.setSingleStep(1)
        self.take_profit.setValue(30)
        params_layout.addRow(QLabel("Take Profit [pts]:"), self.take_profit)
        
        # Sauvegarder les résultats
        self.save_results = QCheckBox("Sauvegarder les résultats")
        params_layout.addRow("", self.save_results)
        
        params_group.setLayout(params_layout)
        layout.addWidget(params_group)
        
        # Configuration des indicateurs
        indicators_group = QGroupBox("Configuration des indicateurs")
        indicators_layout = QVBoxLayout()
        
        # EMA
        self.ema_short_check = QCheckBox("Activer EMA court")
        self.ema_short_check.setChecked(True)
        self.ema_short_spin = QSpinBox()
        self.ema_short_spin.setRange(1, 500)
        self.ema_short_spin.setValue(20)
        
        self.ema_medium_check = QCheckBox("Activer EMA moyen")
        self.ema_medium_check.setChecked(True)
        self.ema_medium_spin = QSpinBox()
        self.ema_medium_spin.setRange(1, 500)
        self.ema_medium_spin.setValue(50)
        
        self.ema_long_check = QCheckBox("Activer EMA long")
        self.ema_long_check.setChecked(True)
        self.ema_long_spin = QSpinBox()
        self.ema_long_spin.setRange(1, 500)
        self.ema_long_spin.setValue(200)
        
        ema_layout = QGridLayout()
        ema_layout.addWidget(self.ema_short_check, 0, 0)
        ema_layout.addWidget(QLabel("Période:"), 0, 1)
        ema_layout.addWidget(self.ema_short_spin, 0, 2)
        ema_layout.addWidget(self.ema_medium_check, 1, 0)
        ema_layout.addWidget(QLabel("Période:"), 1, 1)
        ema_layout.addWidget(self.ema_medium_spin, 1, 2)
        ema_layout.addWidget(self.ema_long_check, 2, 0)
        ema_layout.addWidget(QLabel("Période:"), 2, 1)
        ema_layout.addWidget(self.ema_long_spin, 2, 2)
        
        indicators_layout.addLayout(ema_layout)
        
        # ATR
        atr_widget = QWidget()
        atr_layout = QHBoxLayout(atr_widget)
        atr_layout.setContentsMargins(0, 0, 0, 0)
        
        self.atr_check = QCheckBox("Activer ATR")
        self.atr_check.setChecked(True)
        self.atr_period_spin = QSpinBox()
        self.atr_period_spin.setRange(1, 100)
        self.atr_period_spin.setValue(14)
        
        atr_layout.addWidget(self.atr_check)
        atr_layout.addWidget(QLabel("Période:"))
        atr_layout.addWidget(self.atr_period_spin)
        atr_layout.addStretch()
        
        indicators_layout.addWidget(atr_widget)
        
        # Stochastic
        stoch_group = QGroupBox("Stochastic")
        stoch_layout = QGridLayout()
        
        self.stoch_check = QCheckBox("Activer Stochastic")
        self.stoch_check.setChecked(True)
        self.fastk_spin = QSpinBox()
        self.fastk_spin.setRange(1, 100)
        self.fastk_spin.setValue(10)
        self.slowk_spin = QSpinBox()
        self.slowk_spin.setRange(1, 100)
        self.slowk_spin.setValue(10)
        self.slowd_spin = QSpinBox()
        self.slowd_spin.setRange(1, 100)
        self.slowd_spin.setValue(3)
        
        stoch_layout.addWidget(self.stoch_check, 0, 0, 1, 3)
        stoch_layout.addWidget(QLabel("Fast %K:"), 1, 0)
        stoch_layout.addWidget(self.fastk_spin, 1, 1)
        stoch_layout.addWidget(QLabel("Slow %K:"), 2, 0)
        stoch_layout.addWidget(self.slowk_spin, 2, 1)
        stoch_layout.addWidget(QLabel("Slow %D:"), 3, 0)
        stoch_layout.addWidget(self.slowd_spin, 3, 1)
        
        stoch_group.setLayout(stoch_layout)
        indicators_layout.addWidget(stoch_group)
             
        # SuperTrend
        st_group = QGroupBox("SuperTrend")
        st_layout = QGridLayout()
        
        self.supertrend_check = QCheckBox("Activer SuperTrend")
        self.st_atr_period_spin = QSpinBox()
        self.st_atr_period_spin.setRange(1, 500)
        self.st_atr_period_spin.setValue(100)
        self.st_multiplier_spin = QSpinBox()
        self.st_multiplier_spin.setRange(1, 200)
        self.st_multiplier_spin.setValue(50)
        
        st_layout.addWidget(self.supertrend_check, 0, 0, 1, 3)
        st_layout.addWidget(QLabel("Période ATR:"), 1, 0)
        st_layout.addWidget(self.st_atr_period_spin, 1, 1)
        st_layout.addWidget(QLabel("Multiplicateur:"), 2, 0)
        st_layout.addWidget(self.st_multiplier_spin, 2, 1)
        
        st_group.setLayout(st_layout)
        indicators_layout.addWidget(st_group)
        
        indicators_group.setLayout(indicators_layout)
        layout.addWidget(indicators_group)
        
        # Ajouter le scroll area pour le panneau de contrôle
        scroll_area = QScrollArea()
        scroll_area.setWidget(control_panel)
        scroll_area.setWidgetResizable(True)
        self.splitter.addWidget(scroll_area)
        
        # Trading hours
        trading_hours_group = QGroupBox("Heures de trading")
        trading_hours_layout = QGridLayout()
        
        # Trading start time
        self.trading_from_hour = QSpinBox()
        self.trading_from_hour.setRange(0, 23)
        self.trading_from_hour.setValue(9)
        self.trading_from_minute = QSpinBox()
        self.trading_from_minute.setRange(0, 59)
        self.trading_from_minute.setValue(30)
        from_layout = QHBoxLayout()
        from_layout.addWidget(self.trading_from_hour)
        from_layout.addWidget(QLabel(":"))
        from_layout.addWidget(self.trading_from_minute)
        
        # Trading end time
        self.trading_to_hour = QSpinBox()
        self.trading_to_hour.setRange(0, 23)
        self.trading_to_hour.setValue(16)
        self.trading_to_minute = QSpinBox()
        self.trading_to_minute.setRange(0, 59)
        self.trading_to_minute.setValue(0)
        to_layout = QHBoxLayout()
        to_layout.addWidget(self.trading_to_hour)
        to_layout.addWidget(QLabel(":"))
        to_layout.addWidget(self.trading_to_minute)
        
        # Trading days (checkboxes for each day)
        days_layout = QHBoxLayout()
        self.trading_days_check = []
        days = ["Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"]
        for i, day in enumerate(days):
            check = QCheckBox(day)
            check.setChecked(i < 5)  # Check Mon-Fri by default
            self.trading_days_check.append(check)
            days_layout.addWidget(check)
        
        trading_hours_layout.addWidget(QLabel("De:"), 0, 0)
        trading_hours_layout.addLayout(from_layout, 0, 1)
        trading_hours_layout.addWidget(QLabel("À:"), 1, 0)
        trading_hours_layout.addLayout(to_layout, 1, 1)
        trading_hours_layout.addWidget(QLabel("Jours:"), 2, 0)
        trading_hours_layout.addLayout(days_layout, 2, 1)
        
        trading_hours_group.setLayout(trading_hours_layout)
        layout.addWidget(trading_hours_group)
    
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
        
        # État des widgets en fonction des checkboxes
        self.ema_short_check.toggled.connect(lambda checked: self.ema_short_spin.setEnabled(checked))
        self.ema_medium_check.toggled.connect(lambda checked: self.ema_medium_spin.setEnabled(checked))
        self.ema_long_check.toggled.connect(lambda checked: self.ema_long_spin.setEnabled(checked))
        self.atr_check.toggled.connect(lambda checked: self.atr_period_spin.setEnabled(checked))
        self.stoch_check.toggled.connect(self.toggle_stoch_widgets)
        self.supertrend_check.toggled.connect(self.toggle_supertrend_widgets)
    
    def toggle_stoch_widgets(self, checked):
        """Active/désactive les widgets Stochastic en fonction de la checkbox"""
        self.fastk_spin.setEnabled(checked)
        self.slowk_spin.setEnabled(checked)
        self.slowd_spin.setEnabled(checked)
    
    def toggle_supertrend_widgets(self, checked):
        """Active/désactive les widgets SuperTrend en fonction de la checkbox"""
        self.st_atr_period_spin.setEnabled(checked)
        self.st_multiplier_spin.setEnabled(checked)
    
    def get_indicator_config(self):
        """Récupère la configuration des indicateurs à partir des widgets"""
        indicators = {}
        
        # EMA
        ema_periods = []
        if self.ema_short_check.isChecked():
            ema_periods.append([self.ema_short_spin.value()])
        if self.ema_medium_check.isChecked():
            ema_periods.append([self.ema_medium_spin.value()])
        if self.ema_long_check.isChecked():
            ema_periods.append([self.ema_long_spin.value()])
        
        if ema_periods:
            indicators['EMA'] = ema_periods
        
        # ATR
        if self.atr_check.isChecked():
            indicators['ATR'] = [[self.atr_period_spin.value()]]
        
        # Stochastic
        if self.stoch_check.isChecked():
            indicators['STOCH'] = [[
                self.fastk_spin.value(),
                self.slowk_spin.value(),
                self.slowd_spin.value()
            ]]
        
        # SuperTrend
        if self.supertrend_check.isChecked():
            indicators['SUPERTREND'] = [[
                self.st_atr_period_spin.value(),
                self.st_multiplier_spin.value()
            ]]
        
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
        perf_layout.addWidget(MetricWidget("Capital final [$]", f"${stats['Equity Final [$]']:,.2f}"), 0, 1)
        perf_layout.addWidget(MetricWidget("Capital max [$]", f"${stats['Equity Peak [$]']:,.2f}"), 1, 1)
        perf_layout.addWidget(MetricWidget("Buy & Hold [%]", f"{stats['Buy & Hold Return [%]']:.2f}%"), 2, 1)
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
        chart = QtChart(chart_container, toolbox=True, inner_height=0.6)
        
        # Configurer l'apparence du graphique
        chart.layout(background_color='#f0f8ff', text_color='black')
        chart.grid(color='rgba(1,1,1,0.1)', vert_enabled=False, horz_enabled=False, style='solid')
        chart.price_scale(minimum_width=120, auto_scale=True, mode='normal', scale_margin_bottom=0.1, scale_margin_top=0.1)
        chart.time_scale(visible=True, seconds_visible=True, border_color='black', min_bar_spacing=0.0)
        chart.crosshair(mode='normal', vert_visible=True, horz_visible=True)
        chart.legend(visible=True, color_based_on_candle=False, color='rgba(1,1,1,1)', font_size=12, font_family='Arial')
        
        # Définir les données sur le graphique
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
            
            # Add the main equity line
            equity_line = equity_chart.create_line(name='Equity', color='rgba(20,20,180,1)', width=1, price_line=False)
            equity_line.horizontal_line(price=100, color='black', width=1, style='dashed', text='Initial Equity')
            
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
                stoch_chart = chart.create_subchart(height=0.15, width=1, position="bottom", sync=True)
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
                atr_chart = chart.create_subchart(height=0.15, width=1, position="bottom", sync=True)
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
                    price=entry_price
                )
                
                # Exit marker
                chart.marker(
                    time=exit_time,
                    position="below",
                    color=exit_color,
                    text=f"Exit: {exit_price:.2f} (P/L: {trade['PnL']:.2f})",
                    shape="arrow_up",
                    price=exit_price
                )
            
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
            # Récupérer les paramètres du backtest
            symbol = self.symbol_combo.currentText()
            period = self.period_combo.currentText()
            interval = self.interval_combo.currentText()
            end_date = self.end_date.date().toString("dd/MM/yyyy")
            timezone = self.timezone.text()
            spread = self.spread.value()
            cash = self.cash.value()
            strategy_name = self.strategy_combo.currentText()
            candle_type = self.candle_type_combo.currentText()
            stop_loss = self.stop_loss.value()
            take_profit = self.take_profit.value()
            
            # Récupérer la configuration des indicateurs
            indicators = self.get_indicator_config()
            logging.debug(f"Using indicators: {indicators}")
            
            # Charger les données
            logging.debug("Chargement des données...")
            data = load_data(
                symbol=symbol,
                period=period,
                interval=interval,
                end_date=end_date,
                timezone=timezone,
                indicators=indicators,
                candle_type=candle_type
            )
            
            # Vérifier la stratégie
            if strategy_name not in self.strategy_map:
                logging.error(f"Strategy {strategy_name} not found. Available strategies: {list(self.strategy_map.keys())}")
                return
            
            # Get trading hours from UI
            trading_from = time(
                self.trading_from_hour.value(),
                self.trading_from_minute.value()
            )
            trading_to = time(
                self.trading_to_hour.value(),
                self.trading_to_minute.value()
            )

            # Get trading days from UI
            trading_days = [i for i, check in enumerate(self.trading_days_check) if check.isChecked()]
            
            # Préparer les arguments pour la stratégie
            strategy_kwargs = {
                "trading_from": trading_from,
                "trading_to": trading_to,
                "trading_days": trading_days,
                "take_profit_distance": take_profit,
                "stop_loss_distance": stop_loss,
                
                "ema_short_period": self.ema_short_spin.value(),
                "ema_long_period": self.ema_long_spin.value(),
                "stoch_fastk": self.fastk_spin.value(),
                "stoch_slowk": self.slowk_spin.value(),
                "stoch_slowd": self.slowd_spin.value(),
            }
            logging.debug(f"Stop Loss: {stop_loss}, Take Profit: {take_profit}")
            
            # Configurer les indicateurs attendus par la stratégie
            if "EMA" in indicators:
                # Trouver l'EMA court, moyen et long
                ema_periods = sorted([params[0] for params in indicators["EMA"]])
                if len(ema_periods) >= 2:
                    strategy_kwargs["ema_short_name"] = f"EMA_{ema_periods[-2]}"  # Avant-dernier (moyen)
                    strategy_kwargs["ema_long_name"] = f"EMA_{ema_periods[-1]}"   # Dernier (long)
            
            if "SUPERTREND" in indicators:
                # Utiliser le premier SuperTrend de la configuration
                atr_period, multiplier = indicators["SUPERTREND"][0]
                strategy_kwargs["supertrend_name"] = f"SUPERTREND_{atr_period}_{multiplier}"
            
            if "STOCH" in indicators:
                # Utiliser le premier Stochastic de la configuration
                fastk, slowk, slowd = indicators["STOCH"][0]
                strategy_kwargs["stoch_k_name"] = f"STOCH_K_{fastk}_{slowk}_{slowd}"
                strategy_kwargs["stoch_d_name"] = f"STOCH_D_{fastk}_{slowk}_{slowd}"
            
            logging.info(f"Utilisant les indicateurs pour la stratégie: {strategy_kwargs}")
            strategy = self.strategy_map[strategy_name]
            
            # Exécuter le backtest
            bt = Backtest(data, strategy, cash=cash, commission=.00, spread=spread, 
                        exclusive_orders=False, strategy_kwargs=strategy_kwargs)
            stats = bt.run()
            
            # Traiter les résultats
            stats['_trades']['EntryTime'] = stats['_trades']['EntryTime'].dt.tz_convert(timezone).dt.tz_localize(None)
            stats['_trades']['ExitTime'] = stats['_trades']['ExitTime'].dt.tz_convert(timezone).dt.tz_localize(None)
            stats['_equity_curve'].index = stats['_equity_curve'].index.tz_convert(timezone).tz_localize(None)
            stats['_equity_curve']['Equity'] = stats['_equity_curve']['Equity'] / 1000
            
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
            
            # Afficher le graphique et les statistiques
            self.setup_chart(chart_data, stats)
            self.create_stats_widgets(stats)
            
            # Basculer vers l'onglet des graphiques
            self.tab_widget.setCurrentIndex(0)
            
            logging.info("Backtest exécuté avec succès")
            
        except Exception as e:
            logging.exception(f"Erreur lors de l'exécution du backtest: {str(e)}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = BacktestApp()
    window.show()
    sys.exit(app.exec_())