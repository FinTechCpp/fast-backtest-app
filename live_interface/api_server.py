import logging
import asyncio
import uvicorn
import json
from datetime import datetime
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse
from pydantic import BaseModel
from typing import List, Dict, Any

from ig_candle_service import IGCandleService, BaseCandle

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

# Initialisation de l'application FastAPI
app = FastAPI(title="IG Trading Data API")
manager = ConnectionManager()

# Configuration des répertoires statiques pour servir le frontend
app.mount("/static", StaticFiles(directory="static"), name="static")

# Variable globale pour stocker le service de bougies
candle_service = None

# Convertir une bougie en format compatible avec Lightweight Charts
def candle_to_model(candle: BaseCandle) -> CandleModel:
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
    await manager.send_json({
        "type": "candle",
        "data": candle_model.model_dump()
    })
        
# Callback pour les mises à jour de tick sur la bougie en cours
async def on_tick_update(candle: BaseCandle):
    candle_model = candle_to_model(candle)
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
    if candle_service:
        candles = candle_service.get_latest_candles(limit)
        return [candle_to_model(candle) for candle in candles]
    return []

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
    
    # Enregistrer le callback pour les nouvelles bougies
    def candle_callback(candle):
        asyncio.run(on_new_candle(candle))
    
    candle_service.register_candle_callback(candle_callback)
    
    # Enregistrer le callback pour les mises à jour de tick
    def tick_callback(candle):
        asyncio.run(on_tick_update(candle))
    
    candle_service.register_tick_callback(tick_callback)
    
    return candle_service

# Point d'entrée pour démarrer le serveur
if __name__ == "__main__":
    # Configuration des paramètres
    EPIC = "IX.D.NASDAQ.IFE.IP"  # Instrument à surveiller
    CANDLE_INTERVAL = 60  # Intervalle en secondes (1 minute)
    
    # Démarrer le service de bougies
    start_candle_service(epic=EPIC, candle_interval=CANDLE_INTERVAL)
    
    # Démarrer le serveur FastAPI
    uvicorn.run(app, host="0.0.0.0", port=8000)