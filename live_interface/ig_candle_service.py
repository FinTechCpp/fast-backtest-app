import logging
import threading
import datetime
import time
from collections import deque
from dataclasses import dataclass
import json
import os
from typing import List, Dict, Any

from igtrader.WrapperIGAPI.trading_ig.rest import IGService
from igtrader.WrapperIGAPI.trading_ig.config import config
from igtrader.WrapperIGAPI.trading_ig.stream import IGStreamService
from igtrader.WrapperIGAPI.trading_ig.streamer.manager import StreamingManager
from igtrader.WrapperIGAPI.TickBroker import PriceSource, BaseCandle

class IGCandleService:
    """Service pour collecter les ticks IG et générer des bougies en temps réel"""
    
    def __init__(self, epic, candle_interval=60, price_source=PriceSource.BID):
        """
        Initialise le service de collecte de bougies.
        
        Args:
            epic (str): Code de l'instrument à surveiller
            candle_interval (int): Intervalle en secondes pour la construction des bougies
            price_source (PriceSource): Source de prix à utiliser (BID, ASK ou LAST)
        """
        self.epic = epic
        self.candle_interval = candle_interval
        self.price_source = price_source
        
        # Configuration du logging
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        
        # Structures de données pour le streaming
        self.stream_lock = threading.Lock()
        self.ticks_buffer = deque(maxlen=10000)
        self.candles = []
        self.current_candle = None
        self.last_tick_time = None
        
        # Connexion à l'API IG
        self._setup_ig_connection()
        
        # Démarrage du thread de construction des bougies
        self.should_continue = True
        self.candle_thread = threading.Thread(target=self._build_candles_from_ticks_loop, daemon=True)
        self.candle_thread.start()
        
        # Callbacks
        self.candle_callbacks = []
        
        # Pour gérer la bougie en cours de formation
        self.current_candle = None
        self.current_candle_time = None
        self.tick_callbacks = []  # Callbacks pour les mises à jour de tick
        

    
    def _setup_ig_connection(self):
        """Établit la connexion avec l'API IG et initialise le streaming."""
        # Initialisation de l'API REST IG
        self.ig_service = IGService(
            config.username, 
            config.password, 
            config.api_key, 
            config.acc_type,
            acc_number=config.acc_number
        )
        
        # Création de la session REST
        self.ig_service.create_session(version='3')
        
        # Initialisation du service de streaming
        self.stream_service = IGStreamService(self.ig_service)
        self.stream_service.acc_number = config.acc_number
        self.stream_service.create_session(version="3")
        
        # Création du gestionnaire de streaming
        self.stream_manager = StreamingManager(self.stream_service)
        
        # Souscription aux ticks pour l'epic spécifié
        if self.epic:
            time.sleep(2)  # Attente pour la stabilisation de la connexion
            self.tick_subscription = self.stream_manager.start_tick_subscription(self.epic)
            logging.info(f"Abonnement aux ticks pour {self.epic} démarré")
    
    def _align_time_to_interval(self, dt):
        """Aligne un datetime sur l'intervalle le plus proche dans le passé."""
        seconds = dt.second + dt.minute * 60 + dt.hour * 3600
        interval_seconds = self.candle_interval
        
        # Calcul des secondes depuis le début de la journée
        aligned_seconds = (seconds // interval_seconds) * interval_seconds
        
        return dt.replace(
            hour=aligned_seconds // 3600,
            minute=(aligned_seconds % 3600) // 60,
            second=aligned_seconds % 60,
            microsecond=0
        )
    
    def _build_candles_from_ticks_loop(self):
        """Thread en arrière-plan qui construit continuellement des bougies à partir des ticks."""
        logging.info("Démarrage du thread de construction des bougies")
        self.last_candle_time = None
        ticks_by_interval = {}
        
        while self.should_continue:
            try:
                now = datetime.datetime.now()
                aligned_time = self._align_time_to_interval(now)
                
                if self.last_candle_time is None:
                    self.last_candle_time = aligned_time - datetime.timedelta(seconds=self.candle_interval)
                    self.current_candle_time = aligned_time
                
                # Traitement des ticks s'ils sont disponibles
                if hasattr(self, 'stream_manager') and hasattr(self.stream_manager, 'tickers'):
                    if self.epic in self.stream_manager.tickers:
                        ticker_data = self.stream_manager.tickers[self.epic]
                        
                        if hasattr(ticker_data, 'timestamp') and ticker_data.timestamp:
                            # Déterminer l'intervalle auquel ce tick appartient
                            tick_time = ticker_data.timestamp
                            candle_start_time = self._align_time_to_interval(tick_time)
                            
                            # Ajouter le tick au buffer pour cet intervalle
                            with self.stream_lock:
                                if candle_start_time not in ticks_by_interval:
                                    ticks_by_interval[candle_start_time] = []
                                
                                # Créer une copie du tick
                                price = 0.0
                                if self.price_source == PriceSource.BID:
                                    price = ticker_data.bid
                                elif self.price_source == PriceSource.ASK:
                                    price = ticker_data.offer
                                elif self.price_source == PriceSource.LAST:
                                    price = ticker_data.last_traded_price
                                
                                tick_info = {
                                    'time': tick_time,
                                    'price': price
                                }
                                
                                ticks_by_interval[candle_start_time].append(tick_info)
                                self.ticks_buffer.append(tick_info)
                                
                                # Mettre à jour la bougie en cours de formation
                                if candle_start_time == self.current_candle_time:
                                    ticks = ticks_by_interval[self.current_candle_time]
                                    
                                    if not ticks:
                                        continue
                                    
                                    # Si c'est le premier tick pour cette bougie
                                    if self.current_candle is None:
                                        self.current_candle = BaseCandle(
                                            date=self.current_candle_time,
                                            Open=ticks[0]['price'],
                                            High=ticks[0]['price'],
                                            Low=ticks[0]['price'],
                                            Close=ticks[0]['price']
                                        )
                                    else:
                                        # Mise à jour de la bougie en cours
                                        current_price = ticks[-1]['price']
                                        self.current_candle.High = max(self.current_candle.High, current_price)
                                        self.current_candle.Low = min(self.current_candle.Low, current_price)
                                        self.current_candle.Close = current_price
                                    
                                    # Notifier les callbacks de tick
                                    for callback in self.tick_callbacks:
                                        callback(self.current_candle)
                
                # Vérifier s'il est temps de créer une nouvelle bougie
                if aligned_time > self.last_candle_time:
                    # Construire une bougie à partir des ticks dans l'intervalle précédent
                    with self.stream_lock:
                        if self.last_candle_time in ticks_by_interval and ticks_by_interval[self.last_candle_time]:
                            ticks = ticks_by_interval[self.last_candle_time]
                            
                            # Créer une nouvelle bougie
                            candle = BaseCandle(
                                date=self.last_candle_time,
                                Open=ticks[0]['price'],
                                High=max(tick['price'] for tick in ticks),
                                Low=min(tick['price'] for tick in ticks),
                                Close=ticks[-1]['price']
                            )
                            
                            # Ajouter la bougie à la liste
                            self.candles.append(candle)
                            
                            # Limiter le nombre de bougies stockées
                            if len(self.candles) > 1000:
                                self.candles = self.candles[-1000:]
                            
                            # Notifier tous les callbacks enregistrés
                            for callback in self.candle_callbacks:
                                callback(candle)
                            
                            logging.debug(f"Nouvelle bougie créée: {candle.date} - O:{candle.Open} H:{candle.High} L:{candle.Low} C:{candle.Close}")
                        else:
                            logging.debug(f"Pas de ticks pour l'intervalle {self.last_candle_time}, aucune bougie créée")
                    
                    # Mettre à jour le temps de la dernière bougie
                    self.last_candle_time = aligned_time
                    self.current_candle_time = aligned_time
                    self.current_candle = None  # Réinitialiser la bougie en cours
                    
                    # Nettoyer les anciennes données pour éviter les fuites de mémoire
                    with self.stream_lock:
                        for timestamp in list(ticks_by_interval.keys()):
                            if timestamp < self.last_candle_time - datetime.timedelta(minutes=5):
                                del ticks_by_interval[timestamp]
                
                # Pause pour éviter une utilisation élevée du CPU
                time.sleep(0.1)
                
            except Exception as e:
                logging.error(f"Erreur dans le thread de construction des bougies: {e}")
                import traceback
                logging.error(traceback.format_exc())
                time.sleep(1)  # Pause en cas d'erreur pour éviter les boucles rapides
    
    def register_candle_callback(self, callback):
        """
        Enregistre une fonction de callback pour les nouvelles bougies.
        
        Args:
            callback (function): Fonction à appeler avec la nouvelle bougie comme argument
        """
        self.candle_callbacks.append(callback)
        
    def register_tick_callback(self, callback):
        """
        Enregistre une fonction de callback pour les mises à jour de tick.
        
        Args:
            callback (function): Fonction à appeler avec le nouveau tick comme argument
        """
        self.tick_callbacks.append(callback)
    
    def get_latest_candles(self, count=100):
        """
        Récupère les dernières bougies.
        
        Args:
            count (int): Nombre de bougies à récupérer
            
        Returns:
            list: Liste des dernières bougies
        """
        with self.stream_lock:
            return self.candles[-count:] if self.candles else []
    
    def stop(self):
        """Arrête le service et libère les ressources."""
        self.should_continue = False
        
        if hasattr(self, 'stream_manager'):
            logging.info("Arrêt des abonnements streaming")
            self.stream_manager.stop_subscriptions()
            
        if hasattr(self, 'candle_thread') and self.candle_thread.is_alive():
            self.candle_thread.join(timeout=1)
            
        logging.info("Service de bougies arrêté")

class CandleStorage:
    """Gère le stockage persistant des bougies OHLC"""
    
    def __init__(self, file_path: str = "candles_history.json", max_candles: int = 10000):
        self.file_path = file_path
        self.max_candles = max_candles
        self.candles = self._load_candles()
        
    def _load_candles(self) -> List[Dict[str, Any]]:
        """Charge les bougies depuis le fichier"""
        if not os.path.exists(self.file_path):
            return []
            
        try:
            with open(self.file_path, 'r') as f:
                return json.load(f)
        except (json.JSONDecodeError, FileNotFoundError) as e:
            logging.error(f"Erreur lors du chargement des bougies: {e}")
            return []
            
    def _save_candles(self):
        """Sauvegarde les bougies dans le fichier"""
        try:
            with open(self.file_path, 'w') as f:
                json.dump(self.candles, f)
        except Exception as e:
            logging.error(f"Erreur lors de la sauvegarde des bougies: {e}")
            
    def add_candle(self, candle: Dict[str, Any]):
        """Ajoute une bougie au stockage"""
        # Vérifier si la bougie existe déjà (même timestamp)
        candle_time = candle.get('time')
        if candle_time:
            # Rechercher une bougie existante avec le même timestamp
            for i, existing in enumerate(self.candles):
                if existing.get('time') == candle_time:
                    # Mise à jour d'une bougie existante
                    self.candles[i] = candle
                    self._save_candles()
                    return
                    
        # Ajout d'une nouvelle bougie
        self.candles.append(candle)
        
        # Limiter le nombre de bougies
        if len(self.candles) > self.max_candles:
            # Trier par date et ne garder que les plus récentes
            self.candles.sort(key=lambda x: x.get('time', ''))
            self.candles = self.candles[-self.max_candles:]
            
        self._save_candles()
        
    def get_candles(self, limit: int = None) -> List[Dict[str, Any]]:
        """Récupère les bougies stockées"""
        # Trier par date
        sorted_candles = sorted(self.candles, key=lambda x: x.get('time', ''))
        
        if limit and limit > 0:
            return sorted_candles[-limit:]
        return sorted_candles