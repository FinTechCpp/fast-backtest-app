from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QLabel, QScrollArea, QGroupBox, 
                           QHBoxLayout, QGridLayout, QTableView, QHeaderView, QPushButton,
                           QTableWidget, QTableWidgetItem, QComboBox, QStackedWidget)
from PyQt5.QtGui import QColor, QStandardItemModel, QStandardItem
from PyQt5.QtCore import Qt, QAbstractTableModel, QModelIndex, QVariant, pyqtSlot

import time
import pandas as pd
import numpy as np
from backtestApp.views.base_view import ResultView
from backtestApp.metric_widget import MetricWidget

class TradesTableModel(QAbstractTableModel):
    """Modèle de données optimisé pour la table des trades."""
    def __init__(self, data=None):
        super().__init__()
        self.dataframe = data if data is not None else pd.DataFrame()
        self.columns = list(self.dataframe.columns)
        # Ajouter une colonne pour l'index numéroté
        self.show_index = True
        
    def rowCount(self, parent=QModelIndex()):
        return len(self.dataframe)
    
    def columnCount(self, parent=QModelIndex()):
        return len(self.columns) + (1 if self.show_index else 0)
    
    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return QVariant()
        
        # Gestion de la colonne d'index numéroté
        if self.show_index and index.column() == 0:
            if role == Qt.DisplayRole:
                # Numéro de trade (base 1)
                return str(index.row() + 1)
            elif role == Qt.TextAlignmentRole:
                return Qt.AlignCenter
            return QVariant()
        
        # Décalage des colonnes pour tenir compte de l'index
        actual_column = index.column() - (1 if self.show_index else 0)
        
        if role == Qt.DisplayRole:
            value = self.dataframe.iloc[index.row(), actual_column]
            return str(value)
        
        if role == Qt.TextColorRole:
            # Colorer les profits et pertes
            col_name = self.columns[actual_column]
            if col_name == 'PnL':
                value = float(self.dataframe.iloc[index.row(), actual_column])
                if value > 0:
                    return QColor('green')
                elif value < 0:
                    return QColor('red')
        
        return QVariant()
    
    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            # Pour la colonne d'index
            if self.show_index and section == 0:
                return "N°"
            # Pour les autres colonnes
            actual_section = section - (1 if self.show_index else 0)
            if actual_section < len(self.columns):
                return self.columns[actual_section]
        return QVariant()
    
    def update_data(self, data):
        """Met à jour les données du modèle efficacement."""
        self.beginResetModel()
        self.dataframe = data
        self.columns = list(data.columns)
        self.endResetModel()


class StatsView(ResultView):
    """Vue optimisée pour afficher les statistiques du backtest."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.metric_widgets = {}  # Pour stocker les références aux widgets
        self.tables_created = False
        self.trades_model = TradesTableModel()
        self.equity_model = TradesTableModel()
        
    def create(self):
        """Crée le widget principal pour les statistiques."""
        start_time = time.time()
        
        self.stats_tab = QWidget()
        self.stats_layout = QVBoxLayout(self.stats_tab)
        
        self.scroll_stats = QScrollArea()
        self.scroll_stats.setWidgetResizable(True)
        
        self.stats_content = QWidget()
        self.stats_content_layout = QVBoxLayout(self.stats_content)
        
        self.stats_placeholder = QLabel("Exécutez le backtest pour afficher les statistiques")
        self.stats_placeholder.setAlignment(Qt.AlignCenter)
        self.stats_content_layout.addWidget(self.stats_placeholder)
        
        # Pré-créer la structure des widgets (sans données)
        self._create_stats_widgets()
        
        self.scroll_stats.setWidget(self.stats_content)
        self.stats_layout.addWidget(self.scroll_stats)
        
        print(f"StatsView création: {(time.time() - start_time) * 1000:.2f} ms")
        return self.stats_tab
    
    def _create_stats_widgets(self):
        """Pré-création de tous les widgets pour éviter de les recréer à chaque mise à jour."""
        # Titre
        self.title = QLabel("Statistiques du backtest")
        self.title.setStyleSheet("font-size: 18px; font-weight: bold; color: #0066cc;")
        self.title.setVisible(False)
        self.stats_content_layout.addWidget(self.title)
        
        # Conteneur principal
        self.main_stats_widget = QWidget()
        self.main_stats_layout = QHBoxLayout(self.main_stats_widget)
        self.main_stats_widget.setVisible(False)
        
        # Colonne 1
        self.col1_widget = QWidget()
        self.col1_layout = QVBoxLayout(self.col1_widget)
        
        # Période
        self.period_group = QGroupBox("⏱️ Période")
        self.period_layout = QGridLayout()
        self.period_group.setLayout(self.period_layout)
        
        # Créer les widgets de métriques pour la période
        self._create_metric_widget("Start", "Date de début", "", 0, 0, self.period_layout)
        self._create_metric_widget("Duration", "Durée", "", 1, 0, self.period_layout)
        self._create_metric_widget("End", "Date de fin", "", 0, 1, self.period_layout)
        self._create_metric_widget("Exposure", "Exposition [%]", "", 1, 1, self.period_layout)
        
        self.col1_layout.addWidget(self.period_group)
        
        # Performance
        self.perf_group = QGroupBox("📊 Performance")
        self.perf_layout = QGridLayout()
        self.perf_group.setLayout(self.perf_layout)
        
        # Créer les widgets de métriques pour la performance
        self._create_metric_widget("Return", "Rendement [%]", "", 0, 0, self.perf_layout)
        self._create_metric_widget("ReturnAnn", "Rendement annualisé [%]", "", 1, 0, self.perf_layout)
        self._create_metric_widget("Volatility", "Volatilité annualisée [%]", "", 2, 0, self.perf_layout)
        self._create_metric_widget("CAGR", "CAGR [%]", "", 3, 0, self.perf_layout)
        
        self._create_metric_widget("EquityFinal", "Capital final [$]", "", 0, 1, self.perf_layout)
        self._create_metric_widget("EquityPeak", "Capital max [$]", "", 1, 1, self.perf_layout)
        self._create_metric_widget("BuyHold", "Buy & Hold [%]", "", 2, 1, self.perf_layout)
        self._create_metric_widget("Alpha", "Alpha [%]", "", 3, 1, self.perf_layout)
        
        self.col1_layout.addWidget(self.perf_group)
        self.main_stats_layout.addWidget(self.col1_widget)
        
        # Colonne 2
        self.col2_widget = QWidget()
        self.col2_layout = QVBoxLayout(self.col2_widget)
        
        # Métriques de risque
        self.risk_group = QGroupBox("⚠️ Métriques de Risque")
        self.risk_layout = QGridLayout()
        self.risk_group.setLayout(self.risk_layout)
        
        # Créer les widgets de métriques pour le risque
        self._create_metric_widget("Sharpe", "Sharpe Ratio", "", 0, 0, self.risk_layout)
        self._create_metric_widget("Sortino", "Sortino Ratio", "", 1, 0, self.risk_layout)
        self._create_metric_widget("MaxDrawdown", "Drawdown max [%]", "", 2, 0, self.risk_layout)
        self._create_metric_widget("AvgDrawdown", "Drawdown moyen [%]", "", 3, 0, self.risk_layout)
        
        self._create_metric_widget("Calmar", "Calmar Ratio", "", 0, 1, self.risk_layout)
        self._create_metric_widget("Beta", "Beta", "", 1, 1, self.risk_layout)
        self._create_metric_widget("MaxDrawdownDuration", "Durée max drawdown", "", 2, 1, self.risk_layout)
        self._create_metric_widget("AvgDrawdownDuration", "Durée moyenne drawdown", "", 3, 1, self.risk_layout)
        
        self.col2_layout.addWidget(self.risk_group)
        
        # Statistiques des trades
        self.trade_group = QGroupBox("💹 Statistiques des Trades")
        self.trade_layout = QGridLayout()
        self.trade_group.setLayout(self.trade_layout)
        
        # Créer les widgets de métriques pour les trades
        self._create_metric_widget("NumTrades", "Nombre de trades", "", 0, 0, self.trade_layout)
        self._create_metric_widget("WinRate", "Taux de réussite [%]", "", 1, 0, self.trade_layout)
        
        self._create_metric_widget("WinningTrades", "Trades gagnants", "", 0, 1, self.trade_layout)
        self._create_metric_widget("LosingTrades", "Trades perdants", "", 1, 1, self.trade_layout)
        self._create_metric_widget("NeutralTrades", "Trades neutres", "", 2, 1, self.trade_layout)
        
        self._create_metric_widget("BestTrade", "Meilleur trade [%]", "", 4, 0, self.trade_layout)
        self._create_metric_widget("WorstTrade", "Pire trade [%]", "", 5, 0, self.trade_layout)
        
        self._create_metric_widget("AvgTrade", "Trade moyen [%]", "", 4, 1, self.trade_layout)
        self._create_metric_widget("MaxTradeDuration", "Durée max trade", "", 5, 1, self.trade_layout)
        self._create_metric_widget("AvgTradeDuration", "Durée moyenne trade", "", 6, 0, self.trade_layout)
        self._create_metric_widget("ProfitFactor", "Profit Factor", "", 6, 1, self.trade_layout)
        
        self.col2_layout.addWidget(self.trade_group)
        self.main_stats_layout.addWidget(self.col2_widget)
        
        # Ajouter le widget principal au conteneur
        self.stats_content_layout.addWidget(self.main_stats_widget)
        
        # Métriques avancées
        self.adv_group = QGroupBox("Métriques avancées")
        self.adv_layout = QGridLayout()
        self.adv_group.setLayout(self.adv_layout)
        self.adv_group.setVisible(False)
        
        # Créer les widgets de métriques avancées
        self._create_metric_widget("Expectancy", "Expectancy [%]", "", 0, 0, self.adv_layout)
        self._create_metric_widget("SQN", "SQN", "", 0, 1, self.adv_layout)
        self._create_metric_widget("Kelly", "Kelly Criterion", "", 0, 2, self.adv_layout)
        
        self.stats_content_layout.addWidget(self.adv_group)
        
        # Conteneurs pour les tableaux
        self.tables_container = QWidget()
        self.tables_layout = QVBoxLayout(self.tables_container)
        self.tables_container.setVisible(False)
        
        # Étiquettes et conteneurs pour les tableaux
        self.trades_label = QLabel("Historique des trades")
        self.trades_label.setStyleSheet("font-weight: bold; color: #0066cc;")
        self.tables_layout.addWidget(self.trades_label)
        
        # Partie configuration des tableaux (pagination, filtres)
        self.trades_config = QWidget()
        self.trades_config_layout = QHBoxLayout(self.trades_config)
        
        self.trades_limit = QComboBox()
        self.trades_limit.addItems(["10 trades", "25 trades", "50 trades", "100 trades", "Tous"])
        self.trades_limit.setCurrentIndex(1)  # Par défaut: 25 trades
        self.trades_limit.currentIndexChanged.connect(self._refresh_trades_table)
        
        self.trades_config_layout.addWidget(QLabel("Afficher:"))
        self.trades_config_layout.addWidget(self.trades_limit)
        self.trades_config_layout.addStretch()
        
        self.tables_layout.addWidget(self.trades_config)
        
        # Table des trades avec modèle optimisé
        self.trades_table = QTableView()
        self.trades_table.setModel(self.trades_model)
        self.trades_table.setSortingEnabled(True)
        self.trades_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.trades_table.setMinimumHeight(300)
        self.tables_layout.addWidget(self.trades_table)
        
        # Table de l'équité (optionnelle car très volumineuse)
        self.equity_show = QPushButton("Afficher/Masquer table d'équité")
        self.equity_show.clicked.connect(lambda checked: self._toggle_equity_table())
        self.tables_layout.addWidget(self.equity_show)
        
        self.equity_label = QLabel("Statistiques de performance")
        self.equity_label.setStyleSheet("font-weight: bold; color: #0066cc;")
        self.equity_label.setVisible(False)
        self.tables_layout.addWidget(self.equity_label)
        
        self.equity_config = QWidget()
        self.equity_config_layout = QHBoxLayout(self.equity_config)
        self.equity_config.setVisible(False)
        
        self.equity_limit = QComboBox()
        self.equity_limit.addItems(["100 points", "500 points", "1000 points", "5000 points", "Tous"])
        self.equity_limit.setCurrentIndex(1)  # Par défaut: 500 points
        self.equity_limit.currentIndexChanged.connect(self._refresh_equity_table)
        
        self.equity_config_layout.addWidget(QLabel("Afficher:"))
        self.equity_config_layout.addWidget(self.equity_limit)
        self.equity_config_layout.addStretch()
        
        self.tables_layout.addWidget(self.equity_config)
        
        self.equity_table = QTableView()
        self.equity_table.setModel(self.equity_model)
        self.equity_table.setSortingEnabled(True)
        self.equity_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.equity_table.setMinimumHeight(300)
        self.equity_table.setVisible(False)
        self.tables_layout.addWidget(self.equity_table)
        
        self.stats_content_layout.addWidget(self.tables_container)
        
        # Ajouter un espacement
        self.stats_content_layout.addStretch()
    
    def _create_metric_widget(self, key, label, value, row, col, layout):
        """Crée un widget de métrique et le stocke dans le dictionnaire."""
        widget = MetricWidget(label, value)
        layout.addWidget(widget, row, col)
        self.metric_widgets[key] = widget
        return widget
    
    def _get_limit_value(self, combo):
        """Extrait la valeur numérique du combobox."""
        text = combo.currentText()
        if text == "Tous":
            return None
        return int(text.split()[0])
    
    def _refresh_trades_table(self):
        """Rafraîchit la table des trades avec la limite sélectionnée."""
        if not hasattr(self, '_last_stats') or self._last_stats is None:
            return
            
        # Vérifier si la clé '_trades' existe
        if '_trades' not in self._last_stats:
            return
            
        limit = self._get_limit_value(self.trades_limit)
        trades = self._last_stats['_trades']
        
        if limit and len(trades) > limit:
            trades = trades.tail(limit)
            
        self.trades_model.update_data(trades)

    def _refresh_equity_table(self):
        """Rafraîchit la table d'équité avec la limite sélectionnée."""
        if not hasattr(self, '_last_stats') or self._last_stats is None:
            return
            
        # Vérifier si la clé '_equity_curve' existe
        if '_equity_curve' not in self._last_stats:
            return
            
        limit = self._get_limit_value(self.equity_limit)
        equity = self._last_stats['_equity_curve']
        
        if limit and len(equity) > limit:
            # Échantillonnage intelligent pour conserver la forme globale
            step = max(1, len(equity) // limit)
            equity = equity.iloc[::step]
            
        self.equity_model.update_data(equity)
    
    def _toggle_equity_table(self):
        """Affiche ou masque la table d'équité."""
        visible = not self.equity_table.isVisible()
        self.equity_label.setVisible(visible)
        self.equity_config.setVisible(visible)
        self.equity_table.setVisible(visible)
        
        # Rafraîchir les données si la table devient visible
        if visible:
            self._refresh_equity_table()
    
    def update(self, data=None, stats=None):
        """Met à jour les statistiques avec les nouvelles données."""
        start_time = time.time()
        
        if stats is None:
            return

        # Stocker les statistiques pour utilisation ultérieure
        self._last_stats = stats
            
        # Rendre visibles les widgets
        self.stats_placeholder.setVisible(False)
        self.title.setVisible(True)
        self.main_stats_widget.setVisible(True)
        self.adv_group.setVisible(True)
        self.tables_container.setVisible(True)
        
        # Mise à jour efficace des métriques
        # Période
        self.metric_widgets["Start"].update_values(stats["Start"].strftime("%Y-%m-%d %H:%M"))
        self.metric_widgets["Duration"].update_values(str(stats["Duration"]))
        self.metric_widgets["End"].update_values(stats["End"].strftime("%Y-%m-%d %H:%M"))
        self.metric_widgets["Exposure"].update_values(f"{stats['Exposure Time [%]']:.2f}%")

        # Performance - Ajout du delta par rapport au B&H dans la case rendement
        delta = f"{stats['Return [%]'] - stats['Buy & Hold Return [%]']:.2f}% vs B&H"
        self.metric_widgets["Return"].update_values(f"{stats['Return [%]']:.2f}%", delta)
        self.metric_widgets["ReturnAnn"].update_values(f"{stats['Return (Ann.) [%]']:.2f}%")
        self.metric_widgets["Volatility"].update_values(f"{stats['Volatility (Ann.) [%]']:.2f}%")
        self.metric_widgets["CAGR"].update_values(f"{stats['CAGR [%]']:.2f}%")

        self.metric_widgets["EquityFinal"].update_values(f"${stats['Equity Final [$]']:,.3f}")
        self.metric_widgets["EquityPeak"].update_values(f"${stats['Equity Peak [$]']:,.3f}")
        self.metric_widgets["BuyHold"].update_values(f"{stats['Buy & Hold Return [%]']:.3f}%")
        self.metric_widgets["Alpha"].update_values(f"{stats['Alpha [%]']:.4f}%")

        # Risque
        sharpe_color = "normal" if stats['Sharpe Ratio'] >= 0 else "inverse"
        self.metric_widgets["Sharpe"].update_values(f"{stats['Sharpe Ratio']:.2f}", delta_color=sharpe_color)
        self.metric_widgets["Sortino"].update_values(f"{stats['Sortino Ratio']:.2f}")
        self.metric_widgets["MaxDrawdown"].update_values(f"{stats['Max. Drawdown [%]']:.2f}%", delta_color="inverse")
        self.metric_widgets["AvgDrawdown"].update_values(f"{stats['Avg. Drawdown [%]']:.2f}%", delta_color="inverse")

        self.metric_widgets["Calmar"].update_values(f"{stats['Calmar Ratio']:.2f}")
        self.metric_widgets["Beta"].update_values(f"{stats['Beta']:.4f}")
        self.metric_widgets["MaxDrawdownDuration"].update_values(str(stats["Max. Drawdown Duration"]))
        self.metric_widgets["AvgDrawdownDuration"].update_values(str(stats["Avg. Drawdown Duration"]))

        # Trades
        self.metric_widgets["NumTrades"].update_values(f"{stats['# Trades']}")
        self.metric_widgets["WinRate"].update_values(f"{stats['Win Rate [%]']:.2f}%")

        self.metric_widgets["WinningTrades"].update_values(f"{stats['# Winning Trades']}")
        self.metric_widgets["LosingTrades"].update_values(f"{stats['# Losing Trades']}")
        self.metric_widgets["NeutralTrades"].update_values(f"{stats['# Neutral Trades']}")

        self.metric_widgets["BestTrade"].update_values(f"{stats['Best Trade [%]']:.2f}%")
        self.metric_widgets["WorstTrade"].update_values(f"{stats['Worst Trade [%]']:.2f}%")

        self.metric_widgets["AvgTrade"].update_values(f"{stats['Avg. Trade [%]']:.4f}%")
        self.metric_widgets["MaxTradeDuration"].update_values(str(stats["Max. Trade Duration"]))
        self.metric_widgets["AvgTradeDuration"].update_values(str(stats["Avg. Trade Duration"]))
        self.metric_widgets["ProfitFactor"].update_values(f"{stats['Profit Factor']:.2f}")

        # Métriques avancées
        self.metric_widgets["Expectancy"].update_values(f"{stats['Expectancy [%]']:.4f}%")
        self.metric_widgets["SQN"].update_values(f"{stats['SQN']:.4f}")
        self.metric_widgets["Kelly"].update_values(f"{stats['Kelly Criterion']:.4f}")
        
        # Mettre à jour les tableaux avec pagination
        self._refresh_trades_table()
        
        print(f"StatsView update: {(time.time() - start_time) * 1000:.2f} ms")