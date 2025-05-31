#include "views/histogram_view.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QFont>

InteractiveChartView::InteractiveChartView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent)
    , m_horizontalLine(nullptr)
    , m_verticalLine(nullptr)
    , m_tooltipItem(nullptr)
    , m_crosshairVisible(false)
{
    setMouseTracking(true);
    setRubberBand(QChartView::NoRubberBand);
    
    // CORRECTION Qt 6: Pas d'initialisation immédiate des éléments crosshair
    // Les éléments seront créés quand la scene sera disponible
}

void InteractiveChartView::setupCrosshairElements()
{
    if (!scene()) {
        qWarning() << "Scene non disponible pour créer les éléments crosshair";
        return;
    }
    
    // Créer les lignes de crosshair
    QPen crosshairPen(Qt::gray, 1, Qt::DashLine);
    
    if (!m_horizontalLine) {
        m_horizontalLine = new QGraphicsLineItem();
        m_horizontalLine->setPen(crosshairPen);
        m_horizontalLine->setVisible(false);
        scene()->addItem(m_horizontalLine);
    }
    
    if (!m_verticalLine) {
        m_verticalLine = new QGraphicsLineItem();
        m_verticalLine->setPen(crosshairPen);
        m_verticalLine->setVisible(false);
        scene()->addItem(m_verticalLine);
    }
    
    // Créer l'élément de tooltip
    if (!m_tooltipItem) {
        m_tooltipItem = new QGraphicsTextItem();
        m_tooltipItem->setFont(QFont("Arial", 10));
        m_tooltipItem->setDefaultTextColor(Qt::black);
        m_tooltipItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        m_tooltipItem->setVisible(false);
        scene()->addItem(m_tooltipItem);
    }
}

void InteractiveChartView::setTooltipData(const QStringList& categories, const QList<double>& values, 
                                         const QMap<QString, QDateTime>& fullDates)
{
    m_categories = categories;
    m_values = values;
    m_fullDates = fullDates;
}

void InteractiveChartView::mouseMoveEvent(QMouseEvent* event)
{
    QChartView::mouseMoveEvent(event);
    
    // CORRECTION Qt 6: S'assurer que les éléments crosshair existent
    if (!m_horizontalLine) {
        setupCrosshairElements();
    }
    
    if (chart() && !m_categories.isEmpty()) {
        QPointF chartPos = chart()->mapToValue(event->pos());
        
        // Discrétiser la position X
        int barIndex = qRound(chartPos.x());
        barIndex = qMax(0, qMin(barIndex, m_categories.size() - 1));
        
        // Utiliser la position discrétisée pour le tooltip
        QPointF discreteChartPos(barIndex, chartPos.y());
        
        // Pour le crosshair, utiliser la position de la souris convertie
        QPointF discreteViewPos = chart()->mapToPosition(QPointF(barIndex, chartPos.y()));
        
        showCrosshair(discreteViewPos);
        updateTooltip(discreteChartPos);
    }
}

void InteractiveChartView::leaveEvent(QEvent* event)
{
    QChartView::leaveEvent(event);
    hideCrosshair();
}

void InteractiveChartView::showCrosshair(const QPointF& position)
{
    if (!chart() || !m_horizontalLine || !m_verticalLine) return;
    
    QRectF plotArea = chart()->plotArea();
    
    // Ligne horizontale
    m_horizontalLine->setLine(plotArea.left(), position.y(), 
                             plotArea.right(), position.y());
    m_horizontalLine->setVisible(true);
    
    // Ligne verticale
    m_verticalLine->setLine(position.x(), plotArea.top(), 
                           position.x(), plotArea.bottom());
    m_verticalLine->setVisible(true);
    
    m_crosshairVisible = true;
}

void InteractiveChartView::hideCrosshair()
{
    if (m_horizontalLine) m_horizontalLine->setVisible(false);
    if (m_verticalLine) m_verticalLine->setVisible(false);
    if (m_tooltipItem) m_tooltipItem->setVisible(false);
    m_crosshairVisible = false;
}

void InteractiveChartView::updateTooltip(const QPointF& chartPos)
{
    if (m_categories.isEmpty() || m_values.isEmpty()) return;
    
    // Déterminer quelle barre est sous le curseur
    int barIndex = qRound(chartPos.x());
    
    if (barIndex >= 0 && barIndex < m_categories.size()) {
        QString category = m_categories[barIndex];
        double value = m_values[barIndex];
        
        // Créer le texte du tooltip avec des informations adaptées à la période
        QString tooltipText = createTooltipText(category, value);
        
        m_tooltipItem->setHtml(tooltipText);
        
        // Positionner le tooltip près du curseur
        QPointF scenePos = mapToScene(mapFromGlobal(QCursor::pos()));
        QRectF tooltipRect = m_tooltipItem->boundingRect();
        
        // Ajuster la position pour éviter que le tooltip sorte de l'écran
        qreal x = scenePos.x() + 15;
        qreal y = scenePos.y() - tooltipRect.height() - 15;
        
        QRectF chartRect = chart()->plotArea();
        if (x + tooltipRect.width() > chartRect.right()) {
            x = scenePos.x() - tooltipRect.width() - 15;
        }
        if (y < chartRect.top()) {
            y = scenePos.y() + 15;
        }
        
        m_tooltipItem->setPos(x, y);
        m_tooltipItem->setVisible(true);
    }
}

QString InteractiveChartView::createTooltipText(const QString& category, double value)
{
    // Déterminer le type de période à partir de la catégorie
    QString periodLabel;
    QString detailedInfo = category; // Par défaut, utiliser la catégorie
    
    if (m_fullDates.contains(category)) {
        QDateTime referenceDate = m_fullDates[category];
        
        // Analyser le format de la catégorie pour déterminer le type de période
        if (category.contains("/") && category.length() <= 10) {
            // Format jour: "27/03/2025"
            periodLabel = "Jour";
            detailedInfo = referenceDate.toString("dddd dd MMMM yyyy");
        }
        else if (category.startsWith("S") && category.contains("(")) {
            // Format semaine: "S13 (25/03 - 31/03)"
            periodLabel = "Semaine";
            // Extraire le numéro de semaine et l'année
            int weekNumber = referenceDate.date().weekNumber();
            int year = referenceDate.date().year();
            QDate weekStart = referenceDate.date().addDays(-(referenceDate.date().dayOfWeek() - 1));
            QDate weekEnd = weekStart.addDays(6);
            detailedInfo = QString("S%1 de %2 (%3 au %4)")
                          .arg(weekNumber)
                          .arg(year)
                          .arg(weekStart.toString("dd MMMM"))
                          .arg(weekEnd.toString("dd MMMM"));
        }
        else if (category.contains("/") && category.length() == 7) {
            // Format mois: "03/2025"
            periodLabel = "Mois";
            detailedInfo = referenceDate.toString("MMMM yyyy");
        }
        else if (category.startsWith("T")) {
            // Format trimestre: "T1 2025"
            periodLabel = "Trimestre";
            int quarter = (referenceDate.date().month() - 1) / 3 + 1;
            QString quarterNames[] = {"", "1er trimestre", "2ème trimestre", "3ème trimestre", "4ème trimestre"};
            detailedInfo = QString("%1 %2").arg(quarterNames[quarter]).arg(referenceDate.date().year());
        }
        else if (category.length() == 4) {
            // Format année: "2025"
            periodLabel = "Année";
            detailedInfo = QString("Année %1").arg(category);
        }
    }
    
    // Créer le texte du tooltip
    QString tooltipText = QString(
        "<div style='background-color: rgba(255,255,224,240); "
        "border: 1px solid gray; padding: 8px; border-radius: 4px;'>"
        "<b>%1:</b> %2<br/>"
        "<b>P&L total:</b> <span style='color: %4;'>%3$</span>"
        "</div>")
        .arg(periodLabel.isEmpty() ? "Période" : periodLabel)
        .arg(detailedInfo)
        .arg(QString::number(value, 'f', 2))
        .arg(value >= 0 ? "green" : "red");
    
    return tooltipText;
}

HistogramView::HistogramView(QWidget* parent)
    : BaseView(parent)
    , m_timeUnitCombo(nullptr)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_currentResults(nullptr) // Mise à jour du type
{
    qDebug() << "HistogramView créée avec parent:" << parent;
    setupUI();
}

HistogramView::~HistogramView()
{
    // CORRECTION Qt 6: Nettoyage explicite et sécurisé
    if (m_chart) {
        // Supprimer toutes les séries AVANT de supprimer le chart
        const auto allSeries = m_chart->series();
        for (auto series : allSeries) {
            m_chart->removeSeries(series);
            // IMPORTANT: Ne pas appeler deleteLater() ici, Qt s'en charge
        }
        
        // Supprimer tous les axes
        const auto allAxes = m_chart->axes();
        for (auto axis : allAxes) {
            m_chart->removeAxis(axis);
            // IMPORTANT: Ne pas appeler deleteLater() ici, Qt s'en charge
        }
        
        // Maintenant, supprimer le chart
        m_chart->deleteLater();
        m_chart = nullptr;
    }
    
    // Le m_chartView sera automatiquement détruit par Qt car c'est un enfant
}

void HistogramView::setupUI()
{
    QTime start = QTime::currentTime();
    
    // Créer les contrôles pour sélectionner l'unité de temps
    QWidget* controlsWidget = new QWidget(this);
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 10);
    
    // Label pour l'unité de temps
    controlsLayout->addWidget(new QLabel("Unité de temps:", controlsWidget));
    
    // ComboBox pour sélectionner l'unité de temps
    m_timeUnitCombo = new QComboBox(controlsWidget);
    m_timeUnitCombo->addItems({"Jour", "Semaine", "Mois", "Trimestre", "Année"});
    m_timeUnitCombo->setCurrentIndex(0);
    connect(m_timeUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &HistogramView::updateHistogram);
    controlsLayout->addWidget(m_timeUnitCombo);
    
    controlsLayout->addStretch();
    
    // Ajouter les contrôles au layout principal
    m_mainLayout->addWidget(controlsWidget);
    
    // CORRECTION Qt 6: Ordre de création critique
    // 1. Créer le chart SANS parent
    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setVisible(false);
    m_chart->setTitle("Exécutez le backtest pour afficher l'histogramme des gains/pertes");
    
    // 2. Créer le chartView avec le chart ET le parent
    m_chartView = new InteractiveChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    
    // 3. Ajouter au layout
    m_mainLayout->addWidget(m_chartView);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "HistogramView::setupUI() took" << elapsed << "ms";
}

void HistogramView::updateData(BacktestResults* results)
{
    QTime start = QTime::currentTime();
    
    // Stocker les données pour les mises à jour ultérieures
    m_currentResults = results;
    
    qDebug() << "HistogramView::updateData() appelé avec results:" << results;
    
    // Mettre à jour l'histogramme
    if (results) {
        updateHistogram();
    } else {
        clear();
    }
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "HistogramView::updateData() took" << elapsed << "ms";
}

void HistogramView::clear()
{
    if (m_chart) {
        // CORRECTION Qt 6: Nettoyage sécurisé
        const auto allSeries = m_chart->series();
        for (auto series : allSeries) {
            m_chart->removeSeries(series);
        }
        m_chart->setTitle("Exécutez le backtest pour afficher l'histogramme des gains/pertes");
    }
    m_currentResults = nullptr;
}

void HistogramView::updateHistogram()
{
    if (!m_currentResults || !m_timeUnitCombo) {
        return;
    }
    
    qDebug() << "Mise à jour de l'histogramme avec les données C++...";
    
    // Extraire les trades directement depuis les résultats C++
    std::vector<TradeInfo> trades = extractTradesFromResults(m_currentResults);
    
    if (trades.empty()) {
        qWarning() << "Aucun trade trouvé dans les données";
        m_chart->setTitle("Aucun trade à afficher");
        return;
    }
    
    qDebug() << "Nombre de trades récupérés:" << trades.size();
    
    // Récupérer l'unité de temps sélectionnée
    QString timeUnit = m_timeUnitCombo->currentText();
    
    // Regrouper les données par unité de temps
    GroupedData groupedData = groupDataByTimeUnit(trades, timeUnit);
    
    if (groupedData.categories.isEmpty()) {
        qWarning() << "Aucune donnée groupée disponible";
        m_chart->setTitle("Impossible de regrouper les données");
        return;
    }
    
    // Créer le graphique
    createChart(groupedData);
    
    qDebug() << "Histogramme mis à jour avec" << groupedData.categories.size() << "périodes";
}

// Nouvelle méthode pour extraire les trades depuis les résultats C++
std::vector<HistogramView::TradeInfo> HistogramView::extractTradesFromResults(BacktestResults* results)
{
    std::vector<TradeInfo> trades;
    
    if (!results) {
        return trades;
    }
    
    try {
        // Parcourir tous les trades dans les résultats
        for (const auto& tradePtr : results->stats.trades) {
            if (!tradePtr) continue;
            
            TradeInfo info;
            info.exitTime = QDateTime(
                QDate(tradePtr->exitDate().getYear(), tradePtr->exitDate().getMonth(), tradePtr->exitDate().getDay()),
                QTime(tradePtr->exitDate().getHour(), tradePtr->exitDate().getMinute(), tradePtr->exitDate().getSecond())
            );
            info.pnl = tradePtr->pl();
            info.isLong = tradePtr->size() > 0;
            
            trades.push_back(info);
        }
        
        qDebug() << "Nombre de trades extraits:" << trades.size();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception lors de l'extraction des trades:" << e.what();
    }
    
    return trades;
}

// Mettre à jour groupDataByTimeUnit pour utiliser std::vector<TradeInfo>
HistogramView::GroupedData HistogramView::groupDataByTimeUnit(
    const std::vector<TradeInfo>& trades, const QString& timeUnit)
{
    GroupedData result;
    
    if (trades.empty()) {
        return result;
    }
    
    qDebug() << "Regroupement des données par" << timeUnit;
    
    // Map pour stocker les PnL par période
    QMap<QString, double> periodPnL;
    QMap<QString, QDateTime> periodDates; // Pour trier chronologiquement
    
    for (const TradeInfo& trade : trades) {
        // Extraire la date de sortie
        QDateTime exitTime = trade.exitTime;
        
        if (!exitTime.isValid()) {
            qWarning() << "Date de sortie invalide pour un trade";
            continue;
        }
        
        // Générer la clé de période selon l'unité de temps
        QString periodKey = generatePeriodKey(exitTime, timeUnit);
        
        if (periodKey.isEmpty()) {
            continue;
        }
        
        // Accumuler le PnL pour cette période
        periodPnL[periodKey] += trade.pnl;
        
        // Stocker une date représentative de la période
        if (!periodDates.contains(periodKey)) {
            // Pour les périodes plus longues, utiliser le début de la période
            QDateTime representativeDate = getRepresentativeDate(exitTime, timeUnit);
            periodDates[periodKey] = representativeDate;
        }
    }
    
    // Trier les périodes chronologiquement
    QList<QPair<QDateTime, QString>> sortedPeriods;
    for (auto it = periodDates.begin(); it != periodDates.end(); ++it) {
        sortedPeriods.append(qMakePair(it.value(), it.key()));
    }
    
    std::sort(sortedPeriods.begin(), sortedPeriods.end());
    
    // Construire les résultats triés
    for (const auto& pair : sortedPeriods) {
        QString periodKey = pair.second;
        result.categories.append(periodKey);
        result.values.append(periodPnL[periodKey]);
        result.fullDates[periodKey] = pair.first;
    }
    
    qDebug() << "Données regroupées en" << result.categories.size() << "périodes";
    
    return result;
}

// void HistogramView::createChart(const GroupedData& data)
// {
//     // Vérifier si des données sont disponibles
//     if (data.categories.isEmpty() || data.values.isEmpty()) {
//         qWarning() << "Aucune donnée à afficher dans l'histogramme";
//         return;
//     }
    
//     // Créer un nouveau graphique s'il n'existe pas
//     if (!m_chart) {
//         m_chart = new QChart();
//         m_chart->setAnimationOptions(QChart::SeriesAnimations);
//         m_chart->setTheme(QChart::ChartThemeDark);
        
//         // Si le ChartView n'existe pas, le créer aussi
//         if (!m_chartView) {
//             m_chartView = new InteractiveChartView(m_chart);
//             m_chartView->setRenderHint(QPainter::Antialiasing);
            
//             // Ajouter au layout
//             if (m_mainLayout) {
//                 m_mainLayout->addWidget(m_chartView);
//             }
//         } else {
//             m_chartView->setChart(m_chart);
//         }
//     } else {
//         // Nettoyer le graphique existant
//         m_chart->removeAllSeries();
//         m_chart->removeAxis(m_chart->axisX());
//         m_chart->removeAxis(m_chart->axisY());
//     }
    
//     // Préparer les données pour l'histogramme
//     double maxValue = 0;
//     double minValue = 0;
//     QBarSet* positiveSet = new QBarSet("Gains");
//     QBarSet* negativeSet = new QBarSet("Pertes");
//     positiveSet->setColor(QColor(0, 180, 0));
//     negativeSet->setColor(QColor(180, 0, 0));
    
//     // Analyser les données pour l'histogramme
//     for (const double& value : data.values) {
//         if (value >= 0) {
//             *positiveSet << value;
//             *negativeSet << 0;
//             maxValue = qMax(maxValue, value);
//         } else {
//             *positiveSet << 0;
//             *negativeSet << qAbs(value);  // Valeur absolue pour l'affichage
//             minValue = qMin(minValue, value);
//         }
//     }
    
//     // Créer la série de barres
//     QBarSeries* series = new QBarSeries();
//     series->append(positiveSet);
//     series->append(negativeSet);
//     series->setBarWidth(0.9);
    
//     // Configurer l'axe des X
//     QBarCategoryAxis* axisX = new QBarCategoryAxis();
//     axisX->append(data.categories);
//     m_chart->addAxis(axisX, Qt::AlignBottom);
//     series->attachAxis(axisX);
    
//     // Configurer l'axe des Y
//     QValueAxis* axisY = new QValueAxis();
//     double range = qMax(maxValue, qAbs(minValue)) * 1.1;  // Ajouter une marge
//     axisY->setRange(-range * 0.1, range);  // Petite marge en bas
//     axisY->setTickCount(6);
//     axisY->setLabelFormat("%.2f");
//     m_chart->addAxis(axisY, Qt::AlignLeft);
//     series->attachAxis(axisY);
    
//     // Ajouter la série au graphique
//     m_chart->addSeries(series);
    
//     // Titre du graphique
//     QString timeUnit = m_timeUnitCombo ? m_timeUnitCombo->currentText() : "Période";
//     m_chart->setTitle(QString("Distribution des profits et pertes par %1").arg(timeUnit));
    
//     // Configurer les tooltips
//     connect(series, &QBarSeries::hovered, [this, &data](bool status, int index, QBarSet* barset) {
//         if (status && index >= 0 && index < data.categories.size()) {
//             QString category = data.categories[index];
//             double value = data.values[index];
            
//             // Format pour la tooltip
//             QString tooltipText;
//             if (data.fullDates.contains(category)) {
//                 QDateTime date = data.fullDates[category];
//                 tooltipText = QString("%1\nP&L: %2")
//                     .arg(date.toString("dd/MM/yyyy"))
//                     .arg(value, 0, 'f', 2);
//             } else {
//                 tooltipText = QString("%1\nP&L: %2")
//                     .arg(category)
//                     .arg(value, 0, 'f', 2);
//             }
            
//             QToolTip::showText(QCursor::pos(), tooltipText);
//         }
//     });
    
//     // Ajuster la vue
//     m_chartView->setRubberBand(QChartView::HorizontalRubberBand);
// }

void HistogramView::createChart(const GroupedData& data)
{
    if (!m_chart) {
        qCritical() << "Chart non initialisé dans createChart";
        return;
    }
    
    qDebug() << "Début de createChart avec" << data.categories.size() << "catégories";
    
    try {
        // CORRECTION Qt 6: Nettoyage sécurisé et COMPLET
        // Étape 1: Collecter toutes les séries et axes à supprimer
        QList<QAbstractSeries*> seriesToRemove;
        QList<QAbstractAxis*> axesToRemove;
        
        const auto currentSeries = m_chart->series();
        for (auto series : currentSeries) {
            seriesToRemove.append(series);
        }
        
        const auto currentAxes = m_chart->axes();
        for (auto axis : currentAxes) {
            axesToRemove.append(axis);
        }
        
        // Étape 2: Supprimer proprement (ordre important!)
        for (auto series : seriesToRemove) {
            m_chart->removeSeries(series);
            // Qt 6: Ne pas appeler delete manuellement
        }
        
        for (auto axis : axesToRemove) {
            m_chart->removeAxis(axis);
            // Qt 6: Ne pas appeler delete manuellement
        }
        
        qDebug() << "Nettoyage terminé, création des nouvelles séries...";
        
        // CORRECTION Qt 6: Créer la série SANS parent, elle sera adoptée par le chart
        QStackedBarSeries* barSeries = new QStackedBarSeries();
        
        // Créer les BarSets SANS parent explicite
        QBarSet* gainsSet = new QBarSet("Gains");
        QBarSet* lossesSet = new QBarSet("Pertes");
        
        // Configurer les couleurs
        gainsSet->setColor(QColor(76, 175, 80));      
        gainsSet->setBorderColor(QColor(56, 142, 60));
        
        lossesSet->setColor(QColor(244, 67, 54));     
        lossesSet->setBorderColor(QColor(198, 40, 40));
        
        // Séparer les données en gains et pertes
        for (int i = 0; i < data.values.size(); ++i) {
            double value = data.values[i];
            
            if (value >= 0) {
                gainsSet->append(value);
                lossesSet->append(0);  
            } else {
                gainsSet->append(0);   
                lossesSet->append(value);
            }
        }
        
        // CORRECTION Qt 6: Ajouter les sets à la série AVANT d'ajouter au chart
        barSeries->append(gainsSet);
        barSeries->append(lossesSet);
        
        qDebug() << "Sets ajoutés à la série, ajout au chart...";
        
        // CORRECTION Qt 6: Ajouter la série au chart (prend ownership)
        m_chart->addSeries(barSeries);
        
        qDebug() << "Série ajoutée au chart, création des axes...";
        
        // CORRECTION Qt 6: Créer les axes APRÈS avoir ajouté la série
        QBarCategoryAxis* axisX = new QBarCategoryAxis();
        axisX->append(data.categories);
        axisX->setTitleText("Période");
        
        // Rotation des labels si nécessaire
        if (data.categories.size() > 10) {
            axisX->setLabelsAngle(-45);
        }
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("P&L ($)");
        
        // Configurer l'échelle Y pour inclure zéro
        if (!data.values.isEmpty()) {
            double minValue = *std::min_element(data.values.begin(), data.values.end());
            double maxValue = *std::max_element(data.values.begin(), data.values.end());
            
            double range = maxValue - minValue;
            double margin = range > 0 ? range * 0.1 : 100.0; // Marge par défaut si range = 0
            
            axisY->setRange(minValue - margin, maxValue + margin);
        }
        
        qDebug() << "Axes configurés, ajout au chart...";
        
        // CORRECTION Qt 6: Ajouter les axes au chart AVANT d'attacher les séries
        m_chart->addAxis(axisX, Qt::AlignBottom);
        m_chart->addAxis(axisY, Qt::AlignLeft);
        
        qDebug() << "Axes ajoutés, attachement aux séries...";
        
        // CORRECTION Qt 6: Attacher les axes aux séries APRÈS les avoir ajoutés au chart
        barSeries->attachAxis(axisX);
        barSeries->attachAxis(axisY);
        
        qDebug() << "Axes attachés avec succès";
        
        // Configurer le titre
        QString timeUnit = m_timeUnitCombo->currentText();
        double totalPnL = std::accumulate(data.values.begin(), data.values.end(), 0.0);
        
        m_chart->setTitle(QString("Histogramme P&L par %1 (Total: %2$)")
                         .arg(timeUnit.toLower())
                         .arg(QString::number(totalPnL, 'f', 2)));
        
        // Configurer l'apparence générale
        m_chart->setBackgroundRoundness(0);
        m_chart->legend()->setVisible(true);
        m_chart->setMargins(QMargins(10, 10, 10, 10));
        
        // Configurer les données du tooltip pour le chartView interactif
        if (m_chartView) {
            m_chartView->setTooltipData(data.categories, data.values, data.fullDates);
        }
        
        qDebug() << "createChart terminé avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Exception dans createChart:" << e.what();
        // En cas d'erreur, nettoyer proprement
        if (m_chart) {
            const auto series = m_chart->series();
            for (auto s : series) {
                m_chart->removeSeries(s);
            }
        }
    } catch (...) {
        qCritical() << "Exception inconnue dans createChart";
        // En cas d'erreur, nettoyer proprement
        if (m_chart) {
            const auto series = m_chart->series();
            for (auto s : series) {
                m_chart->removeSeries(s);
            }
        }
    }
}

QDateTime HistogramView::getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit)
{
    QDate date = dateTime.date();
    
    if (timeUnit == "Jour") {
        // Pour un jour, utiliser 00:00 du jour
        return QDateTime(date, QTime(0, 0, 0));
    }
    else if (timeUnit == "Semaine") {
        // Pour une semaine, utiliser le lundi 00:00
        QDate weekStart = date.addDays(-(date.dayOfWeek() - 1));
        return QDateTime(weekStart, QTime(0, 0, 0));
    }
    else if (timeUnit == "Mois") {
        // Pour un mois, utiliser le 1er du mois 00:00
        return QDateTime(QDate(date.year(), date.month(), 1), QTime(0, 0, 0));
    }
    else if (timeUnit == "Trimestre") {
        // Pour un trimestre, utiliser le 1er jour du trimestre
        int quarter = (date.month() - 1) / 3 + 1;
        int firstMonthOfQuarter = (quarter - 1) * 3 + 1;
        return QDateTime(QDate(date.year(), firstMonthOfQuarter, 1), QTime(0, 0, 0));
    }
    else if (timeUnit == "Année") {
        // Pour une année, utiliser le 1er janvier 00:00
        return QDateTime(QDate(date.year(), 1, 1), QTime(0, 0, 0));
    }
    
    return dateTime; // Fallback
}

QDateTime HistogramView::parseDateTime(const QString& dateTimeStr)
{
    // Format attendu: "2025-03-27 15:30:40"
    QDateTime dateTime = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd hh:mm:ss");
    
    if (!dateTime.isValid()) {
        // Essayer d'autres formats si nécessaire
        dateTime = QDateTime::fromString(dateTimeStr, Qt::ISODate);
    }
    
    return dateTime;
}

QString HistogramView::generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit)
{
    if (!dateTime.isValid()) {
        return QString();
    }
    
    QDate date = dateTime.date();
    
    if (timeUnit == "Jour") {
        return date.toString("dd/MM/yyyy");
    }
    else if (timeUnit == "Semaine") {
        // Calculer le début de la semaine (lundi)
        QDate weekStart = date.addDays(-(date.dayOfWeek() - 1));
        QDate weekEnd = weekStart.addDays(6);
        return QString("S%1 (%2 - %3)")
               .arg(weekStart.weekNumber())
               .arg(weekStart.toString("dd/MM"))
               .arg(weekEnd.toString("dd/MM"));
    }
    else if (timeUnit == "Mois") {
        return date.toString("MM/yyyy");
    }
    else if (timeUnit == "Trimestre") {
        int quarter = (date.month() - 1) / 3 + 1;
        return QString("T%1 %2").arg(quarter).arg(date.year());
    }
    else if (timeUnit == "Année") {
        return date.toString("yyyy");
    }
    
    return QString();
}