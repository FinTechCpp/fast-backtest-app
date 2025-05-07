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
from collections import deque
import traceback
import talib
EXPIRY = '-'
CURRENCY = 'EUR'
ORDER_TYPE = 'MARKET'

class Broker:
    def __init__(self, epic=None, working_resolution=None, candle_interval=None, price_source=None):
        """
        Initialise un broker pour interagir avec l'API IG.
        
        Args:
            epic (str): Code de l'instrument sur lequel trader.
            working_resolution (str): Résolution de travail pour les données historiques.
            candle_interval (int): Intervalle en secondes pour la construction des bougies.
            price_source (str): Source de prix à utiliser ('bid', 'ask', ou 'last').
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
        self.working_resolution = working_resolution
        self.candle_interval = candle_interval  # in seconds
        
        # Validate and set price source
        if price_source not in ['bid', 'ask', 'last']:
            logging.warning(f"Source de prix '{price_source}' invalide. Utilisation de 'bid' par défaut.")
            self.price_source = 'bid'
        else:
            self.price_source = price_source
        
        logging.info(f"Utilisation des prix '{self.price_source}' pour la construction des bougies")
        
        # Streaming data structures
        self.stream_lock = threading.Lock()
        self.current_candle = None
        self.previous_candle = None
        self.ticks_buffer = deque(maxlen=1000)  # Store recent ticks
        self.last_tick_time = None
        
        # Initialize streaming connection
        self._setup_streaming()
        
        # Start candle builder thread
        self.should_continue = True
        self.candle_thread = threading.Thread(target=self._build_candles_loop, daemon=True)
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
            
    def _setup_streaming(self):
        """Initialize streaming connection and subscribe to candle data."""
        try:
            # Ajouter des logs détaillés
            logging.debug("Creating streaming service...")
            # Create streaming service
            self.stream_service = IGStreamService(self.ig_service)
            
            # Set account number from config
            self.stream_service.acc_number = config.acc_number
            
            # Create streaming session with version 3 (same as main session)
            logging.debug("Creating streaming session...")
            self.stream_service.create_session(version="3")
            logging.info("Streaming session created")
            
            # Create streaming manager
            logging.debug("Creating streaming manager...")
            self.stream_manager = StreamingManager(self.stream_service)
            
            # Subscribe to SECOND candle data for the specified epic
            if self.epic:
                # Attendre plus longtemps que la connexion soit bien établie
                logging.debug("Waiting for connection to stabilize...")
                time.sleep(2)
                
                # Log des paramètres de souscription
                logging.debug(f"Creating subscription for {self.epic} SECOND candles with fields: {CandleSubscription.CANDLE_FIELDS}")
                
                # Souscription avec plus de logs
                self.candle_subscription = self.stream_manager.start_candle_subscription(self.epic, "SECOND")
                logging.info(f"Started SECOND candle subscription for {self.epic}")
                
                # Vérifier que la souscription est active
                candle_key = f"{self.epic}:SECOND"
                timeout = time.time() + 10  # 10 secondes de timeout
                while time.time() < timeout:
                    if hasattr(self.stream_manager, 'candles') and candle_key in self.stream_manager.candles:
                        logging.info(f"Subscription active for {candle_key}")
                        break
                    time.sleep(0.5)
                    
                # Vérifier si nous sommes toujours dans la boucle après timeout
                if time.time() >= timeout:
                    logging.warning(f"Subscription timeout for {candle_key}")
        except Exception as e:
            logging.error(f"Error setting up streaming: {e}")
            logging.error(f"Details: {traceback.format_exc()}")
            raise
    
    def _build_candles_loop(self):
        """Background thread that continuously builds candles from SECOND candle data."""
        logging.info("Starting candle building thread")
        last_candle_time = None
        second_candles_buffer = {}  # Buffer pour stocker les candles secondaires
        
        while self.should_continue:
            try:
                now = datetime.datetime.now()
                aligned_time = self._align_time_to_interval(now)
                
                if last_candle_time is None:
                    last_candle_time = aligned_time - datetime.timedelta(seconds=self.candle_interval)
                
                # Process current SECOND candle if available
                candle_key = f"{self.epic}:SECOND"
                if hasattr(self, 'stream_manager') and hasattr(self.stream_manager, 'candles'):
                    candle_data = self.stream_manager.candles.get(candle_key)
                    
                    if candle_data:
                        if hasattr(candle_data, 'timestamp') and candle_data.timestamp:
                            # Stocker ou mettre à jour la bougie dans le buffer en utilisant la date de la bougie comme clé
                            # (et non le timestamp de réception)
                            candle_datetime = candle_data.timestamp
                            with self.stream_lock:
                                second_candles_buffer[candle_datetime] = candle_data
                        else:
                            logging.warning(f"Candle data has no timestamp: {candle_data}")
                
                # Check if it's time to create a new candle
                if aligned_time > last_candle_time:
                    # Process SECOND candles that belong to the previous interval
                    candles_in_interval = []
                    
                    # Debug - log all candles in buffer
                    logging.debug(f"Buffer contains {len(second_candles_buffer)} candles")
                    for ts, cd in sorted(second_candles_buffer.items()):
                        logging.debug(f"Buffer candle: {ts} - {cd}")
                    
                    with self.stream_lock:
                        # Get all second candles from buffer that fall within the interval
                        for timestamp, candle_data in sorted(second_candles_buffer.items()):
                            if last_candle_time <= timestamp < aligned_time:
                                candles_in_interval.append(candle_data)
                                logging.debug(f"Added candle to interval: {timestamp}")
                    
                    # Log the number of candles found for this interval
                    logging.info(f"Found {len(candles_in_interval)} SECOND candles for interval {last_candle_time} to {aligned_time}")
                    
                    # Build a new candle if we have second candles
                    if candles_in_interval:
                        # Sorting to ensure correct order
                        candles_in_interval.sort(key=lambda x: x.timestamp)
                        
                        # Determine which price fields to use based on price_source setting
                        price_source_prefix = 'bid_'
                        if self.price_source == 'ask':
                            price_source_prefix = 'ofr_'
                        elif self.price_source == 'mid':
                            price_source_prefix = 'mid_'
                        
                        # Initialize the new candle with data from the first second candle
                        first_candle = candles_in_interval[0]
                        open_price = getattr(first_candle, f"{price_source_prefix}open")
                        
                        # Explicitly log all values to identify any problems
                        all_highs = [getattr(c, f"{price_source_prefix}high") for c in candles_in_interval]
                        all_lows = [getattr(c, f"{price_source_prefix}low") for c in candles_in_interval]
                        logging.debug(f"All highs: {all_highs}")
                        logging.debug(f"All lows: {all_lows}")
                        
                        high_price = max(all_highs)
                        low_price = min(all_lows)
                        
                        last_candle = candles_in_interval[-1]
                        close_price = getattr(last_candle, f"{price_source_prefix}close")
                        
                        new_candle = {
                            'date': last_candle_time,
                            'Open': open_price,
                            'High': high_price,
                            'Low': low_price,
                            'Close': close_price
                        }
                        
                        # Update candles
                        with self.stream_lock:
                            if self.current_candle:
                                self.previous_candle = self.current_candle.copy()
                            self.current_candle = new_candle
                            
                        logging.info(f"Built new {self.price_source} candle for {last_candle_time}: "
                                    f"O={open_price:.5f}, H={high_price:.5f}, L={low_price:.5f}, C={close_price:.5f}")
                    else:
                        logging.warning(f"No valid {self.price_source} candles in interval {last_candle_time} to {aligned_time}")
                    
                    # Update last candle time
                    last_candle_time = aligned_time
                    
                    # Clear processed second candles from the buffer to prevent memory leaks
                    with self.stream_lock:
                        for timestamp in list(second_candles_buffer.keys()):
                            if timestamp < aligned_time:
                                del second_candles_buffer[timestamp]
                
                # Sleep to avoid high CPU usage
                time.sleep(0.1)
                
            except Exception as e:
                logging.error(f"Error in candle building thread: {e}")
                import traceback
                logging.error(traceback.format_exc())
                time.sleep(1)  # Sleep on error to avoid rapid looping

    def fetch_previous_and_current_candles(self):
        """
        Récupère les deux dernières bougies construites à partir des données de streaming.
        Retourne un tuple (bougie_precedente, bougie_actuelle).
        """
        with self.stream_lock:
            if self.current_candle is None:
                logging.warning("No streaming candles available yet, waiting for streaming data...")
                # Ne pas utiliser le fallback REST, car il ne comprend pas "SECOND"
                # return self._fetch_candles_rest_fallback()
                return None, None
                    
            current = self.current_candle.copy() if self.current_candle else None
            previous = self.previous_candle.copy() if self.previous_candle else None
            
        # Check if we have valid data
        if not current or not previous:
            logging.warning("Incomplete streaming candle data, waiting for more data...")
            # Ne pas utiliser le fallback REST
            # return self._fetch_candles_rest_fallback()
            return None, None
                
        return previous, current

    def _fetch_candles_rest_fallback(self):
        """Fallback to REST API when streaming data is not available."""
        # Note: Pour 'last', on doit utiliser 'bid' car l'API REST ne fournit pas de prix 'last'
        price_col = 'bid'  # default
        if self.price_source == 'ask':
            price_col = 'ask'
            
        logging.info(f"Using REST API as fallback for {price_col} candles")
        
        try:
            results = self.ig_service.fetch_historical_prices_by_epic(
                epic=self.epic,
                resolution=self.working_resolution,
                numpoints=2)
            
            data = results['prices'][[price_col]].copy()
            data.columns = data.columns.get_level_values(1)
    
            candle_previous = {
                'date': data.index[-2],
                'Open': data['Open'].iloc[-2],
                'High': data['High'].iloc[-2],
                'Low': data['Low'].iloc[-2],
                'Close': data['Close'].iloc[-2]
            }
            
            candle_current = {
                'date': data.index[-1],
                'Open': data['Open'].iloc[-1],
                'High': data['High'].iloc[-1],
                'Low': data['Low'].iloc[-1],
                'Close': data['Close'].iloc[-1]
            }
            
            return candle_previous, candle_current
        except Exception as e:
            logging.error(f"Error in REST API fallback: {e}")
            return None, None

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