#!/usr/bin/env python3
# filepath: /home/maxime/ig-trading-bot/scripts/live_Display_IG.py

import sys
import os
import logging
import datetime
import time
import threading
import math
import csv
from collections import deque
from dataclasses import dataclass
from typing import Optional
from queue import Queue, Empty
from pathlib import Path

# Add project paths
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(project_root)

# Configuration du logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('live_display.log')
    ]
)

from igtrader.WrapperIGAPI.trading_ig.rest import IGService
from igtrader.WrapperIGAPI.trading_ig.config import config
from igtrader.WrapperIGAPI.trading_ig.stream import IGStreamService
from igtrader.WrapperIGAPI.trading_ig.streamer.manager import StreamingManager

@dataclass
class Tick:
    """Structure pour stocker un tick"""
    timestamp: datetime.datetime
    bid: float
    ask: float
    last: Optional[float] = None
    epic: str = ""

@dataclass
class Candle:
    """Structure pour stocker une bougie OHLC avec spread"""
    timestamp: datetime.datetime
    open: float
    high: float
    low: float
    close: float
    tick_count: int = 0
    avg_spread: float = 0.0  # Spread moyen pendant l'intervalle

class CSVWriter:
    """Gestionnaire d'écriture CSV optimisé pour les gros volumes - Un seul fichier"""
    
    def __init__(self, epic: str, candle_interval: int, csv_filename: str = None):
        self.epic = epic.replace(".", "_")
        self.candle_interval = candle_interval
        
        # Nom du fichier CSV unique
        if csv_filename:
            self.csv_file = Path(csv_filename)
        else:
            today = datetime.datetime.now().strftime("%Y-%m-%d")
            # Résoudre le chemin absolu au lieu d'utiliser ~
            project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            market_data_dir = os.path.join(project_root, "marketData")
            
            # Créer le répertoire s'il n'existe pas
            os.makedirs(market_data_dir, exist_ok=True)
            
            filename = f"{self.epic}_{self.candle_interval}secs_{today}_to_MID.csv"
            self.csv_file = Path(os.path.join(market_data_dir, filename))
    
        # Queue pour les bougies à écrire
        self.write_queue = Queue()
        
        # Thread d'écriture
        self.writer_thread = None
        self.running = True
        
        # Buffer d'écriture pour optimiser les I/O
        self.write_buffer = []
        self.buffer_size = 50  # Écrire par batch de 50 bougies
        self.last_flush_time = time.time()
        self.flush_interval = 15  # Forcer un flush toutes les 15 secondes
        
        # Lock pour l'écriture thread-safe
        self.write_lock = threading.Lock()
        
        # Initialiser le fichier CSV avec les en-têtes si nécessaire
        self._ensure_csv_headers()
        
        logging.info(f"CSV Writer initialisé - Fichier: {self.csv_file}")

    def _ensure_csv_headers(self):
        """S'assure que le fichier CSV a les en-têtes appropriés"""
        if not self.csv_file.exists():
            try:
                with open(self.csv_file, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)
                    writer.writerow(['date', 'open', 'high', 'low', 'close', 'spread', 'ticks'])
                logging.info(f"Nouveau fichier CSV créé: {self.csv_file}")
            except Exception as e:
                logging.error(f"Erreur lors de la création du fichier CSV: {e}")

    def add_candle(self, candle: Candle):
        """Ajoute une bougie à la queue d'écriture (non-bloquant)"""
        try:
            self.write_queue.put_nowait(candle)
        except:
            logging.warning("Queue d'écriture CSV pleine, bougie ignorée")

    def _writer_thread_func(self):
        """Thread dédié à l'écriture CSV"""
        logging.info("Thread d'écriture CSV démarré")
        
        while self.running:
            try:
                # Récupérer une bougie avec timeout
                try:
                    candle = self.write_queue.get(timeout=1.0)
                    self.write_buffer.append(candle)
                    self.write_queue.task_done()
                except Empty:
                    pass

                # Conditions de flush du buffer
                current_time = time.time()
                should_flush = (
                    len(self.write_buffer) >= self.buffer_size or
                    (self.write_buffer and current_time - self.last_flush_time > self.flush_interval)
                )

                if should_flush:
                    self._flush_buffer()
                    self.last_flush_time = current_time

            except Exception as e:
                logging.error(f"Erreur dans le thread d'écriture CSV: {e}")
                time.sleep(1)

        # Flush final à l'arrêt
        if self.write_buffer:
            self._flush_buffer()

    def _flush_buffer(self):
        """Écrit toutes les bougies du buffer dans le fichier CSV"""
        if not self.write_buffer:
            return
    
        with self.write_lock:
            try:
                # Ouvrir le fichier en mode append
                with open(self.csv_file, 'a', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)
                    
                    for candle in self.write_buffer:
                        # Format avec timezone UTC
                        timestamp_str = candle.timestamp.strftime("%Y-%m-%d %H:%M:%S+00:00")
                        writer.writerow([
                            timestamp_str,
                            f"{candle.open:.2f}",
                            f"{candle.high:.2f}",
                            f"{candle.low:.2f}",
                            f"{candle.close:.2f}",
                            f"{candle.avg_spread:.2f}",
                            candle.tick_count
                        ])
                
                total_written = len(self.write_buffer)
                logging.debug(f"CSV: {total_written} bougies écrites dans {self.csv_file}")
                self.write_buffer.clear()
                
            except Exception as e:
                logging.error(f"Erreur lors de l'écriture dans {self.csv_file}: {e}")

    def start(self):
        """Démarre le thread d'écriture"""
        self.writer_thread = threading.Thread(target=self._writer_thread_func, daemon=True)
        self.writer_thread.start()
        logging.info("Thread d'écriture CSV démarré")

    def stop(self):
        """Arrête le thread d'écriture"""
        self.running = False
        if self.writer_thread and self.writer_thread.is_alive():
            self.writer_thread.join(timeout=5)
        logging.info("Thread d'écriture CSV arrêté")

class LiveTickDisplay:
    def __init__(self, epic: str, candle_interval: int = 60, csv_filename: str = None):
        """
        Initialise le display de ticks en temps réel
        
        Args:
            epic: Code de l'instrument (ex: "IX.D.NASDAQ.IFE.IP")
            candle_interval: Intervalle en secondes pour les bougies (défaut: 60s)
            csv_filename: Nom du fichier CSV pour l'historique (optionnel)
        """
        self.epic = epic
        self.candle_interval = candle_interval
        self.running = True
        
        # Queue thread-safe pour les ticks
        self.tick_queue = Queue()
        
        # Buffer pour construire les bougies
        self.current_candle_data = deque()
        self.last_candle_time = None
        
        # CSV Writer
        self.csv_writer = CSVWriter(epic, candle_interval, csv_filename)
        
        # Threads
        self.tick_processor_thread = None
        self.candle_builder_thread = None
        
        # Services IG
        self.ig_service = None
        self.stream_service = None
        self.stream_manager = None
        
        logging.info(f"Initialisation pour EPIC: {epic}, intervalle: {candle_interval}s")

    def _setup_ig_services(self):
        """Configure les services IG REST et Streaming"""
        try:
            # Service REST
            self.ig_service = IGService(
                config.username,
                config.password,
                config.api_key,
                config.acc_type,
                acc_number=config.acc_number
            )
            self.ig_service.create_session(version='3')
            logging.info("Service REST IG initialisé")

            # Service Streaming
            self.stream_service = IGStreamService(self.ig_service)
            self.stream_service.acc_number = config.acc_number
            self.stream_service.create_session(version="3")
            logging.info("Service Streaming IG initialisé")

            # Manager de streaming
            self.stream_manager = StreamingManager(self.stream_service)
            logging.info("StreamingManager initialisé")

        except Exception as e:
            logging.error(f"Erreur lors de l'initialisation des services IG: {e}")
            raise

    def _on_tick_update(self, tick_data):
        """Callback appelé à chaque nouveau tick reçu"""
        try:
            # Extraire les données du tick
            if hasattr(tick_data, 'timestamp') and tick_data.timestamp:
                tick = Tick(
                    timestamp=tick_data.timestamp,
                    bid=tick_data.bid if hasattr(tick_data, 'bid') and not math.isnan(tick_data.bid) else 0.0,
                    ask=tick_data.offer if hasattr(tick_data, 'offer') and not math.isnan(tick_data.offer) else 0.0,
                    last=tick_data.last_traded_price if hasattr(tick_data, 'last_traded_price') and not math.isnan(tick_data.last_traded_price) else None,
                    epic=self.epic
                )
                
                # Log du tick en niveau DEBUG
                logging.debug(f"TICK - {tick.epic} - {tick.timestamp} - Bid: {tick.bid:.5f}, Ask: {tick.ask:.5f}, Last: {tick.last}")
                
                # Ajouter le tick à la queue pour traitement
                self.tick_queue.put(tick)
                
        except Exception as e:
            logging.error(f"Erreur lors du traitement du tick: {e}")

    def _tick_processor_thread_func(self):
        """Thread qui traite les ticks en continu"""
        logging.info("Thread de traitement des ticks démarré")
        
        while self.running:
            try:
                # Récupérer un tick de la queue (bloquant avec timeout)
                tick = self.tick_queue.get(timeout=1.0)
                
                # Ajouter le tick aux données de la bougie courante
                self.current_candle_data.append(tick)
                
                # Signaler que la tâche est terminée
                self.tick_queue.task_done()
                
            except Empty:
                # Timeout normal - continuer
                continue
            except Exception as e:
                logging.error(f"Erreur dans le thread de traitement des ticks: {e}")

    def _align_time_to_interval(self, dt: datetime.datetime) -> datetime.datetime:
        """Aligne un datetime sur l'intervalle de bougie"""
        total_seconds = dt.hour * 3600 + dt.minute * 60 + dt.second
        aligned_seconds = (total_seconds // self.candle_interval) * self.candle_interval
        
        return dt.replace(
            hour=aligned_seconds // 3600,
            minute=(aligned_seconds % 3600) // 60,
            second=aligned_seconds % 60,
            microsecond=0
        )

    def _build_candle_from_ticks(self, ticks: list) -> Optional[Candle]:
        """Construit une bougie OHLC à partir d'une liste de ticks"""
        if not ticks:
            return None
            
        # Utiliser les prix mid (moyenne bid/ask) pour construire la bougie
        prices = []
        spreads = []
        
        for tick in ticks:
            if tick.bid > 0 and tick.ask > 0:
                mid_price = (tick.bid + tick.ask) / 2.0
                spread = tick.ask - tick.bid
                prices.append(mid_price)
                spreads.append(spread)
        
        if not prices:
            return None
            
        # Calculer le spread moyen
        avg_spread = sum(spreads) / len(spreads) if spreads else 0.0
            
        return Candle(
            timestamp=ticks[0].timestamp,
            open=prices[0],
            high=max(prices),
            low=min(prices),
            close=prices[-1],
            tick_count=len(prices),
            avg_spread=avg_spread
        )

    def _candle_builder_thread_func(self):
        """Thread qui construit les bougies à partir des ticks"""
        logging.info("Thread de construction des bougies démarré")
        
        while self.running:
            try:
                now = datetime.datetime.now()
                aligned_time = self._align_time_to_interval(now)
                
                # Initialiser le temps de la première bougie
                if self.last_candle_time is None:
                    self.last_candle_time = aligned_time - datetime.timedelta(seconds=self.candle_interval)
                
                # Vérifier s'il est temps de créer une nouvelle bougie
                if aligned_time > self.last_candle_time:
                    # Collecter tous les ticks de l'intervalle précédent
                    candle_ticks = []
                    current_data_copy = list(self.current_candle_data)
                    
                    for tick in current_data_copy:
                        tick_candle_time = self._align_time_to_interval(tick.timestamp)
                        if tick_candle_time == self.last_candle_time:
                            candle_ticks.append(tick)
                    
                    # Construire la bougie si nous avons des ticks
                    if candle_ticks:
                        candle = self._build_candle_from_ticks(candle_ticks)
                        if candle:
                            # Log de la bougie en niveau INFO
                            logging.info(
                                f"CANDLE - {self.epic} - {candle.timestamp} - "
                                f"O: {candle.open:.5f}, H: {candle.high:.5f}, "
                                f"L: {candle.low:.5f}, C: {candle.close:.5f}, "
                                f"Spread: {candle.avg_spread:.5f}, Ticks: {candle.tick_count}"
                            )
                            
                            # Enregistrer la bougie en CSV (asynchrone, zéro latence)
                            self.csv_writer.add_candle(candle)
                            
                    else:
                        logging.debug(f"Aucun tick pour l'intervalle {self.last_candle_time}")
                    
                    # Nettoyer les anciens ticks
                    cutoff_time = aligned_time - datetime.timedelta(seconds=self.candle_interval * 2)
                    while self.current_candle_data and self.current_candle_data[0].timestamp < cutoff_time:
                        self.current_candle_data.popleft()
                    
                    # Mettre à jour le temps de la dernière bougie
                    self.last_candle_time = aligned_time
                
                # Petit délai pour éviter une consommation CPU excessive
                time.sleep(0.1)
                
            except Exception as e:
                logging.error(f"Erreur dans le thread de construction des bougies: {e}")
                time.sleep(1)

    def start_subscription(self):
        """Démarre la souscription aux ticks"""
        try:
            logging.info(f"Démarrage de la souscription pour {self.epic}")
            
            # Attendre que la connexion soit stable
            time.sleep(2)
            
            # Démarrer la souscription aux ticks
            self.stream_manager.start_tick_subscription(self.epic)
            
            # Attendre que la souscription soit active
            timeout = time.time() + 10
            while time.time() < timeout:
                if hasattr(self.stream_manager, 'tickers') and self.epic in self.stream_manager.tickers:
                    logging.info(f"Souscription active pour {self.epic}")
                    break
                time.sleep(0.5)
            else:
                raise Exception(f"Timeout lors de la souscription à {self.epic}")
                
        except Exception as e:
            logging.error(f"Erreur lors du démarrage de la souscription: {e}")
            raise

    def _monitor_ticks(self):
        """Monitor les ticks et appelle le callback"""
        logging.info("Monitoring des ticks démarré")
        
        while self.running:
            try:
                if (hasattr(self.stream_manager, 'tickers') and 
                    self.epic in self.stream_manager.tickers):
                    
                    ticker_data = self.stream_manager.tickers[self.epic]
                    if hasattr(ticker_data, 'timestamp') and ticker_data.timestamp:
                        self._on_tick_update(ticker_data)
                
                # Petit délai pour éviter une consommation CPU excessive
                time.sleep(0.05)  # 50ms
                
            except Exception as e:
                logging.error(f"Erreur dans le monitoring des ticks: {e}")
                time.sleep(1)

    def start(self):
        """Démarre le système complet"""
        try:
            logging.info("Démarrage du système de display des ticks")
            
            # Démarrer le CSV writer
            self.csv_writer.start()
            
            # Initialiser les services IG
            self._setup_ig_services()
            
            # Démarrer la souscription
            self.start_subscription()
            
            # Démarrer les threads de traitement
            self.tick_processor_thread = threading.Thread(
                target=self._tick_processor_thread_func, 
                daemon=True
            )
            self.tick_processor_thread.start()
            
            self.candle_builder_thread = threading.Thread(
                target=self._candle_builder_thread_func, 
                daemon=True
            )
            self.candle_builder_thread.start()
            
            # Démarrer le monitoring des ticks
            monitor_thread = threading.Thread(
                target=self._monitor_ticks, 
                daemon=True
            )
            monitor_thread.start()
            
            logging.info("Système démarré - En attente des ticks...")
            
            # Boucle principale
            while self.running:
                time.sleep(1)
                
        except KeyboardInterrupt:
            logging.info("Arrêt demandé par l'utilisateur")
        except Exception as e:
            logging.error(f"Erreur dans le système principal: {e}")
        finally:
            self.stop()

    def stop(self):
        """Arrête le système"""
        logging.info("Arrêt du système...")
        self.running = False
        
        try:
            # Arrêter le CSV writer en premier pour sauvegarder les données
            self.csv_writer.stop()
            
            if self.stream_manager:
                self.stream_manager.stop_subscriptions()
                
            if self.stream_service:
                self.stream_service.disconnect()
                
        except Exception as e:
            logging.error(f"Erreur lors de l'arrêt: {e}")

def main():
    """Fonction principale"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Display des ticks IG en temps réel avec sauvegarde CSV")
    parser.add_argument(
        "--epic", 
        default="IX.D.NASDAQ.IFE.IP", 
        help="Code EPIC de l'instrument (défaut: IX.D.NASDAQ.IFE.IP)"
    )
    parser.add_argument(
        "--interval", 
        type=int, 
        default=10, 
        help="Intervalle des bougies en secondes (défaut: 10)"
    )
    parser.add_argument(
        "--csv-file",
        default=None,
        help="Nom du fichier CSV pour l'historique (défaut: auto-généré)"
    )
    
    args = parser.parse_args()
    
    # Créer et démarrer le système
    display = LiveTickDisplay(
        epic=args.epic, 
        candle_interval=args.interval,
        csv_filename=args.csv_file
    )
    
    try:
        display.start()
    except KeyboardInterrupt:
        logging.info("Arrêt demandé")
    finally:
        display.stop()

if __name__ == "__main__":
    main()