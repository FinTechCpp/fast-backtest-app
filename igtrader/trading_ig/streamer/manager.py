import logging
from queue import Queue
from threading import Thread
import time

from lightstreamer.client import SubscriptionListener, ItemUpdate

from igtrader.trading_ig.stream import IGStreamService
from .ticker import Ticker
from .ticker import TickerSubscription
from .candler import CandleSubscription, CandleData

logger = logging.getLogger(__name__)


class StreamingManager:
    def __init__(self, service: IGStreamService):
        self._service = service
        self._subs = {}

        # setup data objects
        self._tickers = {}
        self._candles = {}  # Pour stocker les données des bougies

        # set up consumer queue
        self._queue = Queue()
        self._consumer_thread = Consumer(self._queue, self)
        self._consumer_thread.start()

    @property
    def service(self):
        return self._service

    @property
    def tickers(self):
        return self._tickers
    
    @property
    def candles(self):
        return self._candles

    def start_tick_subscription(self, epic) -> TickerSubscription:
        tick_sub = TickerSubscription(epic)
        tick_sub.addListener(TickerListener(self._queue))
        self.service.subscribe(tick_sub)
        self._subs[epic] = tick_sub
        return tick_sub

    def stop_tick_subscription(self, epic):
        subscription = self._subs.pop(epic)
        self.service.unsubscribe(subscription)

    def ticker(self, epic, timeout_length=3):
        # we won't have a ticker until at least one update is received from server,
        # let's give it a few seconds
        timeout = time.time() + timeout_length
        while True:
            logger.info(f"Waiting for ticker for '{epic}'...")
            if epic in self._tickers or time.time() > timeout:
                break
            time.sleep(0.25)
        try:
            ticker = self._tickers[epic]
        except KeyError:
            raise Exception(
                f"No ticker found for {epic} after "
                f"waiting {timeout_length} seconds - giving up"
            )
        return ticker

    def on_update(self, update):
        self._queue.put(update)

    def stop_subscriptions(self):
        logger.info("Unsubscribing from all")
        self.service.unsubscribe_all()
        self.service.disconnect()
        if self._consumer_thread:
            self._consumer_thread.join(timeout=5)
            self._consumer_thread = None

    def start_candle_subscription(self, epic, resolution="SECOND") -> CandleSubscription:
        """Subscribe to candle data for the given epic and resolution"""
        sub_key = f"{epic}:{resolution}"
        candle_sub = CandleSubscription(epic, resolution)
        candle_sub.addListener(CandleListener(self._queue))
        self.service.subscribe(candle_sub)
        self._subs[sub_key] = candle_sub
        return candle_sub

    def stop_candle_subscription(self, epic, resolution="SECOND"):
        """Unsubscribe from candle data for the given epic and resolution"""
        sub_key = f"{epic}:{resolution}"
        if sub_key in self._subs:
            subscription = self._subs.pop(sub_key)
            self.service.unsubscribe(subscription)

    def candle(self, epic, resolution="SECOND", timeout_length=3):
        """Get the latest candle data for the given epic and resolution"""
        candle_key = f"{epic}:{resolution}"
        # we won't have a candle until at least one update is received from server,
        # let's give it a few seconds
        timeout = time.time() + timeout_length
        while True:
            logger.info(f"Waiting for candle for '{candle_key}'...")
            if candle_key in self._candles or time.time() > timeout:
                break
            time.sleep(0.25)
        try:
            candle = self._candles[candle_key]
        except KeyError:
            raise Exception(
                f"No candle found for {candle_key} after "
                f"waiting {timeout_length} seconds - giving up"
            )
        return candle


class TickerListener(SubscriptionListener):
    def __init__(self, queue: Queue) -> None:
        self._queue = queue

    def onItemUpdate(self, update: ItemUpdate):
        self._queue.put(update)

    def onSubscription(self):
        logger.info("TickerListener onSubscription()")

    def onSubscriptionError(self, code, message):
        logger.info(f"TickerListener onSubscriptionError(): '{code}' {message}")

    def onUnsubscription(self):
        logger.info("TickerListener onUnsubscription()")


class CandleListener(SubscriptionListener):
    def __init__(self, queue: Queue) -> None:
        self._queue = queue

    def onItemUpdate(self, update: ItemUpdate):
        self._queue.put(update)

    def onSubscription(self):
        logger.info("CandleListener onSubscription()")

    def onSubscriptionError(self, code, message):
        logger.info(f"CandleListener onSubscriptionError(): '{code}' {message}")

    def onUnsubscription(self):
        logger.info("CandleListener onUnsubscription()")


class Consumer(Thread):
    def __init__(self, queue: Queue, manager: StreamingManager):
        super().__init__(name="ConsumerThread", daemon=True)
        self._queue = queue
        self._manager = manager

    @property
    def manager(self):
        return self._manager

    def run(self):
        logging.info("Consumer: Running")
        while True:
            try:
                item = self._queue.get()
    
                # deal with each different type of update
                name = item.getItemName()
                
                if name.startswith("CHART:"):
                    if ":TICK" in name:
                        self._handle_ticker_update(item)
                    else:
                        self._handle_candle_update(item)
            except Exception as e:
                logging.error(f"Error in consumer run: {e}")

    def _handle_ticker_update(self, item: ItemUpdate):
        name = item.getItemName()
        if name.startswith("CHART:") and ":TICK" in name:
            epic = Ticker.identifier(name)

            if epic not in self.manager.tickers:
                ticker = Ticker(epic)
                self.manager.tickers[epic] = ticker

            ticker = self.manager.tickers[epic]
            ticker.populate(item.getChangedFields())

    # Dans la classe Consumer
    def _handle_candle_update(self, item: ItemUpdate):
        try:
            name = item.getItemName()
            if name.startswith("CHART:"):
                parts = name.split(":")
                if len(parts) >= 3:
                    epic = parts[1]
                    resolution = parts[2]
                    candle_key = f"{epic}:{resolution}"
                    
                    # Obtenir et logger tous les champs disponibles, pas seulement les changés
                    all_fields = item.getFields()
                    changed_fields = item.getChangedFields()
                    
                    if candle_key not in self.manager.candles:
                        candle = CandleData(epic, resolution)
                        self.manager.candles[candle_key] = candle
                        
                    candle = self.manager.candles[candle_key]
                    candle.populate(changed_fields)
                    
                    # Log pour confirmer le traitement
                    logging.debug(f"Updated candle: {candle}")
        except Exception as e:
            logging.error(f"Error in _handle_candle_update: {e}")
            import traceback
            logging.error(traceback.format_exc())
