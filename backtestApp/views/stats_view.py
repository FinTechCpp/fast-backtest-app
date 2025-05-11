from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QLabel, QScrollArea, QGroupBox, 
                           QHBoxLayout, QGridLayout, QTableWidget, QTableWidgetItem)
from PyQt5.QtGui import QColor
from PyQt5.QtCore import Qt

from views.base_view import ResultView
from metric_widget import MetricWidget

class StatsView(ResultView):
    """Vue pour afficher les statistiques du backtest."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
    
    def create(self):
        """Crée le widget principal pour les statistiques."""
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
        
        return self.stats_tab
    
    def update(self, data=None, stats=None):
        """Met à jour les statistiques avec les nouvelles données."""
        if stats is None:
            return
            
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

        # Nouvelles statistiques sur les types de trades
        trade_layout.addWidget(MetricWidget("Trades gagnants", f"{stats['# Winning Trades']}"), 0, 1)
        trade_layout.addWidget(MetricWidget("Trades perdants", f"{stats['# Losing Trades']}"), 1, 1)
        trade_layout.addWidget(MetricWidget("Trades neutres", f"{stats['# Neutral Trades']}"), 2, 1)

        trade_layout.addWidget(MetricWidget("Meilleur trade [%]", f"{stats['Best Trade [%]']:.2f}%"), 4, 0)
        trade_layout.addWidget(MetricWidget("Pire trade [%]", f"{stats['Worst Trade [%]']:.2f}%"), 5, 0)
        
        trade_layout.addWidget(MetricWidget("Trade moyen [%]", f"{stats['Avg. Trade [%]']:.4f}%"), 4, 1)
        trade_layout.addWidget(MetricWidget("Durée max trade", str(stats["Max. Trade Duration"])), 5, 1)
        trade_layout.addWidget(MetricWidget("Durée moyenne trade", str(stats["Avg. Trade Duration"])), 6, 0)
        trade_layout.addWidget(MetricWidget("Profit Factor", f"{stats['Profit Factor']:.2f}"), 6, 1)
        
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
        
        # Ajouter les tableaux de trades et d'equity
        self._add_trades_table(stats)
        self._add_equity_table(stats)
        
        # Ajouter un espacement en bas
        self.stats_content_layout.addStretch()
    
    def _add_trades_table(self, stats):
        """Ajoute le tableau des trades."""
        trades_label = QLabel("Historique des trades")
        trades_label.setStyleSheet("font-weight: bold; color: #0066cc;")
        self.stats_content_layout.addWidget(trades_label)
        
        trades = stats['_trades']
        trades_table = QTableWidget()
        
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
    
    def _add_equity_table(self, stats):
        """Ajoute le tableau de l'équité."""
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