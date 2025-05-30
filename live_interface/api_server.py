import logging
import asyncio
import uvicorn
import json
import math
from datetime import datetime
from contextlib import asynccontextmanager
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse
from pydantic import BaseModel
from typing import List, Dict, Any

from ig_candle_service import IGCandleService, BaseCandle
from ig_candle_service import CandleStorage

# Variable globale pour stocker le service de bougies
candle_service = None

# Variable globale pour le stockage
candle_storage = CandleStorage()

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Code exécuté au démarrage
    global candle_service
    
    # Configuration des paramètres
    EPIC = "IX.D.NASDAQ.IFE.IP"  # Instrument à surveiller
    CANDLE_INTERVAL = 60  # Intervalle en secondes (1 minute)
    
    logging.info("Démarrage du service de bougies...")
    # Démarrer le service de bougies
    candle_service = start_candle_service(epic=EPIC, candle_interval=CANDLE_INTERVAL)
    logging.info("Service de bougies démarré avec succès")
    
    yield  # Séparation entre le code de démarrage et d'arrêt
    
    # Code exécuté à l'arrêt
    if candle_service:
        logging.info("Arrêt du service de bougies...")
        candle_service.stop()
        logging.info("Service de bougies arrêté")

# Initialisation de l'application FastAPI avec le gestionnaire de cycle de vie
app = FastAPI(title="IG Trading Data API", lifespan=lifespan)

# Configuration des répertoires statiques pour servir le frontend
app.mount("/static", StaticFiles(directory="static"), name="static")

# Classe pour la sérialisation des bougies
class CandleModel(BaseModel):
    time: str  # Format ISO pour la compatibilité avec Lightweight Charts
    open: float
    high: float
    low: float
    close: float

# Classe pour gérer les connexions WebSocket
class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket):
        self.active_connections.remove(websocket)

    async def send_json(self, message: Dict[str, Any]):
        disconnected = []
        for connection in self.active_connections:
            try:
                await connection.send_json(message)
            except Exception:
                disconnected.append(connection)
        
        # Nettoyer les connexions déconnectées
        for conn in disconnected:
            self.active_connections.remove(conn)

# Créer une instance globale du gestionnaire de connexions
manager = ConnectionManager()

# Convertir une bougie en format compatible avec Lightweight Charts
def candle_to_model(candle: BaseCandle) -> CandleModel:
    # Vérifier si la bougie contient des valeurs NaN
    if (math.isnan(candle.Open) or math.isnan(candle.High) or 
        math.isnan(candle.Low) or math.isnan(candle.Close)):
        
        # Si la bougie n'est pas valide, retourner None
        return None
    
    # Si toutes les valeurs sont valides, créer le modèle
    return CandleModel(
        time=candle.date.isoformat(),
        open=candle.Open,
        high=candle.High,
        low=candle.Low,
        close=candle.Close
    )

# Callback pour les nouvelles bougies
async def on_new_candle(candle: BaseCandle):
    candle_model = candle_to_model(candle)
    if candle_model:  # Vérifier que le modèle n'est pas None
        # Stocker la bougie
        candle_data = candle_model.model_dump()
        candle_storage.add_candle(candle_data)
        
        # Envoyer aux clients connectés
        await manager.send_json({
            "type": "candle",
            "data": candle_data
        })
        
# Callback pour les mises à jour de tick sur la bougie en cours
async def on_tick_update(candle: BaseCandle):
    candle_model = candle_to_model(candle)
    if candle_model:  # Vérifier que le modèle n'est pas None
        await manager.send_json({
            "type": "current_candle",
            "data": candle_model.model_dump()
        })

# Route pour la page d'accueil
@app.get("/", response_class=HTMLResponse)
async def get_html():
    with open("static/index.html", "r") as file:
        return file.read()

# Route pour récupérer les dernières bougies
@app.get("/api/candles", response_model=List[CandleModel])
async def get_candles(limit: int = 100):
    # Récupérer les bougies depuis le stockage persistent
    stored_candles = candle_storage.get_candles(limit)
    
    # Récupérer aussi les nouvelles bougies du service si disponible
    if candle_service:
        candles = candle_service.get_latest_candles(limit)
        for candle in candles:
            model = candle_to_model(candle)
            if model:
                # Stocker les nouvelles bougies
                candle_storage.add_candle(model.model_dump())
                
    return stored_candles

# WebSocket pour les mises à jour en temps réel
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            # Attendre des messages du client (comme des commandes ou des configurations)
            data = await websocket.receive_text()
            # On pourrait traiter les messages ici si nécessaire
            await asyncio.sleep(1)
    except WebSocketDisconnect:
        manager.disconnect(websocket)


def start_candle_service(epic="IX.D.NASDAQ.IFE.IP", candle_interval=60):
    global candle_service
    
    # Initialiser le service de bougies
    candle_service = IGCandleService(epic=epic, candle_interval=candle_interval)
    
    # Obtenir l'event loop principal
    loop = asyncio.get_running_loop()
    
    # Enregistrer le callback pour les nouvelles bougies
    def candle_callback(candle):
        try:
            # Utiliser run_coroutine_threadsafe pour exécuter des coroutines depuis un autre thread
            asyncio.run_coroutine_threadsafe(on_new_candle(candle), loop)
        except Exception as e:
            logging.error(f"Erreur dans candle_callback: {e}")
    
    candle_service.register_candle_callback(candle_callback)
    
    def tick_callback(candle):
        try:
            logging.debug(f"Tick reçu: {candle.date} - O:{candle.Open} H:{candle.High} L:{candle.Low} C:{candle.Close}")
            # Utiliser run_coroutine_threadsafe pour exécuter des coroutines depuis un autre thread
            asyncio.run_coroutine_threadsafe(on_tick_update(candle), loop)
        except Exception as e:
            logging.error(f"Erreur dans tick_callback: {e}")
    
    candle_service.register_tick_callback(tick_callback)
    
    return candle_service

# Point d'entrée pour démarrer le serveur
if __name__ == "__main__":
    # Configurer le logging avant tout
    logging.basicConfig(level=logging.INFO, 
                      format='%(asctime)s - %(levelname)s - %(message)s')
    
    # Démarrer le serveur
    uvicorn.run(app, host="0.0.0.0", port=8000)