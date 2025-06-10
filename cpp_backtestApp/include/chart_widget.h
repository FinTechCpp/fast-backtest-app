#pragma once

#include <QWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>

#include "data.hpp"
#include "trade.hpp"
#include "components/technical_indicators.h"
#include "components/backtest_results.h"

// Include ChartDirector headers
#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"

/**
 * @brief Widget qui encapsule un graphique financier ChartDirector
 */
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Enum pour les différents types de graphiques financiers
     */
    enum class ChartType {
        CandleStick,  ///< Graphique en chandeliers japonais
        HeikinAshi,   ///< Chandeliers Heikin Ashi (moyenne)
        OHLC,         ///< Barres OHLC (Open-High-Low-Close)
        Close,        ///< Ligne de prix de clôture uniquement
        
        Count         ///< Nombre total de types de graphiques
    };

    /**
     * @brief Structure qui représente une instance d'indicateur RSI
     */
    struct RSIInstance {
        int id;                 ///< Identifiant unique 
        int period;             ///< Période du RSI
        bool visible = true;    ///< Si l'indicateur est visible
        int height = 120;       ///< Hauteur du panneau
        int color = 0x800080;   ///< Couleur de la ligne principale (violet par défaut)
        double range = 20;      ///< Plage pour les niveaux de survente/surachat (70/30)
        int upperColor = 0xff6666; ///< Couleur pour la zone de surachat
        int lowerColor = 0x6666ff; ///< Couleur pour la zone de survente
        
        bool operator==(const RSIInstance& other) const {
            return id == other.id;
        }
    };

    /**
     * @brief Structure qui représente une instance d'indicateur EMA
     */
    struct EMAInstance {
        int id;                ///< Identifiant unique 
        int period;            ///< Période de l'EMA
        bool visible = true;   ///< Si l'indicateur est visible
        int color = 0x0000FF;  ///< Couleur de la ligne (bleu par défaut)
        
        bool operator==(const EMAInstance& other) const {
            return id == other.id;
        }
    };

    /**
     * @brief Structure qui représente une instance d'indicateur Stochastique
     */
    struct StochasticInstance {
        int id;                 ///< Identifiant unique 
        int fastKPeriod;        ///< Période pour calculer le %K brut
        int slowKPeriod;        ///< Période de lissage pour %K
        int slowDPeriod;        ///< Période pour calculer %D
        bool visible = true;    ///< Si l'indicateur est visible
        int height = 120;       ///< Hauteur du panneau
        int kColor = 0x0000FF;  ///< Couleur de la ligne %K (bleu par défaut)
        int dColor = 0xFF0000;  ///< Couleur de la ligne %D (rouge par défaut)
        int overboughtLevel = 80; ///< Niveau de surachat
        int oversoldLevel = 20;   ///< Niveau de survente
        
        bool operator==(const StochasticInstance& other) const {
            return id == other.id;
        }
    };
    
    // ======== Constructeurs et destructeur ========
    explicit ChartWidget(QWidget* parent = nullptr);
    ~ChartWidget() override;
    
    // ======== API Publique ========
    // Méthodes d'initialisation des données
    void setBacktestResults(const BacktestResults* results);

    // Configuration et contrôle du graphique
    void setChartType(ChartType chartType);
    ChartType getChartType() const { return m_config.chartType; }
    const std::vector<RSIInstance>& getRSIInstances() const { return m_rsiInstances; }
    const std::vector<EMAInstance>& getEMAInstances() const { return m_emaInstances; }
    const std::vector<StochasticInstance>& getStochasticInstances() const { return m_stochasticInstances; }
    RSIInstance* findRSI(int id);
    EMAInstance* findEMA(int id);
    StochasticInstance* findStochastic(int id);


    void setChartWidth(int width);
    void setShowVolume(bool show);
    void setShowTrades(bool show);
    void setShowEquity(bool show);

    void setRSIPeriod(int period);
    
    // Actions sur le graphique
    // void createChart();    ///< Crée le graphique complet avec les données actuelles
    // void updateChart();    ///< Met à jour le graphique avec les nouvelles données
    /**
     * @brief Crée ou met à jour le graphique avec les données actuelles
     * 
     * @param useViewport Si true, utiliser le viewport actuel; sinon, afficher toutes les données
     * @param preserveViewport Si true, préserver la position du viewport actuel après mise à jour
     * @return True si le graphique a été créé/mis à jour avec succès
     */
    bool updateChartDisplay(bool useViewport = true, bool preserveViewport = true);
    void clearChart();     ///< Efface le graphique et les données
    void resetZoom();      ///< Réinitialise le zoom à l'état initial
    
    // État du graphique
    bool hasValidData() const;
    bool isChartCreated() const;
    
    // Conversion de ChartType 
    static QString chartTypeToString(ChartType type);
    static ChartType stringToChartType(const QString& typeStr);

    // Resize et gestion de la vue
    void setResizing(bool isResizing) { m_isResizing = isResizing; }
    bool isResizing() const { return m_isResizing; }
    
    // Méthode pour activer/désactiver l'outil règle
    void setRulerToolEnabled(bool enabled);
    
signals:
    void chartCreated();
    void viewPortChanged();
    void mouseOverPoint(double timestamp, double price);

    // Signaux pour le RSI
    void rsiAdded(int id, int period);
    void rsiChanged(int id, int period);
    void rsiRemoved(int id);

    // Signaux pour l'EMA
    void emaAdded(int id, int period);
    void emaChanged(int id, int period);
    void emaRemoved(int id);

    // Signaux pour le Stochastique
    void stochasticAdded(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void stochasticChanged(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void stochasticRemoved(int id);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent* event);
    void onMouseClickPlotArea(QMouseEvent* event); // Nouvelle méthode
    
public slots:
    // Pour le RSI
    int addRSI(int period = 14);
    bool setRSIConfig(int id, const RSIInstance& config);
    bool removeRSI(int id);

    // Pour l'EMA
    int addEMA(int period = 20);
    bool setEMAConfig(int id, const EMAInstance& config);
    bool removeEMA(int id);

    // Pour le Stochastique
    int addStochastic(int fastKPeriod = 14, int slowKPeriod = 3, int slowDPeriod = 3);
    bool setStochasticConfig(int id, const StochasticInstance& config);
    bool removeStochastic(int id);

    // Pour gérer le redimensionnement du graphique
    // void onWindowResized(QSize newSize);

private:
    // ======== Structures de données internes ========
    /**
     * @brief Structure pour stocker les données d'équité
     */
    struct EquityData {
        std::vector<double> timestamps;
        std::vector<double> equity_values;
        std::vector<double> drawdown;
    };

    /**
     * @brief Structure pour la configuration du graphique
     */
    struct ChartConfig {
        ChartType chartType = ChartType::CandleStick;
        int chartWidth = 1200;
        int mainChartHeight = 400;
        int equityHeight = 150;
        int volumeHeight = 100;
        bool showTrades = true;
        bool showVolume = true;
        bool showEquity = true;
    };

    /**
     * @brief Structure pour stocker les données Heikin-Ashi en cache
     */
    struct HeikinAshiCache {
        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        bool isValid = false;
    };

    /**
     * @brief Structure pour stocker les indicateurs en cache
     */
    struct IndicatorCache {
        std::map<int, std::vector<double>> rsi;  // Clé: période, Valeur: données RSI
        std::map<int, std::vector<double>> ema;  // Clé: période, Valeur: données EMA
        std::map<std::tuple<int,int,int>, std::pair<std::vector<double>, std::vector<double>>> stochastic;
        bool isValid = false;
    };
    
    /**
     * @brief Structure de métadonnées pour chaque type de graphique
     */
    struct ChartTypeInfo {
        ChartType type;
        const char* name;
    };

    struct TPSLSegment {
        double startX;        // Index du point d'entrée
        double endX;          // Index du point de sortie
        double level;         // Niveau de prix (TP ou SL)
        bool isTakeProfit;    // true = TP, false = SL
        int color;            // Couleur basée sur le résultat du trade
    };

    // ======== Méthodes privées ========
    // 1. Traitement et conversion des données
    void convertBacktestData(const std::shared_ptr<be::Data>& data);
    void convertEquityCurve(const std::vector<double>& equityCurve, 
                          const std::shared_ptr<be::Data>& data);
    double dateToChartTimestamp(const be::Date& date);
    void prepareTimestampsCache();
    DoubleArray vectorToDoubleArray(const std::vector<double>& vec);
    void updateHeikinAshiCache();
    void updateIndicatorCache();

    
    // 2. Fonctions utilitaires pour les données
    template<typename T>
    std::vector<T> getVisibleDataRange(const std::vector<T>& data, int startIndex, int count);
    int calculateStartIndex(const DoubleArray& timestamps) const;
    void prepareVisibleOHLCVData(int startIndex, int endIndex, 
                               DoubleArray& timestamps, DoubleArray& open,
                               DoubleArray& high, DoubleArray& low,
                               DoubleArray& close, DoubleArray& volume);
    
    // 3. Méthodes d'initialisation et de configuration
    void setupChart();
    void setupChartViewer();
    
    // 4. Méthodes de rendu du graphique
    FinanceChart* drawChart(const DoubleArray& timestamps, 
                          const DoubleArray& highData, 
                          const DoubleArray& lowData, 
                          const DoubleArray& openData, 
                          const DoubleArray& closeData,
                          const DoubleArray& volumeData,
                          int chartWidth);
    
    // 5. Composants du graphique
    FinanceChart* initializeChart(int chartWidth);
    void addEquityCurveSection(FinanceChart* chart, const DoubleArray& timestamps, int startIndex);
    void addMainChartSection(FinanceChart* chart, int chartHeight);
    void addTradeMarkers(FinanceChart* chart, const DoubleArray& timestamps, int startIndex);
    void addTPSLSegments(XYChart* chart, const std::vector<TPSLSegment>& segments);
    FinanceChart* finalizeChart(FinanceChart* chart);
    void addMarkers(XYChart* chart, const std::vector<std::pair<double, double>>& arrows, const char* name,
                  int symbolType, int symbolSize = 5, int color = -1);

    // 6. Gestion des interactions utilisateur
    void trackFinance(MultiChart* m, int mouseX);
    
    // 7. Fonctions utilitaires pour le tracking
    void setupTrackingLayer(DrawArea* d, XYChart* c);
    std::string getOHLCLegend(XYChart* c, Layer* layer, int xIndex);
    std::vector<std::string> getIndicatorLegends(XYChart* c, Layer* layer, int xIndex);
    void drawTrackingLine(DrawArea* d, XYChart* c, int mouseX, int xValue);
    
    // ======== Membres de données ========
    // 1. Configuration
    ChartConfig m_config;
    
    // 2. Données
    std::shared_ptr<be::Data> m_backtestData; 
    std::vector<double> m_timestampsCache;
    HeikinAshiCache m_heikinAshiCache;
    IndicatorCache m_indicatorCache;
    std::vector<std::shared_ptr<be::Trade>> m_trades;
    EquityData m_equityData;

    std::vector<RSIInstance> m_rsiInstances;  ///< Instances de RSI actives
    std::vector<EMAInstance> m_emaInstances;  ///< Instances d'EMA actives
    std::vector<StochasticInstance> m_stochasticInstances; ///< Instances de Stochastique actives

    int m_nextRSIId = 1;                     ///< Prochain ID disponible pour RSI
    int m_nextEMAId = 1;                     ///< Prochain ID disponible pour EMA
    int m_nextStochasticId = 1;              ///< Prochain ID disponible pour Stochastique

    // Méthodes privées pour le RSI
    void addRSIToChart(FinanceChart* chart, const RSIInstance& rsi, int startIndex, int pointsToShow);
    void ensureRSICached(int period);

    // Méthodes privées pour l'EMA
    void addEMAToChart(FinanceChart* chart, const EMAInstance& ema, int startIndex, int pointsToShow);
    void ensureEMACached(int period);

    // Méthodes privées pour le Stochastic
    void addStochasticToChart(FinanceChart* chart, const StochasticInstance& stochastic, int startIndex, int pointsToShow);
    void ensureStochasticCached(int fastKPeriod, int slowKPeriod, int slowDPeriod);

    // 3. Composants d'interface
    QChartViewer* m_chartViewer = nullptr;
    FinanceChart* m_financeChart = nullptr;
    
    // 4. Constantes statiques
    static const std::array<ChartTypeInfo, static_cast<size_t>(ChartType::Count)> s_chartTypeData;

    bool m_isResizing = false;  ///< Indique si le widget est en cours de redimensionnement
    QSize m_pendingResize;  ///< Taille en attente de redimensionnement

    // Variables pour l'outil règle
    bool m_rulerToolEnabled;           // Si l'outil règle est activé
    bool m_rulerFirstPointSelected;    // Si le premier point a été sélectionné
    double m_rulerStartX;              // Coordonnée X du point de départ
    double m_rulerStartY;              // Coordonnée Y du point de départ
    double m_rulerEndX;                // Coordonnée X actuelle
    double m_rulerEndY;                // Coordonnée Y actuelle
    
    // Méthode pour dessiner la règle
    void drawRuler(MultiChart* chart, int mouseX, int mouseY, DrawArea* d);
};