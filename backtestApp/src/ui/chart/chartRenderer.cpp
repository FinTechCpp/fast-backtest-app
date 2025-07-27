#include "ui/chart/chartRenderer.h"
#include "ui/chart/chartDataManager.h"
#include <QDebug>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

ChartRenderer::ChartRenderer() : m_financeChart(nullptr) {
}

ChartRenderer::~ChartRenderer() {
}

void ChartRenderer::createOrUpdateChart(
    QChartViewer* viewer,
    const ChartDataManager& dataManager,
    const ChartConfiguration& config,
    const ChartDataManager::AggregationInfo& aggregationInfo)
{
    // Extraire les données selon le niveau d'agrégation
    DoubleArray timestamps, openData, highData, lowData, closeData, volumeData;
    
    if (aggregationInfo.level == AggregationLevel::Raw) {
        // Utiliser les données brutes directement
        const auto& backtestData = dataManager.getBacktestData();
        int startIndex = aggregationInfo.startIndex;
        int pointCount = aggregationInfo.pointCount;
        
        timestamps = DoubleArray(&dataManager.getTimestamps()[startIndex], pointCount);
        volumeData = DoubleArray(&backtestData->getVolume()[startIndex], pointCount);
        
        // Déterminer quel type de données afficher (standard ou Heikin-Ashi)
        if (config.chartType == ChartDataManager::ChartType::HeikinAshi) {
            const auto& heikinAshiCache = dataManager.getHeikinAshiCache();
            
            openData = DoubleArray(&heikinAshiCache.open[startIndex], pointCount);
            highData = DoubleArray(&heikinAshiCache.high[startIndex], pointCount);
            lowData = DoubleArray(&heikinAshiCache.low[startIndex], pointCount);
            closeData = DoubleArray(&heikinAshiCache.close[startIndex], pointCount);
        } else {
            openData = DoubleArray(&backtestData->getOpen()[startIndex], pointCount);
            highData = DoubleArray(&backtestData->getHigh()[startIndex], pointCount);
            lowData = DoubleArray(&backtestData->getLow()[startIndex], pointCount);
            closeData = DoubleArray(&backtestData->getClose()[startIndex], pointCount);
        }
    } else {
        // Utiliser les données agrégées
        const auto& aggregated = dataManager.getAggregatedData(aggregationInfo.level);
        
        timestamps = DoubleArray(&aggregated.timestamps[aggregationInfo.startIndex], aggregationInfo.pointCount);
        openData = DoubleArray(&aggregated.open[aggregationInfo.startIndex], aggregationInfo.pointCount);
        highData = DoubleArray(&aggregated.high[aggregationInfo.startIndex], aggregationInfo.pointCount);
        lowData = DoubleArray(&aggregated.low[aggregationInfo.startIndex], aggregationInfo.pointCount);
        closeData = DoubleArray(&aggregated.close[aggregationInfo.startIndex], aggregationInfo.pointCount);
        volumeData = DoubleArray(&aggregated.volume[aggregationInfo.startIndex], aggregationInfo.pointCount);
    }
    
    // Créer un nouveau graphique
    m_financeChart = std::make_unique<FinanceChart>(config.chartWidth);
    
    // Configurer l'apparence du graphique
    m_financeChart->setPlotAreaStyle(0xE2F4FF, 0xCC999999, 0xCC999999, 0xCC999999, 0xCC999999);
    m_financeChart->setDateLabelFormat(
        "<*font=Arial Bold,size=12,color=800000*>{value|yyyy}<*/font*>",  // Années en gras et rouge
        "<*font=Arial Bold,size=10*>{value|MMM yyyy}<*/font*>",  // Premier mois avec année
        "<*font=Arial*>{value|MMM}<*/font*>",  // Autres mois
        "<*font=Arial Bold*>{value|d MMM}<*/font*>",  // Premier jour avec mois
        "{value|d}",  // Autres jours
        "<*font=Arial Bold*>{value|d MMM hh:nn}<*/font*>",  // Première heure avec jour
        "{value|hh:nn}"  // Autres heures
    );
    m_financeChart->setMargins(0, 20, 50, 100);  // Pas de marges
    m_financeChart->setDateLabelSpacing(80);
    
    // Configurer les données
    m_financeChart->setData(timestamps, highData, lowData, openData, closeData, volumeData, 0);
    
    // Cacher la légende par défaut en la rendant transparente
    m_financeChart->setLegendStyle("normal", 8, Chart::Transparent, Chart::Transparent);
    m_financeChart->setPlotAreaBorder(Chart::Transparent, 0);

    // Ajouter le titre du graphique
    std::string chartTypeStr = dataManager.chartTypeToString(config.chartType);
    std::string aggregationStr = dataManager.aggregationLevelToString(aggregationInfo.level);
    std::string title = "Graphique de trading (" + chartTypeStr + ", " + aggregationStr + ") - " + 
                       std::to_string(timestamps.len) + " points";
    m_financeChart->addTitle(title.c_str());
    
    // Déterminer l'index de début pour les données visibles
    int startIndex = aggregationInfo.startIndex;

    int subChartsTotalHeight = 0;
    
    // 1. Ajouter la courbe d'équité en haut si disponible et demandée
    if (config.showEquity) {
        addEquityCurveSection(m_financeChart.get(), dataManager, timestamps, startIndex);
        subChartsTotalHeight += config.equityHeight;
    }

    for (const RSIInstance* rsi : dataManager.getIndicatorsOfType<RSIInstance>())
        if (rsi->visible)
            subChartsTotalHeight += rsi->height;
    
    // 3. Espace pour Stochastique
    for (const StochasticInstance* stochastic : dataManager.getIndicatorsOfType<StochasticInstance>())
        if (stochastic->visible)
            subChartsTotalHeight += stochastic->height;
    
    // 4. Espace pour ATR
    for (const ATRInstance* atr : dataManager.getIndicatorsOfType<ATRInstance>())
        if (atr->visible)
            subChartsTotalHeight += atr->height;
    
    // 2. Ajouter le graphique principal
    int mainChartHeight = std::max(300, config.chartHeight - subChartsTotalHeight);
    XYChart* mainChart = m_financeChart->addMainChart(mainChartHeight);
    
    // Personnaliser l'affichage des grilles
    mainChart->xAxis()->setWidth(2);  // Axe plus épais
    mainChart->xAxis()->setTickLength(4, 2);  // Ticks plus visibles
    mainChart->xAxis()->setLabelStyle("Arial Bold", 9);  // Étiquettes plus lisibles
    mainChart->yAxis()->setAutoScale(0.01, 0.01, 0);
    // mainChart->xAxis()->setMinTickGap(20); // Espace minimal entre les ticks (en pixels)
    // mainChart->xAxis()->setMaxTickGap(20);

    if (config.fixedYScale) {
        // Calculer les nouvelles limites en appliquant l'offset
        double newMin = config.yScaleMin + config.yScaleOffset;
        double newMax = config.yScaleMax + config.yScaleOffset;
        
        // Appliquer les limites au graphique principal
        mainChart->yAxis()->setRounding(false, false);
        mainChart->yAxis()->setLinearScale(newMin, newMax);
    }
 
    // Ajouter le type de graphique approprié selon le type actuel
    if (config.chartType == ChartDataManager::ChartType::CandleStick || 
        config.chartType == ChartDataManager::ChartType::HeikinAshi) {
        CandleStickLayer* candleStickLayer = m_financeChart->addCandleStick(0x0d9901, 0xF30000); // Vert/Rouge pour les bougies
        candleStickLayer->setColors(0x0d9901, 0x0d9901, 0xF30000, 0xF30000);
        candleStickLayer->setDataWidth(50);
        candleStickLayer->setDataGap(0.1); // Espace entre les bougies
    } else if (config.chartType == ChartDataManager::ChartType::OHLC) {
        m_financeChart->addHLOC(0x00CC00, 0xFF3333); // Vert/Rouge pour les barres OHLC
    } else if (config.chartType == ChartDataManager::ChartType::Close) {
        m_financeChart->addCloseLine(0x000088); // Ligne bleue pour le prix de clôture
    }
    
    // 3. Ajouter tous les indicateurs actifs
    // RSI
    for (const RSIInstance* rsi : dataManager.getIndicatorsOfType<RSIInstance>()) {
        if (rsi->visible) {
            addRSIToChart(m_financeChart.get(), *rsi, dataManager, aggregationInfo);
        }
    }
    
    // EMA
    for (const EMAInstance* ema : dataManager.getIndicatorsOfType<EMAInstance>()) {
        if (ema->visible) {
            addEMAToChart(m_financeChart.get(), *ema, dataManager, aggregationInfo);
        }
    }
    
    // Supertrend
    for (const SuperTrendInstance* supertrend : dataManager.getIndicatorsOfType<SuperTrendInstance>()) {
        if (supertrend->visible) {
            addSupertrendToChart(m_financeChart.get(), *supertrend, dataManager, aggregationInfo);
        }
    }
    
    // Stochastique
    for (const StochasticInstance* stochastic : dataManager.getIndicatorsOfType<StochasticInstance>()) {
        if (stochastic->visible) {
            addStochasticToChart(m_financeChart.get(), *stochastic, dataManager, aggregationInfo);
        }
    }
    
    // ATR
    for (const ATRInstance* atr : dataManager.getIndicatorsOfType<ATRInstance>()) {
        if (atr->visible) {
            addATRToChart(m_financeChart.get(), *atr, dataManager, aggregationInfo);
        }
    }

    // Points pivots
    for (const PivotPointsInstance* pivotPoints : dataManager.getIndicatorsOfType<PivotPointsInstance>()) {
        if (pivotPoints->visible) {
            addPivotPointsToChart(mainChart, *pivotPoints, dataManager, aggregationInfo);
        }
    }

    // super important va me permettre de faire le zoome vertical
    // mainChart->yAxis()->setLinearScale(200, 20000);

    
    // // 4. Ajouter le volume si demandé
    // if (config.showVolume) {
    //     m_financeChart->addVolBars(config.volumeHeight, 0x99ff99, 0xff9999, 0x808080);
    // }
    
    // 5. Ajouter les trades si disponibles et demandés
    if (config.showTrades) {
        addTradeMarkers(m_financeChart.get(), timestamps, dataManager, aggregationInfo);
    }

    // std::cout << "Avant " << mainChart->getYCoor(20000) << std::endl;

    // Mettre à jour le graphique dans le viewer
    if (viewer) 
        viewer->setChart(m_financeChart.get());

    // std::cout << "Après " << mainChart->getYCoor(20000) << std::endl;

    // Mettre à jour les dimensions du renderer
    m_lastYMin = mainChart->yAxis()->getMinValue();
    m_lastYMax = mainChart->yAxis()->getMaxValue();
    m_plotAreaHeight = mainChart->getPlotArea()->getHeight();
}

void ChartRenderer::updateDynamicLayer(QChartViewer *viewer, bool rulerEnabled, 
    bool rulerFirstPointSelected, int rulerStartX, int rulerStartY, 
    const ChartDataManager &dataManager, const ChartDataManager::AggregationInfo &aggregationInfo)
{
    int mouseX = viewer->getPlotAreaMouseX();
    int mouseY = viewer->getPlotAreaMouseY();

    MultiChart* chart = dynamic_cast<MultiChart*>(viewer->getChart());

    // Vérifier que le chart est valide
    if (!viewer || chart->getChartCount() == 0) return;

    // Initialiser le dynamic layer une seule fois
    DrawArea* d = chart->initDynamicLayer();

    trackFinance(chart, mouseX, mouseY, d);

    if (rulerEnabled && rulerFirstPointSelected) {
        int rulerEndX = viewer->getPlotAreaMouseX();
        int rulerEndY = viewer->getPlotAreaMouseY();

        drawRuler(chart, rulerStartX, rulerStartY, rulerEndX, rulerEndY, d, 
                 dataManager, aggregationInfo);
    }
}

void ChartRenderer::addEquityCurveSection(FinanceChart *chart, 
                                         const ChartDataManager& dataManager, 
                                         const DoubleArray &timestamps, 
                                         int startIndex)
{
    const auto& equityData = dataManager.getEquityData();
    if (equityData.equity_values.empty() || timestamps.len == 0)
        return;

    int equityHeight = 120;
    
    // Obtenir la plage de temps visible
    double visibleStartTime = timestamps[0];
    double visibleEndTime = timestamps[timestamps.len - 1];
    
    // Créer les vecteurs pour les données d'équité interpolées
    std::vector<double> interpolatedTimes;
    std::vector<double> interpolatedValues;
    std::vector<double> colorValues;
    
    interpolatedTimes.reserve(timestamps.len);
    interpolatedValues.reserve(timestamps.len);
    colorValues.reserve(timestamps.len - 1);
    
    // Trouver le premier point d'équité qui précède ou correspond à visibleStartTime
    size_t equityIndex = 0;
    while (equityIndex + 1 < equityData.timestamps.size() && 
           equityData.timestamps[equityIndex + 1] < visibleStartTime) {
        equityIndex++;
    }
    
    // Valeur d'équité au début de la fenêtre visible
    double currentEquityValue = equityData.equity_values[equityIndex];

    // Pour chaque timestamp visible, interpoler la valeur d'équité
    for (int i = 0; i < timestamps.len; ++i) {
        double currentTime = timestamps[i];
        
        // Avancer dans les données d'équité si nécessaire
        while (equityIndex + 1 < equityData.timestamps.size() && 
               equityData.timestamps[equityIndex + 1] <= currentTime) {
            equityIndex++;
            currentEquityValue = equityData.equity_values[equityIndex];
        }
        
        // Ajouter le point interpolé
        interpolatedTimes.push_back(i);
        interpolatedValues.push_back(currentEquityValue);
        
        // Déterminer la couleur du segment
        if (i > 0) {
            double prev = interpolatedValues[i-1];
            double curr = currentEquityValue;
            double diff = curr - prev;
            
            if (std::abs(diff) < 1e-10) {
                colorValues.push_back(0);  // Constant
            } else if (diff > 0) {
                colorValues.push_back(1);  // Hausse
            } else {
                colorValues.push_back(2);  // Baisse
            }
        }
    }
    
    // Convertir en DoubleArray
    DoubleArray equityTimes = ChartDataManager::vectorToDoubleArray(interpolatedTimes);
    DoubleArray equityValues = ChartDataManager::vectorToDoubleArray(interpolatedValues);
    
    // Ajouter l'indicateur pour l'equity curve
    XYChart* equityChart = chart->addIndicator(equityHeight);
    
    // Configuration du titre et des libellés
    equityChart->xAxis()->setColors(Chart::Transparent);
    
    // Définir les couleurs pour les segments
    int constColor = 0x999999;
    int upColor = 0x53DD00;
    int downColor = 0xFF0000;
    
    // Créer trois couches de stepline séparées
    StepLineLayer* equityLineLayer = equityChart->addStepLineLayer(equityValues, Chart::Transparent, "Equity");
    StepLineLayer* constantLayer = equityChart->addStepLineLayer();
    StepLineLayer* upLayer = equityChart->addStepLineLayer();
    StepLineLayer* downLayer = equityChart->addStepLineLayer(); 
    
    constantLayer->setFastLineMode(true);
    upLayer->setFastLineMode(true);
    downLayer->setFastLineMode(true);

    // Créer une ligne horizontale pour le cash initial
    double initialCash = equityData.equity_values.front();

    Mark* mark = equityChart->yAxis()->addMark(initialCash, 0x000000, "Initial Cash");
    mark->setLineWidth(2);
    mark->setMarkColor(equityChart->dashLineColor(0x000000), 0xffffff);
    mark->setAlignment(Chart::Left);
    mark->setBackground(0x000000, 0xffffff, 1);
    equityChart->addInterLineLayer(equityLineLayer->getLine(), mark->getLine(), 0xCC53DD00, 0xCCff0000);

    // Configurer l'alignement des steplines
    constantLayer->setAlignment(Chart::Left);
    upLayer->setAlignment(Chart::Left);
    downLayer->setAlignment(Chart::Left);
    
    // Création des ensembles de données pour chaque type de segment
    std::vector<std::vector<double>> segmentX(3);
    std::vector<std::vector<double>> segmentY(3);
    
    // Parcourir les points et créer des segments colorés
    for (size_t i = 1; i < interpolatedValues.size(); ++i) {
        int colorIndex = static_cast<int>(colorValues[i-1]);
        
        segmentX[colorIndex].push_back(interpolatedTimes[i-1]);
        segmentY[colorIndex].push_back(interpolatedValues[i-1]);
        
        segmentX[colorIndex].push_back(interpolatedTimes[i]);
        segmentY[colorIndex].push_back(interpolatedValues[i]);
        
        segmentX[colorIndex].push_back(Chart::NoValue);
        segmentY[colorIndex].push_back(Chart::NoValue);
    }
    
    // Ajouter les segments à leurs couches respectives
    if (!segmentX[0].empty()) {
        DoubleArray x = ChartDataManager::vectorToDoubleArray(segmentX[0]);
        DoubleArray y = ChartDataManager::vectorToDoubleArray(segmentY[0]);
        constantLayer->setXData(x);
        DataSet* constDataSet = constantLayer->addDataSet(y, constColor, "Constant");
        constantLayer->setLineWidth(2);
    }
    
    if (!segmentX[1].empty()) {
        DoubleArray x = ChartDataManager::vectorToDoubleArray(segmentX[1]);
        DoubleArray y = ChartDataManager::vectorToDoubleArray(segmentY[1]);
        upLayer->setXData(x);
        DataSet* upDataSet = upLayer->addDataSet(y, upColor, "Up");
        upLayer->setLineWidth(5);
    }
    
    if (!segmentX[2].empty()) {
        DoubleArray x = ChartDataManager::vectorToDoubleArray(segmentX[2]);
        DoubleArray y = ChartDataManager::vectorToDoubleArray(segmentY[2]);
        downLayer->setXData(x);
        DataSet* downDataSet = downLayer->addDataSet(y, downColor, "Down");
        downLayer->setLineWidth(5);
    }
    
    // Ajouter un point à la fin de la courbe pour marquer la valeur actuelle
    if (!interpolatedValues.empty()) {
        std::vector<double> lastPointX = {(double)(interpolatedTimes.size() - 1)};
        std::vector<double> lastPointY = {interpolatedValues.back()};
        
        DoubleArray xPoint = ChartDataManager::vectorToDoubleArray(lastPointX);
        DoubleArray yPoint = ChartDataManager::vectorToDoubleArray(lastPointY);
        
        ScatterLayer* endPoint = equityChart->addScatterLayer(xPoint, yPoint, 
                                                            "Current", Chart::CircleShape, 7, 
                                                            0x000000, 0x000000);
        endPoint->moveFront();
    }
}

void ChartRenderer::addTradeMarkers(FinanceChart *chart, 
                                   const DoubleArray &timestamps,
                                   const ChartDataManager& dataManager,
                                   const ChartDataManager::AggregationInfo& aggregationInfo)
{
    const std::vector<std::shared_ptr<be::Trade>>& trades = dataManager.getTrades();
    int startIndex = aggregationInfo.startIndex;

    if (trades.empty() || !dataManager.hasValidData())
        return;

    if (aggregationInfo.level != AggregationLevel::Raw) 
        return; // Les trades ne sont affichés qu'en mode Raw pour l'instant

    // Obtenir le graphique principal
    XYChart* mainChart = (XYChart*)chart->getChart(1);
    if (!mainChart)
        return;

    // Constantes pour les couleurs selon résultat
    const int COLOR_TP = 0x00AA00;    // Vert
    const int COLOR_SL = 0xCC0000;   // Rouge
    const int COLOR_BE = 0x0000CC; // Bleu
    const int COLOR_NEUTRAL = 0x000000; // Noir
    const int COLOR_UNKNOWN = 0x808080; // Gris

    // Marqueurs carrés pour position exacte
    std::vector<std::pair<double, double>> entryMarkers;
    std::vector<std::pair<double, double>> exitMarkers;
    
    // Flèches d'entrée (selon direction du trade)
    std::vector<std::pair<double, double>> entryLongArrows;  // Achats (flèche verte vers le haut)
    std::vector<std::pair<double, double>> entryShortArrows; // Ventes (flèche rouge vers le bas)
    
    // Flèches de sortie (selon résultat et direction)
    std::vector<std::pair<double, double>> exitLongTPArrows;    // Sortie achat gagnant
    std::vector<std::pair<double, double>> exitLongSLArrows;   // Sortie achat perdante
    std::vector<std::pair<double, double>> exitLongBEArrows;   // Sortie achat break-even
    std::vector<std::pair<double, double>> exitLongNeutralArrows; // Sortie achat neutre
    std::vector<std::pair<double, double>> exitShortTPArrows;   // Sortie vente gagnante
    std::vector<std::pair<double, double>> exitShortSLArrows;  // Sortie vente perdante
    std::vector<std::pair<double, double>> exitShortBEArrows;  // Sortie vente break-even
    std::vector<std::pair<double, double>> exitShortNeutralArrows; // Sortie vente neutre
    
    // Préallocation
    size_t estimatedMarkers = std::min(size_t(100), trades.size() * 2);
    entryMarkers.reserve(estimatedMarkers);
    exitMarkers.reserve(estimatedMarkers);
    entryLongArrows.reserve(estimatedMarkers);
    entryShortArrows.reserve(estimatedMarkers);

    exitLongTPArrows.reserve(estimatedMarkers/3);
    exitLongSLArrows.reserve(estimatedMarkers/3);
    exitLongBEArrows.reserve(estimatedMarkers/3);
    exitLongNeutralArrows.reserve(estimatedMarkers/3);

    exitShortTPArrows.reserve(estimatedMarkers/3);
    exitShortSLArrows.reserve(estimatedMarkers/3);
    exitShortBEArrows.reserve(estimatedMarkers/3);
    exitShortNeutralArrows.reserve(estimatedMarkers/3);

    std::vector<TPSLBESegment> tpslbeSegments;
    tpslbeSegments.reserve(estimatedMarkers * 2);

    for (const auto& trade : trades) {
        bool isLong = trade->isLong();
        
        // Déterminer le résultat du trade
        be::CloseReason closeReason = trade->closeReason();

        // Indices pour l'entrée et la sortie
        size_t entryBarIndex = trade->entryBar();
        size_t exitBarIndex = trade->isClosed() ? trade->exitBar() : 0;
        
        // Vérifier si le trade est visible dans la fenêtre (au moins entrée ou sortie visible)
        bool entryVisible = (entryBarIndex >= static_cast<size_t>(startIndex) && 
                            entryBarIndex < static_cast<size_t>(startIndex + timestamps.len));
        bool exitVisible = trade->isClosed() && (exitBarIndex >= static_cast<size_t>(startIndex) && 
                           exitBarIndex < static_cast<size_t>(startIndex + timestamps.len));
                           
        // Si ni l'entrée ni la sortie n'est visible, ignorer ce trade
        if (!entryVisible && !exitVisible) continue;


        if (entryVisible) {
            double relativeIndex = static_cast<double>(entryBarIndex - startIndex);
            
            // Marqueur carré pour la position d'entrée
            entryMarkers.push_back({relativeIndex, trade->entryPrice()});
            
            if (entryBarIndex < static_cast<int>(dataManager.getBacktestData()->size())) {
                const be::Candle& entryCandle = dataManager.getBacktestData()->at(entryBarIndex);
                
                if (isLong) {
                    // Achat: flèche vers le haut SOUS la bougie
                    double arrowY = entryCandle.low;
                    entryLongArrows.push_back({relativeIndex, arrowY});
                } else {
                    // Vente: flèche vers le bas AU-DESSUS de la bougie
                    double arrowY = entryCandle.high;
                    entryShortArrows.push_back({relativeIndex, arrowY});
                }
            }
        }

        // Ajouter les segments TP/SL (inchangé)
        if (trade->isClosed()) {
            // Calculer les indices relatifs comme pour les points pivots
            // en les contraignant aux limites de la fenêtre visible
            double relativeEntryIndex = static_cast<double>(
                std::max(int(entryBarIndex) - startIndex, 0));
            double relativeExitIndex = static_cast<double>(
                std::min(int(exitBarIndex) - startIndex, aggregationInfo.pointCount - 1));
        
            int color;
            if (closeReason == be::CloseReason::TakeProfit) color = COLOR_TP;
            else if (closeReason == be::CloseReason::StopLoss) color = COLOR_SL;
            else if (closeReason == be::CloseReason::BreakEven) color = COLOR_BE;
            else if (closeReason == be::CloseReason::ManualClose) color = COLOR_NEUTRAL;
            else color = COLOR_UNKNOWN;

            double tpValue = trade->tp();
            if (tpValue > 0) {
                tpslbeSegments.push_back({
                    relativeEntryIndex, relativeExitIndex, 
                    tpValue, TPSLBEType::TakeProfit,
                    color
                });
            }
            
            double slValue = (trade->initialSlPrice() > 0) ? trade->initialSlPrice() : trade->sl();
            if (slValue > 0) {
                tpslbeSegments.push_back({
                    relativeEntryIndex, relativeExitIndex,
                    slValue, TPSLBEType::StopLoss,
                    color
                });
            }

            double beValue = trade->breakEvenTriggerPrice();
            if (beValue > 0) {
                tpslbeSegments.push_back({
                    relativeEntryIndex, relativeExitIndex,
                    beValue, TPSLBEType::BreakEven,
                    color
                });
            }

            if (exitVisible) {
                double relativeExitIndex = static_cast<double>(exitBarIndex - startIndex);
                
                // Marqueur carré pour la position de sortie
                exitMarkers.push_back({relativeExitIndex, trade->exitPrice()});
                
                if (exitBarIndex < static_cast<int>(dataManager.getBacktestData()->size())) {
                    const be::Candle& exitCandle = dataManager.getBacktestData()->at(exitBarIndex);
                    
                    if (isLong) {
                        // Sortie achat: flèche vers le bas AU-DESSUS de la bougie
                        double arrowY = exitCandle.high;
                        
                        if (closeReason == be::CloseReason::TakeProfit)
                            exitLongTPArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::StopLoss)
                            exitLongSLArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::BreakEven)
                            exitLongBEArrows.push_back({relativeExitIndex, arrowY});
                        else
                            exitLongNeutralArrows.push_back({relativeExitIndex, arrowY});
                    } else {
                        // Sortie vente: flèche vers le haut SOUS la bougie
                        double arrowY = exitCandle.low;

                        if (closeReason == be::CloseReason::TakeProfit)
                            exitShortTPArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::StopLoss)
                            exitShortSLArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::BreakEven)
                            exitShortBEArrows.push_back({relativeExitIndex, arrowY});
                        else
                            exitShortNeutralArrows.push_back({relativeExitIndex, arrowY});
                    }
                }
            }
        }
    }

    // Ajouter les marqueurs carrés pour les positions exactes
    addMarkers(mainChart, entryMarkers, "Entry", Chart::SquareSymbol, 7, 0x000000);
    addMarkers(mainChart, exitMarkers, "Exit", Chart::SquareSymbol, 7, 0x000000);
    addTPSLSegments(mainChart, tpslbeSegments);
    
    // Taille des symboles
    int symbolSize = (aggregationInfo.level == AggregationLevel::Raw) ? 11 : 9;
    
    // Flèches d'entrée
    if (!entryLongArrows.empty())
        addMarkers(mainChart, entryLongArrows, "Long Entry", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_TP, 0, 20); // Flèche verte vers le haut sous la bougie
    if (!entryShortArrows.empty())
        addMarkers(mainChart, entryShortArrows, "Short Entry", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_SL, 0, -20); // Flèche rouge vers le bas au-dessus
    
    // Flèches de sortie Long (achat)
    if (!exitLongTPArrows.empty())
        addMarkers(mainChart, exitLongTPArrows, "Long Exit Win", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_TP, 0, -20); // Flèche verte vers le bas au-dessus
    if (!exitLongSLArrows.empty())
        addMarkers(mainChart, exitLongSLArrows, "Long Exit Loss", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_SL, 0, -20); // Flèche rouge vers le bas au-dessus
    if (!exitLongBEArrows.empty())
        addMarkers(mainChart, exitLongBEArrows, "Long Exit Break Even", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_BE, 0, -20); // Flèche bleue vers le bas au-dessus
    if (!exitLongNeutralArrows.empty())
        addMarkers(mainChart, exitLongNeutralArrows, "Long Exit Neutral", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_NEUTRAL, 0, -20); // Flèche noire vers le bas au-dessus
    
    // Flèches de sortie Short (vente)
    if (!exitShortTPArrows.empty())
        addMarkers(mainChart, exitShortTPArrows, "Short Exit Win", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_TP, 0, 20); // Flèche verte vers le haut sous la bougie
    if (!exitShortSLArrows.empty())
        addMarkers(mainChart, exitShortSLArrows, "Short Exit Loss", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_SL, 0, 20); // Flèche rouge vers le haut sous la bougie
    if (!exitShortBEArrows.empty())
        addMarkers(mainChart, exitShortBEArrows, "Short Exit Break Even", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_BE, 0, 20); // Flèche bleue vers le haut sous la bougie
    if (!exitShortNeutralArrows.empty())
        addMarkers(mainChart, exitShortNeutralArrows, "Short Exit Neutral", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_NEUTRAL, 0, 20); // Flèche noire vers le haut sous la bougie
}

void ChartRenderer::addRSIToChart(FinanceChart* chart, 
                                const RSIInstance& rsi, 
                                const ChartDataManager& dataManager, 
                                const ChartDataManager::AggregationInfo& aggregationInfo)
{
    int startIndex = aggregationInfo.startIndex;
    int pointsToShow = aggregationInfo.pointCount;
    
    const std::vector<double>* rsiData = nullptr;


    if (aggregationInfo.level == AggregationLevel::Raw) {
        // Utiliser les données brutes
        const auto& rsiMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).rsiValues;
        auto it = rsiMap.find(rsi.id);
        if (it == rsiMap.end()) return;
        
        rsiData = &(it->second);
    } 
    else {
        // Utiliser les données agrégées
        const auto& aggregated = dataManager.getAggregatedIndicators(aggregationInfo.level);
        auto it = aggregated.rsiValues.find(rsi.id);
        
        // Vérifier si les données agrégées sont disponibles et valides
        if (it != aggregated.rsiValues.end() && aggregated.isRsiValid(rsi.id)) {
            rsiData = &(it->second);
        }
        else {
            // Fallback aux données brutes si les données agrégées ne sont pas disponibles
            const auto& rsiMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).rsiValues;
            auto rawIt = rsiMap.find(rsi.id);
            if (rawIt == rsiMap.end()) return;
            
            rsiData = &(rawIt->second);
        }
    }

    if (!rsiData || startIndex >= (int)rsiData->size()) return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)rsiData->size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) return;

    // Extraire les données RSI visibles du cache
    DoubleArray rsiArray(&(*rsiData)[startIndex], actualPoints);

    // Ajouter le graphique d'indicateur
    XYChart* c = chart->addIndicator(rsi.height);
    
    // Configurer et ajouter le RSI
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "RSI (%d)", rsi.period);
    LineLayer* layer = chart->addLineIndicator2(c, rsiArray, rsi.color, buffer);
    layer->setFastLineMode(true);

    // Ajouter les seuils
    chart->addThreshold(c, layer, rsi.overboughtLevel, rsi.upperColor, rsi.oversoldLevel, rsi.lowerColor);

    // Configurer l'échelle de l'axe Y
    c->yAxis()->setLinearScale(0, 100);
}

void ChartRenderer::addEMAToChart(FinanceChart* chart, 
                                const EMAInstance& ema, 
                                const ChartDataManager& dataManager, 
                                const ChartDataManager::AggregationInfo& aggregationInfo)
{
    // Déterminer quelle source de données utiliser
    const std::vector<double>* emaData = nullptr;
    int startIndex = aggregationInfo.startIndex;
    int pointsToShow = aggregationInfo.pointCount;

    if (aggregationInfo.level == AggregationLevel::Raw) {
        // Utiliser les données brutes
        const auto& emaMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).emaValues;
        auto it = emaMap.find(ema.id);
        if (it == emaMap.end()) return;
        
        emaData = &(it->second);
    } 
    else {
        // Utiliser les données agrégées
        const auto& aggregated = dataManager.getAggregatedIndicators(aggregationInfo.level);
        auto it = aggregated.emaValues.find(ema.id);
        
        // Vérifier si les données agrégées sont disponibles et valides
        if (it != aggregated.emaValues.end() && aggregated.isEmaValid(ema.id)) {
            emaData = &(it->second);
        }
        else {
            // Fallback aux données brutes
            const auto& emaMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).emaValues;
            auto rawIt = emaMap.find(ema.id);
            if (rawIt == emaMap.end()) return;
            
            emaData = &(rawIt->second);
        }
    }

    if (startIndex >= (int)emaData->size()) return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)emaData->size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) return;

    // Extraire les données EMA visibles du cache
    DoubleArray emaArray(&(*emaData)[startIndex], actualPoints);

    // Configurer et ajouter l'EMA directement sur le graphique principal
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "EMA (%d)", ema.period);
    LineLayer* layer = chart->addLineIndicator2((XYChart*)chart->getChart(1), emaArray, ema.color, buffer);
    layer->setFastLineMode(true);
}

void ChartRenderer::addSupertrendToChart(FinanceChart* chart, 
                                      const SuperTrendInstance& supertrend, 
                                      const ChartDataManager& dataManager, 
                                      const ChartDataManager::AggregationInfo& aggregationInfo)
{
    const std::vector<double>* supertrendValues = nullptr;
    const std::vector<int>* trendDirections = nullptr;
    int startIndex = aggregationInfo.startIndex;
    int pointsToShow = aggregationInfo.pointCount;

    if (aggregationInfo.level == AggregationLevel::Raw) {
        auto it = dataManager.getAggregatedIndicators(AggregationLevel::Raw).supertrendValues.find(supertrend.id);
        if (it != dataManager.getAggregatedIndicators(AggregationLevel::Raw).supertrendValues.end()) {
            supertrendValues = &it->second.first;
            trendDirections = &it->second.second;
        }
    } else {
        const auto& aggregatedIndicators = dataManager.getAggregatedIndicators(aggregationInfo.level);
        if (aggregatedIndicators.isSupertrendValid(supertrend.id)) {
            auto it = aggregatedIndicators.supertrendValues.find(supertrend.id);
            if (it != aggregatedIndicators.supertrendValues.end()) {
                supertrendValues = &it->second.first;
                trendDirections = &it->second.second;
            }
        }
    }

    if (!supertrendValues || !trendDirections || startIndex >= (int)supertrendValues->size())
        return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)supertrendValues->size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0)
        return;

    // Extraire les données visibles du cache
    std::vector<double> upValues(actualPoints, Chart::NoValue);
    std::vector<double> downValues(actualPoints, Chart::NoValue);

    for (int i = 0; i < actualPoints; ++i) {
        int index = startIndex + i;
        if ((*trendDirections)[index] == 1) {
            upValues[i] = (*supertrendValues)[index];
        } else if ((*trendDirections)[index] == -1) {
            downValues[i] = (*supertrendValues)[index];
        }
    }

    // Convertir en DoubleArray
    DoubleArray upArray = ChartDataManager::vectorToDoubleArray(upValues);
    DoubleArray downArray = ChartDataManager::vectorToDoubleArray(downValues);

    // Ajouter directement sur le graphique principal
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Supertrend (%d, %.1f)", supertrend.period, supertrend.multiplier);
    
    XYChart* mainChart = (XYChart*)chart->getChart(1);
    
    // Lignes haussières (en vert)
    LineLayer* upLayer = chart->addLineIndicator2(mainChart, upArray, supertrend.upColor, 
                                               buffer);
    upLayer->setLineWidth(1);
    
    // Lignes baissières (en rouge)
    LineLayer* downLayer = chart->addLineIndicator2(mainChart, downArray, supertrend.downColor, 
                                                 "");
    downLayer->setLineWidth(1);
}


void ChartRenderer::addStochasticToChart(FinanceChart* chart, 
                                       const StochasticInstance& stochastic, 
                                       const ChartDataManager& dataManager, 
                                       const ChartDataManager::AggregationInfo& aggregationInfo)
{
    const std::vector<double>* kValues = nullptr;
    const std::vector<double>* dValues = nullptr;

    int startIndex = aggregationInfo.startIndex;
    int pointsToShow = aggregationInfo.pointCount;

    if (aggregationInfo.level == AggregationLevel::Raw) {
        // Utiliser les données brutes
        const auto& stochasticMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).stochasticValues;
        auto it = stochasticMap.find(stochastic.id);
        if (it == stochasticMap.end()) return;
        
        kValues = &(it->second.first);
        dValues = &(it->second.second);
    }
    else {
        // Utiliser les données agrégées
        const auto& aggregated = dataManager.getAggregatedIndicators(aggregationInfo.level);
        auto it = aggregated.stochasticValues.find(stochastic.id);
        
        // Vérifier si les données agrégées sont disponibles et valides
        if (it != aggregated.stochasticValues.end() && aggregated.isStochasticValid(stochastic.id)) {
            kValues = &(it->second.first);
            dValues = &(it->second.second);
        }
        else {
            // Fallback aux données brutes
            const auto& stochasticMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).stochasticValues;
            auto rawIt = stochasticMap.find(stochastic.id);
            if (rawIt == stochasticMap.end()) return;
            
            kValues = &(rawIt->second.first);
            dValues = &(rawIt->second.second);
        }
    }

    if (startIndex >= (int)kValues->size() || startIndex >= (int)dValues->size()) return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)kValues->size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) return;

    // Extraire les données Stochastic visibles du cache
    DoubleArray kArray(&(*kValues)[startIndex], actualPoints);
    DoubleArray dArray(&(*dValues)[startIndex], actualPoints);
    
    // Ajouter le graphique d'indicateur
    XYChart* c = chart->addIndicator(stochastic.height);
    
    // Configurer et ajouter les lignes %K et %D
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Stochastic %%K (%d, %d, %d)", 
             stochastic.fastKPeriod, stochastic.slowKPeriod, stochastic.slowDPeriod);

    LineLayer* kLayer = chart->addLineIndicator2(c, kArray, stochastic.kColor, buffer);
    kLayer->setFastLineMode(true);
    
    snprintf(buffer, sizeof(buffer), "%%D (%d)", stochastic.slowDPeriod);
    LineLayer* dLayer = chart->addLineIndicator2(c, dArray, stochastic.dColor, buffer);
    dLayer->setFastLineMode(true);
    
    // Configurer l'échelle de l'axe Y
    c->yAxis()->setLinearScale(0, 100);
    
    // Ajouter les seuils pour les niveaux de surachat et de survente
    Mark* overboughtMark = c->yAxis()->addMark(stochastic.overboughtLevel, 0xff6666, 
                                              std::to_string(stochastic.overboughtLevel).c_str());
    Mark* oversoldMark = c->yAxis()->addMark(stochastic.oversoldLevel, 0x6666ff, 
                                           std::to_string(stochastic.oversoldLevel).c_str());

    c->addInterLineLayer(kLayer->getLine(), overboughtMark->getLine(), 0xCCff0000, Chart::Transparent);
    c->addInterLineLayer(kLayer->getLine(), oversoldMark->getLine(), Chart::Transparent, 0xCC0000ff);
}

void ChartRenderer::addATRToChart(FinanceChart* chart, 
                                const ATRInstance& atr, 
                                const ChartDataManager& dataManager, 
                                const ChartDataManager::AggregationInfo& aggregationInfo)
{
    const std::vector<double>* atrData = nullptr;
    int startIndex = aggregationInfo.startIndex;
    int pointsToShow = aggregationInfo.pointCount;

    if (aggregationInfo.level == AggregationLevel::Raw) {
        // Utiliser les données brutes
        const auto& atrMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).atrValues;
        auto it = atrMap.find(atr.id);
        if (it == atrMap.end()) return;
        
        atrData = &(it->second);
        startIndex = aggregationInfo.startIndex;
        pointsToShow = aggregationInfo.pointCount;
    } 
    else {
        // Utiliser les données agrégées
        const auto& aggregated = dataManager.getAggregatedIndicators(aggregationInfo.level);
        auto it = aggregated.atrValues.find(atr.id);
        
        // Vérifier si les données agrégées sont disponibles et valides
        if (it != aggregated.atrValues.end() && aggregated.isAtrValid(atr.id)) {
            atrData = &(it->second);
            startIndex = aggregationInfo.startIndex;
            pointsToShow = aggregationInfo.pointCount;
        }
        else {
            // Fallback aux données brutes
            const auto& atrMap = dataManager.getAggregatedIndicators(AggregationLevel::Raw).atrValues;
            auto rawIt = atrMap.find(atr.id);
            if (rawIt == atrMap.end()) return;
            
            atrData = &(rawIt->second);
            startIndex = aggregationInfo.startIndex;
            pointsToShow = aggregationInfo.pointCount;
        }
    }

    if (startIndex >= (int)atrData->size()) return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)atrData->size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) return;

    // Extraire les données ATR visibles du cache
    DoubleArray atrArray(&(*atrData)[startIndex], actualPoints);

    // Ajouter le graphique d'indicateur
    XYChart* c = chart->addIndicator(atr.height);
    
    // Configurer et ajouter l'ATR
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "ATR (%d)", atr.period);
    LineLayer* layer = chart->addLineIndicator2(c, atrArray, atr.color, buffer);
    layer->setFastLineMode(true);

    // Configurer l'échelle de l'axe Y
    double maxATR = *std::max_element(atrData->begin() + startIndex, atrData->begin() + endIndex);
    c->yAxis()->setLinearScale(0, maxATR * 1.1); // 10% de marge supérieure
}

void ChartRenderer::addPivotPointsToChart(XYChart *mainChart, 
                                        const PivotPointsInstance &pivotPoints, 
                                        const ChartDataManager &dataManager, 
                                        const ChartDataManager::AggregationInfo &aggregationInfo)
{

    // Récupérer les données des points pivots depuis le cache
    int startIndex = aggregationInfo.startIndex;
    int endIndex = startIndex + aggregationInfo.pointCount - 1;
    AggregationLevel currentLevel = aggregationInfo.level;

    const std::map<int, std::vector<PivotPeriod>>& pivotPeriodsMap = dataManager.getPivotPeriods();
    auto it = pivotPeriodsMap.find(pivotPoints.id);
    if (it == pivotPeriodsMap.end()) {
        return; // Pas de données pour cet ID de points pivots
    }
    
    // Récupérer le vecteur de périodes pivot
    const std::vector<PivotPeriod>& pivotPeriods = it->second;

    // Parcourir toutes les périodes pivot
    for (const auto& period : pivotPeriods) {
        // Récupérer les indices agrégés pour le niveau d'agrégation actuel
        auto indicesIt = period.indices.find(currentLevel);
        
        // Si pas d'indices agrégés pour ce niveau, essayer de convertir les indices bruts
        std::pair<int, int> indices;
        if (indicesIt == period.indices.end())
            continue; // Pas d'indices pour ce niveau
            
        indices = indicesIt->second;
        
        int aggStartIndex = indices.first;
        int aggEndIndex = indices.second;

        // Vérifier si la période est visible dans la plage courante
        if (aggEndIndex < startIndex || aggStartIndex > endIndex) {
            continue;  // Période hors plage visible
        }
        
        // Calculer les indices relatifs pour l'affichage
        int relativeStart = std::max(aggStartIndex - startIndex, 0);
        int relativeEnd = std::min(aggEndIndex - startIndex, aggregationInfo.pointCount - 1);
        
        // Pour chaque niveau de pivot configuré
        for (const auto& [levelType, style] : pivotPoints.levelStyles) {
            // Vérifier si ce niveau doit être affiché
            if (!pivotPoints.isLevelVisible(levelType)) continue;
            
            // Récupérer la valeur du niveau pour cette période
            auto levelIt = period.levelValues.find(static_cast<int>(levelType));
            if (levelIt == period.levelValues.end()) continue;
            
            double value = levelIt->second;

            // Ignorer les segments avec des valeurs non valides ou nulles
            if (value == 0 || std::isnan(value)) continue;
            
            // Créer un vecteur de points pour tracer la ligne horizontale
            std::vector<double> xData = {static_cast<double>(relativeStart), static_cast<double>(relativeEnd)};
            std::vector<double> yData = {value, value};
            
            // Convertir en DoubleArray pour ChartDir
            DoubleArray xArray = ChartDataManager::vectorToDoubleArray(xData);
            DoubleArray yArray = ChartDataManager::vectorToDoubleArray(yData);
            
            // Créer une couche de ligne
            LineLayer* layer = mainChart->addLineLayer(yArray, style.color);
            layer->setXData(xArray);
            layer->setLineWidth(style.thickness);

            // Définir le style de ligne en fonction du style de LineStyle
            int dashPatternColor;
            switch (style.lineStyle) {
                case PivotPointsInstance::LineStyle::Dash:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::DashLine);
                    break;
                case PivotPointsInstance::LineStyle::Dot:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::DotLine);
                    break;
                case PivotPointsInstance::LineStyle::DotDash:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::DotDashLine);
                    break;
                case PivotPointsInstance::LineStyle::AltDash:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::AltDashLine);
                    break;
                default: // LineStyle::Solid
                    dashPatternColor = style.color;
                    break;
            }

            // Appliquer la couleur au DataSet
            DataSet* dataSet = layer->getDataSet(0);
            if (dataSet)
                dataSet->setDataColor(dashPatternColor);
            
            // NOUVEAU CODE: Ajouter une étiquette personnalisée au point de fin du segment si demandé
            if (pivotPoints.showLabels) {

                QString periodSuffix;
                switch (pivotPoints.periodType) {
                    case PivotPointsInstance::PeriodType::FourHour: periodSuffix = "4H"; break;
                    case PivotPointsInstance::PeriodType::Daily: periodSuffix = "J"; break;
                    case PivotPointsInstance::PeriodType::Weekly: periodSuffix = "S"; break;
                    case PivotPointsInstance::PeriodType::Monthly: periodSuffix = "M"; break;
                }

                // Formater l'étiquette selon le format spécifié ou le format par défaut
                QString labelText;
                if (!style.labelFormat.isEmpty())
                    labelText = style.labelFormat.arg(periodSuffix);
                
                // Ajouter un label personnalisé au point final du segment (index 1)
                TextBox* label = layer->addCustomDataLabel(0, 1, 
                                                         labelText.toStdString().c_str(), 
                                                         "Arial Bold", 8, style.color);
                
                // Configurer l'apparence du label
                label->setAlignment(Chart::Right);
                label->setPos(label->getLeftX() - 5, label->getTopY() - 8); // Décalage à droite et vers le haut
                label->setBackground(Chart::Transparent);
                label->setMargin(3);               // Marge interne
            }
        }
    }
}

void ChartRenderer::addMarkers(XYChart *chart, const std::vector<std::pair<double, double>> &markers,
                               const char *name, int symbolType, int symbolSize, int color, int offsetX, int offsetY)
{
    if (markers.empty()) return;

    std::vector<double> xValues;
    std::vector<double> yValues;
    xValues.reserve(markers.size());
    yValues.reserve(markers.size());
    
    for (const auto& pair : markers) {
        xValues.push_back(pair.first);
        yValues.push_back(pair.second);
    }

    // Convertir en DoubleArray
    DoubleArray xArray = ChartDataManager::vectorToDoubleArray(xValues);
    DoubleArray yArray = ChartDataManager::vectorToDoubleArray(yValues);

    ScatterLayer* layer = chart->addScatterLayer(xArray, yArray, name, 
                                              symbolType, symbolSize, color);

    if (offsetX != 0 || offsetY != 0)
        layer->getDataSet(0)->setSymbolOffset(offsetX, offsetY);

    layer->moveFront();
}

void ChartRenderer::addTPSLSegments(XYChart* chart, const std::vector<TPSLBESegment>& segments)
{
    if (segments.empty()) return;
    
    // Créer des vecteurs séparés pour les segments TP et SL
    std::vector<double> tpXData, tpYData;
    std::vector<double> slXData, slYData;
    std::vector<double> beXData, beYData;
    
    // Parcourir tous les segments et les séparer par type
    for (const auto& segment : segments) {
        std::vector<double>& xData = segment.type == TPSLBEType::TakeProfit ? tpXData : (segment.type == TPSLBEType::StopLoss ? slXData : beXData);
        std::vector<double>& yData = segment.type == TPSLBEType::TakeProfit ? tpYData : (segment.type == TPSLBEType::StopLoss ? slYData : beYData);
        
        // Ajouter le point de départ du segment horizontal
        xData.push_back(segment.startX);
        yData.push_back(segment.level);
        
        // Ajouter le point de fin du segment horizontal
        xData.push_back(segment.endX);
        yData.push_back(segment.level);
        
        // Ajouter un point NoValue pour créer une discontinuité
        xData.push_back(Chart::NoValue);
        yData.push_back(Chart::NoValue);
    }
    
    // Ajouter les segments de Take Profit
    if (!tpXData.empty()) {
        LineLayer* tpLayer = chart->addLineLayer();
        tpLayer->setLineWidth(1);
        tpLayer->setFastLineMode(true);
        
        DoubleArray tpX = ChartDataManager::vectorToDoubleArray(tpXData);
        DoubleArray tpY = ChartDataManager::vectorToDoubleArray(tpYData);
        
        tpLayer->setXData(tpX);
        tpLayer->addDataSet(tpY, 0x00AA00, "Take Profit");
        tpLayer->moveFront();
    }
    
    // Ajouter les segments de Stop Loss
    if (!slXData.empty()) {
        LineLayer* slLayer = chart->addLineLayer();
        slLayer->setLineWidth(1);
        slLayer->setFastLineMode(true);
        
        DoubleArray slX = ChartDataManager::vectorToDoubleArray(slXData);
        DoubleArray slY = ChartDataManager::vectorToDoubleArray(slYData);

        slLayer->setXData(slX);
        slLayer->addDataSet(slY, 0xCC0000, "Stop Loss");
        slLayer->moveFront();
    }

    // Ajouter les segments de Break Even
    if (!beXData.empty()) {
        LineLayer* beLayer = chart->addLineLayer();
        beLayer->setLineWidth(1);
        beLayer->setFastLineMode(true);

        DoubleArray beX = ChartDataManager::vectorToDoubleArray(beXData);
        DoubleArray beY = ChartDataManager::vectorToDoubleArray(beYData);

        beLayer->setXData(beX);
        beLayer->addDataSet(beY, 0x0000CC, "Break Even");
        beLayer->moveFront();
    }
}

void ChartRenderer::drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d, 
                             const ChartDataManager& dataManager, 
                             const ChartDataManager::AggregationInfo& aggregationInfo)
{
    // Vérifier que le chart est valide et qu'il y a au moins un graphique
    if (!m || m->getChartCount() == 0) return;
    
    // Obtenir le premier graphique XY (graphique principal)
    XYChart* c = (XYChart*)m->getChart(1);
    if (!c) return;

    // Obtenir les indices correspondant aux positions du curseur
    double xIndexStart = c->getNearestXValue(startX);
    double xIndexEnd = c->getNearestXValue(endX);
    double yValueStart = c->getYValue(startY);
    double yValueEnd = c->getYValue(endY);

    // Récupérer les timestamps réels depuis le ChartDataManager
    double xValueStart, xValueEnd;
    
    // Récupérer les timestamps selon le niveau d'agrégation actuel
    if (aggregationInfo.level == AggregationLevel::Raw) {
        // En mode Raw, utiliser directement l'indice pour accéder aux timestamps
        const auto& timestamps = dataManager.getTimestamps();
        int startIndex = aggregationInfo.startIndex;
        
        // Convertir les indices relatifs en indices absolus
        int absIndexStart = startIndex + static_cast<int>(xIndexStart);
        int absIndexEnd = startIndex + static_cast<int>(xIndexEnd);
        
        // Vérifier que les indices sont valides
        if (absIndexStart >= 0 && absIndexStart < static_cast<int>(timestamps.size()) &&
            absIndexEnd >= 0 && absIndexEnd < static_cast<int>(timestamps.size())) {
            xValueStart = timestamps[absIndexStart];
            xValueEnd = timestamps[absIndexEnd];
        } else {
            // Indices invalides, utiliser les valeurs par défaut
            xValueStart = xIndexStart;
            xValueEnd = xIndexEnd;
        }
    } else {
        // En mode agrégé, utiliser les données agrégées
        const auto& aggregatedData = dataManager.getAggregatedData(aggregationInfo.level);
        int startIndex = aggregationInfo.startIndex;
        
        // Convertir les indices relatifs en indices absolus
        int absIndexStart = startIndex + static_cast<int>(xIndexStart);
        int absIndexEnd = startIndex + static_cast<int>(xIndexEnd);
        
        // Vérifier que les indices sont valides
        if (absIndexStart >= 0 && absIndexStart < static_cast<int>(aggregatedData.timestamps.size()) &&
            absIndexEnd >= 0 && absIndexEnd < static_cast<int>(aggregatedData.timestamps.size())) {
            xValueStart = aggregatedData.timestamps[absIndexStart];
            xValueEnd = aggregatedData.timestamps[absIndexEnd];
        } else {
            // Indices invalides, utiliser les valeurs par défaut
            xValueStart = xIndexStart;
            xValueEnd = xIndexEnd;
        }
    }

    // Récupérer les timestamps formattés pour l'affichage
    const char* startTimeStr = c->xAxis()->getFormattedLabel(xIndexStart, "yyyy-mm-dd hh:nn:ss");
    const char* endTimeStr = c->xAxis()->getFormattedLabel(xIndexEnd, "yyyy-mm-dd hh:nn:ss");

    // Calculer la différence de temps en secondes (des vrais timestamps)
    double deltaX = fabs(xValueEnd - xValueStart);
    double deltaY = yValueEnd - yValueStart;

    // Définir la couleur du rectangle en fonction de deltaY
    int deltaColor = (deltaY < 0) ? 0xFF0000 : 0x008800;
    int alpha = 0xCC;
    int finalColor = (alpha << 24) | deltaColor;

    // Texte pour deltaX (au-dessus du rectangle)
    char bufferX[50];
    
    // Calculer la durée réelle en secondes (pour les TimeCharts)
    int totalSeconds = static_cast<int>(deltaX);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    if (hours > 0) {
        sprintf(bufferX, "%02dh%02dm%02ds", hours, minutes, seconds);
    } else if (minutes > 0) {
        sprintf(bufferX, "%02dm%02ds", minutes, seconds);
    } else {
        sprintf(bufferX, "%02ds", seconds);
    }
        
    // Texte pour deltaY (à droite du rectangle)
    char bufferY[50];
    
    // Ajouter le % de variation pour le deltaY si applicable
    if (yValueStart != 0) {
        double percentChange = (deltaY / yValueStart) * 100.0;
        sprintf(bufferY, "%+.2f \n(%.2f%%)", deltaY, percentChange);
    } else {
        sprintf(bufferY, "%.5f", deltaY);
    }

    // Dessiner le rectangle entre les deux points
    d->rect(startX, startY, endX, endY, deltaColor, finalColor);

    // Position pour le texte deltaX (au-dessus du rectangle)
    int textXPosX = (startX + endX) / 2;
    int textXPosY = std::min(startY, endY) - 15;
    
    // Position pour le texte deltaY (à droite du rectangle)
    int textYPosX = std::max(startX, endX) + 10;
    int textYPosY = (startY + endY) / 2;
    
    // Créer et afficher le texte pour deltaX
    TTFText* tForXDelta = d->text(bufferX, "Arial", 12);
    tForXDelta->draw(textXPosX, textXPosY, deltaColor, Chart::Bottom);
    tForXDelta->destroy();
    
    // Créer et afficher le texte pour deltaY
    TTFText* tForYDelta = d->text(bufferY, "Arial", 12);
    tForYDelta->draw(textYPosX, textYPosY, deltaColor, Chart::Left);
    tForYDelta->destroy();
}

void ChartRenderer::trackFinance(MultiChart* m, int mouseX, int mouseY, DrawArea* d)
{
    // Vérifier que le graphique n'est pas vide
    if (m->getChartCount() == 0)
        return;
    
    // Obtenir la valeur x la plus proche de la souris
    int xValue = (int)(((XYChart*)m->getChart(0))->getNearestXValue(mouseX));
    
    // Itérer sur tous les graphiques XY dans le MultiChart
    XYChart *c = 0;
    
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart*)m->getChart(i);
        
        // Variables pour les entrées de légende
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;
        
        // Itérer sur toutes les couches pour trouver le point de données le plus élevé
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer* layer = c->getLayerByZ(j);
            int xIndex = layer->getXIndexOf(xValue);
            int dataSetCount = layer->getDataSetCount();
            
            // Dans un FinanceChart, seules les couches montrant des données OHLC peuvent avoir 4 ensembles de données
            if (dataSetCount == 4) {
                double highValue = layer->getDataSet(0)->getValue(xIndex);
                double lowValue = layer->getDataSet(1)->getValue(xIndex);
                double openValue = layer->getDataSet(2)->getValue(xIndex);
                double closeValue = layer->getDataSet(3)->getValue(xIndex);
                
                if (closeValue != Chart::NoValue) {
                    // Construction de la légende OHLC
                    ohlcLegend << "      <*block*>";
                    ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
                    ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}"); 
                    ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}"); 
                    ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");
                    
                    // Ajouter une flèche et le % de variation si possible
                    double lastCloseValue = (xIndex > 0) ? 
                        layer->getDataSet(3)->getValue(xIndex - 1) : 
                        Chart::NoValue;

                    if (lastCloseValue != Chart::NoValue) {
                        double change = closeValue - lastCloseValue;
                        double percent = change * 100 / closeValue;
                        std::string symbol = (change >= 0) ?
                            "<*font,color=008800*><*img=@triangle,width=8,color=008800*>" :
                            "<*font,color=CC0000*><*img=@invertedtriangle,width=8,color=CC0000*>";

                        ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
                        ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                    }

                    ohlcLegend << "<*/*>";
                }
            } else {
                // Itérer sur tous les ensembles de données de la couche
                for (int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet* dataSet = layer->getDataSetByZ(k);
                    
                    std::string name = dataSet->getDataName();
                    double value = dataSet->getValue(xIndex);
                    if ((0 != name.size()) && (value != Chart::NoValue)) {
                        
                        // Extraction de l'unité
                        std::string unitChar;
                        
                        // Le nom de l'indicateur est la partie du nom jusqu'au caractère deux-points
                        int delimiterPosition = (int)name.find(':');
                        if ((int)name.npos != delimiterPosition) {
                            
                            // L'unité, le cas échéant, est le(s) caractère(s) non-chiffre(s) final(s)
                            int lastDigitPos = (int)name.find_last_of("0123456789");
                            if (((int)name.npos != lastDigitPos) && (lastDigitPos + 1 < (int)name.size())
                                && (lastDigitPos > delimiterPosition))
                                unitChar = name.substr(lastDigitPos + 1);
                            
                            name.resize(delimiterPosition);
                        }
                        
                        // Gestion des cas particuliers
                        if (dataSetCount == 2) {
                            // Si deux datasets, c'est probablement une plage
                            value = layer->getDataSet(0)->getValue(xIndex);
                            double value2 = layer->getDataSet(1)->getValue(xIndex);
                            name = name + ": " + c->formatValue((std::min)(value, value2), "{value|P3}");
                            name = name + " - " + c->formatValue((std::max)(value, value2), "{value|P3}");
                        } else {
                            // Cas spécial: volume (3 datasets pour up/down/flat)
                            if (dataSetCount == 3) {
                                // Le volume réel est la somme des 3 ensembles de données
                                value = layer->getDataSet(0)->getValue(xIndex) + layer->getDataSet(1
                                    )->getValue(xIndex) + layer->getDataSet(2)->getValue(xIndex);
                            }
                            
                            name = name + ": " + c->formatValue(value, "{value|P3}") + unitChar;
                        }
                        
                        // Construction de l'entrée de légende
                        std::ostringstream legendEntry;
                        legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color="
                            << std::hex << dataSet->getDataColor() << "*> " << name << "<*/*>";
                        legendEntries.push_back(legendEntry.str());
                    }
                }
            }
        }
        
        // Obtenir la position de la zone de tracé
        PlotArea* plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();
        int plotAreaBottomY = plotAreaTopY + plotArea->getHeight();
        
        // Seulement si on a pu récupérer la position Y
        if (mouseY >= plotAreaTopY && mouseY <= plotAreaBottomY) {
            double yValue = c->getYValue(mouseY - c->getAbsOffsetY());
            
            // Position du tooltip sur l'axe Y (côté droit de la zone de tracé)
            int yAxisTooltipX = plotAreaLeftX + plotArea->getWidth() + 5;
            int yAxisTooltipY = mouseY;
            
            // Créer le texte du tooltip avec la valeur Y formatée
            std::string yTooltipText = c->formatValue(yValue, "{value|P4}");
            
            // Dessiner un rectangle de fond pour le tooltip Y
            int tooltipWidth = 60;
            int tooltipHeight = 20;
            d->rect(yAxisTooltipX - 2, yAxisTooltipY - tooltipHeight/2 - 2, 
                    yAxisTooltipX + tooltipWidth + 2, yAxisTooltipY + tooltipHeight/2 + 2, 
                    0x000000, 0xffffcc);
            
            // Afficher le texte du tooltip Y
            TTFText* yTooltip = d->text(yTooltipText.c_str(), "Arial", 8);
            yTooltip->draw(yAxisTooltipX, yAxisTooltipY, 0x000000, Chart::Left);
            yTooltip->destroy();
            
            // Dessiner une ligne horizontale pour le crosshair Y
            d->hline(plotAreaLeftX, plotAreaLeftX + plotArea->getWidth(), 
                    yAxisTooltipY, d->dashLineColor(0x000000, 0x0101));
        }
        
        // La légende commence par l'étiquette de date
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy-mm-dd hh:nn:ss")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";
        
        // Dessiner une ligne de suivi verticale à la position x
        d->vline(plotAreaTopY, plotAreaTopY + plotArea->getHeight(), c->getXCoor(xValue) +
            c->getAbsOffsetX(), d->dashLineColor(0x000000, 0x0101));
        
        // Afficher la légende en haut de la zone de tracé
        TTFText* t = d->text(legendText.str().c_str(), "Arial", 8);
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 5, 0x000000, Chart::TopLeft);
        t->destroy();
        
        // Seulement pour le dernier graphique (celui du bas avec l'axe X visible)
        if (i == m->getChartCount() - 1) {
            // Obtenir le texte formaté du timestamp
            std::string timeStampText = c->xAxis()->getFormattedLabel(xValue, "yyyy-mm-dd hh:nn:ss");
            
            // Créer un fond rectangulaire pour le texte
            int textHeight = 16;
            int textWidth = 150;  // Ajuster selon la longueur du texte
            int xLabelPos = c->getXCoor(xValue) + c->getAbsOffsetX();
            int yLabelPos = plotAreaBottomY + 15;  // Position juste en dessous de l'axe X
            
            // Dessiner le fond du texte
            d->rect(xLabelPos - textWidth/2, yLabelPos - textHeight/2,
                   xLabelPos + textWidth/2, yLabelPos + textHeight/2,
                   0x000000, 0xffffcc);
            
            // Créer et dessiner le texte
            TTFText* timeLabel = d->text(timeStampText.c_str(), "Arial", 8);
            timeLabel->draw(xLabelPos, yLabelPos, 0x000000, Chart::Center);
            timeLabel->destroy();
            
            // Dessiner une petite marque verticale sur l'axe X
            d->vline(plotAreaBottomY, plotAreaBottomY + 5, xLabelPos, 0x000000);
        }
    }
}
