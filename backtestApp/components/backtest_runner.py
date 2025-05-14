import logging
import pandas as pd
from PyQt5.QtWidgets import QPushButton, QProgressBar, QHBoxLayout, QApplication
from PyQt5.QtCore import QObject, pyqtSignal
import traceback
from backtestApp.ui_util import BacktestWorker


class BacktestRunner(QObject):
    """
    Classe qui encapsule la fonctionnalité d'exécution du backtest,
    y compris le bouton de lancement, l'indicateur de chargement,
    et le traitement des résultats.
    """
    # Signaux personnalisés pour communiquer avec l'application parent
    backtest_started = pyqtSignal()
    backtest_completed = pyqtSignal(object, object)  # data, stats
    backtest_error = pyqtSignal(str)
    
    def __init__(self, parent_app):
        super().__init__()
        
        # Stocker une référence à l'application parent
        self.parent = parent_app
        
        # Créer les composants UI
        self.create_ui_components()
        
        # Connecter les signaux internes
        self.backtest_completed.connect(self.on_backtest_finished)
        self.backtest_error.connect(self.on_backtest_error)
    
    def create_ui_components(self):
        """Crée le bouton d'exécution et l'indicateur de chargement"""
        # Créer le layout du bouton
        self.button_layout = QHBoxLayout()
        
        # Créer le bouton d'exécution
        self.run_button = QPushButton("Lancer le backtest")
        self.run_button.setMinimumHeight(40)
        self.run_button.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;")
        self.run_button.clicked.connect(self.run_backtest)
        
        # Créer l'indicateur de chargement
        self.loading_indicator = QProgressBar()
        self.loading_indicator.setMaximum(0)  # Mode indéterminé
        self.loading_indicator.setMinimum(0)
        self.loading_indicator.setTextVisible(False)  # Pas de texte de pourcentage
        self.loading_indicator.setMaximumHeight(10)
        self.loading_indicator.setVisible(False)  # Caché par défaut
        
        # Ajouter les widgets au layout
        self.button_layout.addWidget(self.run_button)
        self.button_layout.addWidget(self.loading_indicator)
    
    def get_layout(self):
        """Retourne le layout du bouton pour l'intégration dans l'application parent"""
        return self.button_layout
    
    def run_backtest(self):
        """Exécute le backtest avec les paramètres sélectionnés et met à jour l'interface"""
        try:
            # Mettre à jour l'UI pour indiquer que le backtest est en cours
            self.loading_indicator.setVisible(True)
            self.run_button.setEnabled(False)
            self.run_button.setText("Exécution du backtest en cours...")
            QApplication.processEvents()
            
            # Émettre le signal de démarrage
            self.backtest_started.emit()
            
            # Récupérer tous les paramètres de configuration
            config = self.parent.get_strategy_config()
            
            # Charger les données
            logging.debug("Chargement des données...")
            from igtrader.Strategies.Helpers import load_data
            data = load_data(
                symbol=config['symbol'],
                period=config['period'],
                interval=config['interval'],
                end_date=config['end_date'],
                # timezone=config['timezone'],
                trading_from=config.get('trading_from'),
                trading_to=config.get('trading_to'),
                trading_days=config.get('trading_days'))
            
            # Vérifier la stratégie
            strategy_name = config['strategy']
            if strategy_name not in self.parent.strategy_map:
                logging.error(f"Stratégie {strategy_name} introuvable. Stratégies disponibles: {list(self.parent.strategy_map.keys())}")
                self.backtest_error.emit(f"Stratégie {strategy_name} introuvable")
                return
            
            # Définir l'onglet à afficher à la fin de l'exécution du backtest (1 = Statistiques)
            if hasattr(self.parent, 'result_manager'):
                self.parent.result_manager.set_current_tab(1)
            
            # Créer et exécuter le thread de backtest
            self.backtest_thread = BacktestWorker(
                data=data,
                strategy=self.parent.strategy_map[strategy_name],
                cash=config['cash'],
                spread=config['spread'],
                strategy_kwargs=config,
            )
            
            # Connecter les signaux
            self.backtest_thread.finished.connect(self.backtest_completed.emit)
            self.backtest_thread.error.connect(self.backtest_error.emit)
            
            # Démarrer le thread
            self.backtest_thread.start()
            
        except Exception as e:
            logging.exception(f"Erreur lors de l'exécution du backtest: {str(e)}")
            self.backtest_error.emit(str(e))
    
    def on_backtest_finished(self, data, stats):
        """Fonction appelée lorsque le backtest est terminé avec succès"""
        try:
            # Traiter les résultats en vérifiant d'abord le type des données
            if '_trades' in stats and isinstance(stats['_trades'], pd.DataFrame):
                # Vérifier EntryTime
                if 'EntryTime' in stats['_trades'].columns:
                    if pd.api.types.is_datetime64_dtype(stats['_trades']['EntryTime']):
                        # Seulement si c'est un datetime et qu'il a une timezone
                        if hasattr(stats['_trades']['EntryTime'].dt, 'tz') and stats['_trades']['EntryTime'].dt.tz is not None:
                            stats['_trades']['EntryTime'] = stats['_trades']['EntryTime'].dt.tz_localize(None)
                
                # Vérifier ExitTime
                if 'ExitTime' in stats['_trades'].columns:
                    if pd.api.types.is_datetime64_dtype(stats['_trades']['ExitTime']):
                        # Seulement si c'est un datetime et qu'il a une timezone
                        if hasattr(stats['_trades']['ExitTime'].dt, 'tz') and stats['_trades']['ExitTime'].dt.tz is not None:
                            stats['_trades']['ExitTime'] = stats['_trades']['ExitTime'].dt.tz_localize(None)
            
            # Vérifier equity_curve
            if '_equity_curve' in stats and hasattr(stats['_equity_curve'], 'index'):
                if pd.api.types.is_datetime64_dtype(stats['_equity_curve'].index):
                    if hasattr(stats['_equity_curve'].index, 'tz') and stats['_equity_curve'].index.tz is not None:
                        stats['_equity_curve'].index = stats['_equity_curve'].index.tz_localize(None)
        
            # Stocker les statistiques dans l'application parent
            self.parent.stats = stats
            
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
                chart_data['time'] = chart_data['time']
            chart_data['time'] = chart_data['time'].dt.tz_localize(None)
            
            # Trier les données
            chart_data.sort_values('time', inplace=True)

            # Mettre à jour toutes les vues
            self.parent.result_manager.update_all(chart_data, stats)
            
            # Afficher l'onglet des statistiques
            self.parent.result_manager.set_current_tab(1)  # Index 1 = Statistics tab
            
            logging.info("Backtest exécuté avec succès")
        
        finally:
            # Réinitialiser l'interface
            self.loading_indicator.setVisible(False)
            self.run_button.setEnabled(True)
            self.run_button.setText("Lancer le backtest")
    
    def on_backtest_error(self, error_msg):
        """Fonction appelée en cas d'erreur pendant le backtest"""
        logging.error(f"Erreur pendant le backtest: {error_msg}")
        
        # Log the full traceback if available
        current_tb = traceback.format_exc()
        if current_tb and 'NoneType' not in current_tb:
            logging.error(f"Traceback complet:\n{current_tb}")
        
        # Réinitialiser l'interface
        self.loading_indicator.setVisible(False)
        self.run_button.setEnabled(True)
        self.run_button.setText("Lancer le backtest")