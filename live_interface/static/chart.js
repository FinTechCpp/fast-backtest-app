// Configuration et initialisation du graphique
const chartContainer = document.getElementById('chart-container');
const connectionStatus = document.getElementById('connection-status');
const intervalSelect = document.getElementById('interval-select');
const refreshBtn = document.getElementById('refresh-btn');

// Création du graphique
const chart = LightweightCharts.createChart(chartContainer, {
    width: chartContainer.clientWidth,
    height: chartContainer.clientHeight,
    layout: {
        background: { color: '#1e222d' },
        textColor: '#d1d4dc',
    },
    grid: {
        vertLines: { color: '#2a2e39' },
        horzLines: { color: '#2a2e39' },
    },
    crosshair: {
        mode: LightweightCharts.CrosshairMode.Normal,
    },
    timeScale: {
        borderColor: '#4c525e',
        timeVisible: true,
    },
    rightPriceScale: {
        borderColor: '#4c525e',
    },
});

// Ajout d'une série de chandeliers
const candlestickSeries = chart.addSeries(LightweightCharts.CandlestickSeries, {
    upColor: '#26a69a',
    downColor: '#ef5350',
    borderVisible: false,
    wickUpColor: '#26a69a',
    wickDownColor: '#ef5350',
});

// Variables pour stocker les données
let candleData = [];
let websocket = null;

// Fonction pour initialiser la WebSocket
function initWebSocket() {
    // Fermer la connexion existante si elle est ouverte
    if (websocket && websocket.readyState !== WebSocket.CLOSED) {
        websocket.close();
    }
    
    // Création d'une nouvelle connexion WebSocket
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    websocket = new WebSocket(wsUrl);
    
    websocket.onopen = function() {
        console.log('WebSocket connected');
        connectionStatus.textContent = 'Connected';
        connectionStatus.classList.remove('disconnected');
        connectionStatus.classList.add('connected');
    };
    
    websocket.onclose = function() {
        console.log('WebSocket disconnected');
        connectionStatus.textContent = 'Disconnected';
        connectionStatus.classList.remove('connected');
        connectionStatus.classList.add('disconnected');
        
        // Tentative de reconnexion après 5 secondes
        setTimeout(initWebSocket, 5000);
    };
    
    websocket.onerror = function(error) {
        console.error('WebSocket error:', error);
    };
    
    websocket.onmessage = function(event) {
        const message = JSON.parse(event.data);
        
        if (message.type === 'candle') {
            // Nouvelle bougie complète reçue
            const newCandle = message.data;
            
            // Conversion de la date ISO en timestamp pour Lightweight Charts
            newCandle.time = new Date(newCandle.time).getTime() / 1000;
            
            // Ajout de la nouvelle bougie aux données
            const existingIndex = candleData.findIndex(candle => candle.time === newCandle.time);
            if (existingIndex >= 0) {
                // Mise à jour d'une bougie existante
                candleData[existingIndex] = newCandle;
            } else {
                // Ajout d'une nouvelle bougie
                candleData.push(newCandle);
            }
            
            // Mise à jour du graphique
            candlestickSeries.setData(candleData);
            
            // Faire défiler vers la bougie la plus récente
            chart.timeScale().scrollToPosition(0, false);
        }
        else if (message.type === 'current_candle') {
            // Mise à jour de la bougie en cours
            const currentCandle = message.data;
            
            // Conversion de la date ISO en timestamp pour Lightweight Charts
            currentCandle.time = new Date(currentCandle.time).getTime() / 1000;
            
            // Recherche de la bougie actuelle dans les données
            const existingIndex = candleData.findIndex(candle => candle.time === currentCandle.time);
            
            if (existingIndex >= 0) {
                // Mise à jour de la bougie existante
                candleData[existingIndex] = currentCandle;
            } else {
                // Ajout de la nouvelle bougie actuelle
                candleData.push(currentCandle);
            }
            
            // Utiliser update au lieu de setData pour de meilleures performances
            // Cela ne met à jour que la dernière bougie sans redessiner tout le graphique
            candlestickSeries.update(currentCandle);
        }
    };
}

// Fonction pour charger les données historiques
async function loadHistoricalData() {
    try {
        const response = await fetch('/api/candles?limit=1000');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        // Conversion des données pour Lightweight Charts
        candleData = data.map(candle => ({
            time: new Date(candle.time).getTime() / 1000,
            open: candle.open,
            high: candle.high,
            low: candle.low,
            close: candle.close
        }));
        
        // Mise à jour du graphique
        candlestickSeries.setData(candleData);
        
        // Ajuster l'échelle de temps pour afficher toutes les données
        chart.timeScale().fitContent();
    } catch (error) {
        console.error('Error loading historical data:', error);
    }
}

// Gestion du redimensionnement de la fenêtre
function handleResize() {
    chart.resize(chartContainer.clientWidth, chartContainer.clientHeight);
}

// Initialisation
window.addEventListener('DOMContentLoaded', () => {
    // Charger les données historiques
    loadHistoricalData();
    
    // Initialiser la connexion WebSocket
    initWebSocket();
    
    // Configurer les gestionnaires d'événements
    window.addEventListener('resize', handleResize);
    
    refreshBtn.addEventListener('click', loadHistoricalData);
    
    intervalSelect.addEventListener('change', async () => {
        const newInterval = intervalSelect.value;
        // Ici, on pourrait envoyer un message WebSocket pour changer l'intervalle
        // Pour l'instant, on se contente de recharger les données
        await loadHistoricalData();
    });
});