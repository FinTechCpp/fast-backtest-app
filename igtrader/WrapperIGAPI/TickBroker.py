from igtrader.WrapperIGAPI.trading_ig.rest import IGService
from igtrader.WrapperIGAPI.trading_ig.config import config
from igtrader.WrapperIGAPI.trading_ig.stream import IGStreamService
from igtrader.WrapperIGAPI.trading_ig.streamer.manager import StreamingManager
from igtrader.WrapperIGAPI.trading_ig.streamer.candler import CandleSubscription
import pandas as pd
import numpy as np
import datetime
import logging
import threading
import time
import math
from collections import deque
import traceback
from enum import Enum

EXPIRY = '-'
CURRENCY = 'EUR'
ORDER_TYPE = 'MARKET'


class PriceSource(Enum):
    BID = 'bid'
    ASK = 'ask'
    LAST = 'last'

    @classmethod
    def get_default(cls):
        return cls.BID


class TickBroker:
    def __init__(self, epic=None, candle_interval=None, price_source: PriceSource = PriceSource.get_default(), candle_callback=None):
        """
        Initialise un broker qui construit des bougies à partir de ticks.
        
        Args:
            epic (str): Code de l'instrument sur lequel trader
            working_resolution (str): Résolution pour les données historiques
            candle_interval (int): Intervalle en secondes pour la construction des bougies
            price_source (str): Source de prix à utiliser ('bid', 'ask', ou 'last')
        """
        # Initialize REST API client for retrieving historical data and placing orders
        self.ig_service = IGService(
            config.username, 
            config.password, 
            config.api_key, 
            config.acc_type,
            acc_number=config.acc_number
        )
    
        # Create REST session for authentication
        self.ig_service.create_session(version='3')
        logging.info("Connexion à l'API du broker établie.")
    
        self.epic = epic
        self.candle_interval = candle_interval  # in seconds
        self.price_source: PriceSource = price_source
        self.candle_callback = candle_callback
        
        logging.info(f"Utilisation des prix '{self.price_source.value}' pour la construction des bougies")
        
        # Streaming data structures
        self.stream_lock = threading.Lock()
        self.current_candle = None
        self.ticks_buffer = deque(maxlen=10000)  # Un buffer plus grand pour les ticks
        self.candles_by_interval = {}  # Pour stocker les bougies construites par intervalle
        self.last_tick_time = None
        self.last_candle_time = None
        
        # Initialize streaming connection
        self._setup_streaming()
        
        # Start candle builder thread
        self.should_continue = True
        self.candle_thread = threading.Thread(target=self._build_candles_from_ticks_loop, daemon=True)
        self.candle_thread.start()
        
        # Wait for initial data collection
        logging.info("Waiting for initial streaming data...")
        time.sleep(3)  # Allow time for data to start coming in

    def _align_time_to_interval(self, dt):
        """Align a datetime to the nearest interval boundary in the past."""
        seconds = dt.second + dt.minute * 60 + dt.hour * 3600
        interval_seconds = self.candle_interval
        
        # Calculate seconds since the start of the day
        aligned_seconds = (seconds // interval_seconds) * interval_seconds
        
        return dt.replace(
            hour=aligned_seconds // 3600,
            minute=(aligned_seconds % 3600) // 60,
            second=aligned_seconds % 60,
            microsecond=0
        )
    
    def _find_candle_start_time(self, tick_time):
        """Find the start time of the candle interval a tick belongs to."""
        return self._align_time_to_interval(tick_time)
            
    def _setup_streaming(self):
        """Initialize streaming connection and subscribe to tick data."""
        try:
            logging.debug("Creating streaming service...")
            self.stream_service = IGStreamService(self.ig_service)
            
            self.stream_service.acc_number = config.acc_number
            
            logging.debug("Creating streaming session...")
            self.stream_service.create_session(version="3")
            logging.info("Streaming session created")
            
            logging.debug("Creating streaming manager...")
            self.stream_manager = StreamingManager(self.stream_service)
            
            # Subscribe to TICK data for the specified epic
            if self.epic:
                logging.debug("Waiting for connection to stabilize...")
                time.sleep(2)
                
                logging.debug(f"Creating tick subscription for {self.epic}")
                self.tick_subscription = self.stream_manager.start_tick_subscription(self.epic)
                logging.info(f"Started tick subscription for {self.epic}")
                
                # Vérifier que la souscription est active
                timeout = time.time() + 10  # 10 secondes de timeout
                while time.time() < timeout:
                    if hasattr(self.stream_manager, 'tickers') and self.epic in self.stream_manager.tickers:
                        logging.info(f"Tick subscription active for {self.epic}")
                        break
                    time.sleep(0.5)
                    
                # Vérifier si nous sommes toujours dans la boucle après timeout
                if time.time() >= timeout:
                    logging.warning(f"Tick subscription timeout for {self.epic}")
        except Exception as e:
            logging.error(f"Error setting up streaming: {e}")
            logging.error(f"Details: {traceback.format_exc()}")
            raise
    
    def _build_candles_from_ticks_loop(self):
        """Background thread that continuously builds candles from tick data."""
        logging.info("Starting candle building from ticks thread")
        self.last_candle_time = None
        ticks_by_interval = {}  # Buffer pour stocker les ticks par intervalle
        
        while self.should_continue:
            try:
                now = datetime.datetime.now()
                aligned_time = self._align_time_to_interval(now)
                
                if self.last_candle_time is None:
                    self.last_candle_time = aligned_time - datetime.timedelta(seconds=self.candle_interval)
                
                # Process ticks if available
                if hasattr(self, 'stream_manager') and hasattr(self.stream_manager, 'tickers'):
                    if self.epic in self.stream_manager.tickers:
                        ticker_data = self.stream_manager.tickers[self.epic]
                        
                        if hasattr(ticker_data, 'timestamp') and ticker_data.timestamp:
                            # Ajouter le tick au buffer approprié basé sur son timestamp
                            tick_time = ticker_data.timestamp
                            # Trouver l'intervalle auquel ce tick appartient
                            candle_start_time = self._find_candle_start_time(tick_time)
                            
                            with self.stream_lock:
                                if candle_start_time not in ticks_by_interval:
                                    ticks_by_interval[candle_start_time] = []
                                # Faire une copie du ticker pour éviter les problèmes de référence
                                tick_copy = {
                                    'timestamp': tick_time,
                                    'bid': ticker_data.bid if hasattr(ticker_data, 'bid') and not math.isnan(ticker_data.bid) else None,
                                    'offer': ticker_data.offer if hasattr(ticker_data, 'offer') and not math.isnan(ticker_data.offer) else None,
                                    'last': ticker_data.last_traded_price if hasattr(ticker_data, 'last_traded_price') and not math.isnan(ticker_data.last_traded_price) else None
                                }
                                ticks_by_interval[candle_start_time].append(tick_copy)   
                                # Ajouter aussi au buffer global pour les stratégies qui veulent des ticks
                                self.ticks_buffer.append(ticker_data)
                                self.last_tick_time = tick_time
                
                # Check if it's time to create a new candle
                if aligned_time > self.last_candle_time:
                    # Construire une bougie à partir des ticks dans l'intervalle précédent
                    with self.stream_lock:
                        if self.last_candle_time in ticks_by_interval and ticks_by_interval[self.last_candle_time]:
                            # Trier les ticks par timestamp pour s'assurer qu'ils sont dans l'ordre
                            ticks_for_candle = sorted(ticks_by_interval[self.last_candle_time], key=lambda x: x['timestamp'])
                            
                            # Dans la section qui détermine les prix à utiliser:
                            if self.price_source == PriceSource.BID:
                                prices = [t['bid'] for t in ticks_for_candle if t['bid'] is not None]
                            elif self.price_source == PriceSource.ASK:
                                prices = [t['offer'] for t in ticks_for_candle if t['offer'] is not None]
                            else:  # 'last' ou autre
                                prices = [t['last'] for t in ticks_for_candle if t['last'] is not None]
                                
                            if prices:  # S'assurer qu'il y a des prix valides
                                open_price = prices[0]
                                high_price = max(prices)
                                low_price = min(prices)
                                close_price = prices[-1]
                                
                                new_candle = {
                                    'date': self.last_candle_time,
                                    'Open': open_price,
                                    'High': high_price,
                                    'Low': low_price,
                                    'Close': close_price,
                                    'ticks_count': len(prices)  # Nombre de ticks utilisés
                                }
                                
                                # Mettre à jour la bougie courante
                                self.current_candle = new_candle
                                
                                # Stocker la bougie dans le dictionnaire par intervalle
                                self.candles_by_interval[self.last_candle_time] = new_candle
                                
                                logging.info(f"Built new {self.price_source.value} candle from {len(prices)} ticks for {self.last_candle_time}: "
                                            f"O={open_price:.5f}, H={high_price:.5f}, L={low_price:.5f}, C={close_price:.5f}")
                                
                                # Appeler le callback avec la nouvelle bougie si défini
                                if self.candle_callback is not None:
                                    # Créer un BaseCandle à partir du dict
                                    from igtrader.Strategies.Strategy import BaseCandle
                                    base_candle = BaseCandle(
                                        date=self.last_candle_time,
                                        Open=open_price,
                                        High=high_price,
                                        Low=low_price,
                                        Close=close_price,
                                        in_position=self.has_open_position(),
                                        entry_price=None,  # À remplir si nécessaire
                                        position_size=None,  # À remplir si nécessaire
                                        position_pl_pct=None  # À remplir si nécessaire
                                    )
                                    self.candle_callback(base_candle)
                            else:
                                logging.warning(f"No valid {self.price_source.value} prices for interval {self.last_candle_time}")
                        else:
                            logging.warning(f"No ticks available for interval {self.last_candle_time}")
                    
                    # Update last candle time
                    self.last_candle_time = aligned_time
                    
                    # Clean up old data pour éviter les fuites de mémoire
                    with self.stream_lock:
                        for timestamp in list(ticks_by_interval.keys()):
                            if timestamp < aligned_time - datetime.timedelta(seconds=self.candle_interval * 10):
                                del ticks_by_interval[timestamp]
                        
                        # Aussi nettoyer les anciennes bougies
                        for timestamp in list(self.candles_by_interval.keys()):
                            if timestamp < aligned_time - datetime.timedelta(seconds=self.candle_interval * 100):
                                del self.candles_by_interval[timestamp]
                
                # Sleep to avoid high CPU usage
                time.sleep(0.1)
                
            except Exception as e:
                logging.error(f"Error in candle building from ticks thread: {e}")
                logging.error(traceback.format_exc())
                time.sleep(1)  # Sleep on error to avoid rapid looping

    def set_candle_callback(self, callback):
        """
        Définit une fonction de rappel pour traiter les bougies construites.
        
        Args:
            callback (function): Fonction à appeler avec la bougie construite.
        """
        self.candle_callback = callback


    def fetch_recent_ticks(self, count=100):
        """
        Récupère les ticks les plus récents du buffer.
        
        Args:
            count (int): Nombre maximum de ticks à récupérer
            
        Returns:
            list: Liste des ticks récents
        """
        with self.stream_lock:
            return list(self.ticks_buffer)[-count:] if self.ticks_buffer else []

    def execute_signal(self, signal):
        """
        Exécute le signal de trading (achat, vente ou liquidation) sur l'API du broker.
        """
        
        open_position = self.has_open_position()
        if signal is None:
            return
        
        
        # On ferme la position
        if signal['action'] == 'LIQUIDATE':
            return self.close_open_position()

        if signal is not None and open_position:
            logging.debug("Position déjà ouverte, pas d'action à prendre.")
            return
        
        # On place l'ordre
        return self.place_order(signal)

    def close_open_position(self):
        """
        Ferme toutes les positions ouvertes sur l'EPIC spécifié.
        """
        # On recupère toutes les positions ouvertes
        open_positions = self.ig_service.fetch_open_positions()
        positions_to_close = open_positions[open_positions['epic'] == self.epic]

        if positions_to_close.empty:
            return

        # Pour chaque position, on appelle la méthode close_open_position de l'API IG
        results = []
        for idx, pos in positions_to_close.iterrows():
            deal_id = pos['dealId']
            if pos['direction'] == 'BUY':
                direction  = 'SELL'
            elif pos['direction'] == 'SELL':
                direction  = 'BUY'
            epic       = None
            expiry     = '-'  
            level      = None
            order_type = 'MARKET'
            quote_id   = None
            size       = pos['size']
            
            try:
                result = self.ig_service.close_open_position(
                    deal_id=deal_id,
                    direction=direction,
                    epic=epic,
                    expiry=expiry,
                    level=level,
                    order_type=order_type,
                    quote_id=quote_id,
                    size=size,
                )
                logging.info(f"Position fermée (dealId: {deal_id}) : {result}")
                results.append(result)
            except Exception as e:
                logging.error(f"Erreur lors de la fermeture de la position (dealId: {deal_id}) : {e}")
                
        return results

    def place_order(self, signal):
        """
        Envoie l'ordre à l'API du broker en se basant sur le signal du bot.
        """
        if signal['action'] == 'LIQUIDATE':
            return self.close_open_position()

        tp_distance = abs(signal['take_profit']) if signal.get('take_profit') is not None else None
        sl_distance = abs(signal['stop_loss']) if signal.get('stop_loss') is not None else None

        # Si on n'a pas de distance de stop loss, on ne peut pas garantir l'arrêt
        guaranteed_stop = not sl_distance is None

        return self.ig_service.create_open_position(
            currency_code=CURRENCY,
            direction=signal['action'],
            epic=self.epic,
            expiry=EXPIRY,
            force_open=True,
            guaranteed_stop=guaranteed_stop,
            level=None,
            limit_distance=tp_distance,
            limit_level=None,
            order_type=ORDER_TYPE,
            quote_id=None,
            size=signal['quantity'],
            stop_distance=sl_distance,
            stop_level=None,
            trailing_stop=False,
            trailing_stop_increment=None
        )
        
    def has_open_position(self, direction=None):
        """
        Vérifie si une position est actuellement ouverte sur l'EPIC spécifié.
        
        Args:
            direction (str, optional): Direction de la position ('BUY' ou 'SELL'). 
                                      Si None, vérifie toute position.
        
        Returns:
            bool: True si une position est ouverte, False sinon.
        """
        # On récupère toutes les positions ouvertes
        open_positions = self.ig_service.fetch_open_positions()
        
        # Si aucune position n'est ouverte, retourne False immédiatement
        if open_positions.empty:
            return False
            
        # On filtre pour ne garder que celles qui correspondent à notre epic
        positions_for_epic = open_positions[open_positions['epic'] == self.epic]
        
        # Si direction est spécifiée, on filtre également sur la direction
        if direction is not None and not positions_for_epic.empty:
            positions_for_epic = positions_for_epic[positions_for_epic['direction'] == direction]
        
        # Si positions_for_epic est vide, alors pas de position ouverte
        return not positions_for_epic.empty
    
    def __del__(self):
        """Clean up resources when the broker instance is destroyed."""
        try:
            self.should_continue = False
            if hasattr(self, 'stream_manager'):
                logging.info("Stopping streaming subscriptions")
                self.stream_manager.stop_subscriptions()
                
            if hasattr(self, 'candle_thread') and self.candle_thread.is_alive():
                self.candle_thread.join(timeout=1)
        except Exception as e:
            logging.error(f"Error during broker cleanup: {e}")