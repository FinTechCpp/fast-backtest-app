#include "ui/views/Stats/HistogramWidget.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QResizeEvent>
#include <numeric>
#include <algorithm>

// Implémentation de InteractiveChartView
InteractiveChartView::InteractiveChartView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent)
    , m_horizontalLine(nullptr)
    , m_verticalLine(nullptr)
    , m_tooltipItem(nullptr)
    , m_crosshairVisible(false)
{
    setMouseTracking(true);
    setRubberBand(QChartView::NoRubberBand);
}

void InteractiveChartView::setupCrosshairElements()
{
    if (!scene()) {
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
    
    if (!m_horizontalLine) {
        setupCrosshairElements();
    }
    
    if (chart() && !m_categories.isEmpty()) {
        QPointF chartPos = chart()->mapToValue(event->pos());
        
        // Discrétiser la position X
        int barIndex = qRound(chartPos.x());
        barIndex = qMax(0, qMin(barIndex, m_categories.size() - 1));
        
        // Pour le crosshair, utiliser la position de la souris convertie
        QPointF discreteViewPos = chart()->mapToPosition(QPointF(barIndex, chartPos.y()));
        
        showCrosshair(discreteViewPos);
        updateTooltip(QPointF(barIndex, chartPos.y()));
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
    
    int barIndex = qRound(chartPos.x());
    
    if (barIndex >= 0 && barIndex < m_categories.size()) {
        QString category = m_categories[barIndex];
        double value = m_values[barIndex];
        
        QString tooltipText = createTooltipText(category, value);
        m_tooltipItem->setHtml(tooltipText);
        
        QPointF scenePos = mapToScene(mapFromGlobal(QCursor::pos()));
        QRectF tooltipRect = m_tooltipItem->boundingRect();
        
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
    QString periodLabel;
    QString detailedInfo = category;
    
    if (m_fullDates.contains(category)) {
        QDateTime referenceDate = m_fullDates[category];
        
        if (category.contains("/") && category.length() <= 10) {
            periodLabel = "Jour";
            detailedInfo = referenceDate.toString("dddd dd MMMM yyyy");
        }
        else if (category.startsWith("S") && category.contains("(")) {
            periodLabel = "Semaine";
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
            periodLabel = "Mois";
            detailedInfo = referenceDate.toString("MMMM yyyy");
        }
        else if (category.startsWith("T")) {
            periodLabel = "Trimestre";
            int quarter = (referenceDate.date().month() - 1) / 3 + 1;
            QString quarterNames[] = {"", "1er trimestre", "2ème trimestre", "3ème trimestre", "4ème trimestre"};
            detailedInfo = QString("%1 %2").arg(quarterNames[quarter]).arg(referenceDate.date().year());
        }
        else if (category.length() == 4) {
            periodLabel = "Année";
            detailedInfo = QString("Année %1").arg(category);
        }
    }
    
    QString tooltipText = QString(
        "<div style='background-color: rgba(255,255,224,240); "
        "border: 1px solid gray; padding: 8px; border-radius: 4px;'>"
        "<b>%1:</b> %2<br/>"
        "<b>P&L total:</b> <span style='color: %4;'>%3€</span>"
        "</div>")
        .arg(periodLabel.isEmpty() ? "Période" : periodLabel)
        .arg(detailedInfo)
        .arg(QString::number(value, 'f', 2))
        .arg(value >= 0 ? "green" : "red");
    
    return tooltipText;
}

// Implémentation de HistogramWidget
HistogramWidget::HistogramWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent)
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_timeUnitCombo(nullptr)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_currentResults(nullptr)
{
    // Créer le widget de contenu
    m_contentWidget = new QWidget(this);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(1, 1, 1, 1);
    m_contentLayout->setSpacing(0);
            
    // ComboBox pour sélectionner l'unité de temps
    m_timeUnitCombo = new QComboBox(this);
    m_timeUnitCombo->addItems({"Jour", "Semaine", "Mois", "Trimestre", "Année"});
    m_timeUnitCombo->setCurrentIndex(0);  // Jour par défaut
    m_timeUnitCombo->setFixedWidth(100);

    // Style pour avoir des coins carrés
    // m_timeUnitCombo->setStyleSheet(
    //     "QComboBox {"
    //     "  border-radius: 0px;"       // Coins parfaitement carrés (était 1px)
    //     "  border: 1px solid black;"  // Bordure noire
    //     "  padding: 2px 10px 2px 5px;"
    //     "}"
    //     "QComboBox::drop-down {"
    //     "  border: none;"  /* Supprimer la bordure du bouton déroulant */
    //     "  width: 20px;"   /* Largeur fixe pour la zone de la flèche */
    //     "}"
    //     "QComboBox::down-arrow {"
    //     "  width: 0;"
    //     "  height: 0;"
    //     "  border-left: 4px solid transparent;"  /* Côté gauche du triangle */
    //     "  border-right: 4px solid transparent;" /* Côté droit du triangle */
    //     "  border-top: 4px solid black;"         /* Base du triangle (en haut) */
    //     "  margin-right: 5px;"                   /* Marge à droite pour le positionnement */
    //     "}"
    // );

    connect(m_timeUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &HistogramWidget::updateHistogram);
        
    // Ajouter le combo box comme widget compagnon du titre
    setTitleCompanionWidget(m_timeUnitCombo);
    
    // Créer le graphique
    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setVisible(false);
    m_chart->setBackgroundVisible(true);
    // m_chart->setBackgroundBrush(Qt::yellow);
    m_chart->setMargins(QMargins(0, 0, 0, 0));
    
    // Créer la vue du graphique
    m_chartView = new InteractiveChartView(m_chart, m_contentWidget);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);
    m_chartView->setContentsMargins(0, 0, 0, 0);
    
    // Ajouter la vue au layout
    m_contentLayout->addWidget(m_chartView);
}

HistogramWidget::~HistogramWidget()
{
    if (m_chart) {
        // Supprimer toutes les séries et axes
        const auto allSeries = m_chart->series();
        for (auto series : allSeries) {
            m_chart->removeSeries(series);
        }
        
        const auto allAxes = m_chart->axes();
        for (auto axis : allAxes) {
            m_chart->removeAxis(axis);
        }
        
        m_chart->deleteLater();
    }
}

void HistogramWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    // Le contenu est géré par les widgets enfants
    if (m_contentWidget) {
        m_contentWidget->setGeometry(contentRect);
    }
}

void HistogramWidget::resizeEvent(QResizeEvent* event)
{
    TitledWidget::resizeEvent(event);
    
    // Ajuster la taille du widget de contenu
    if (m_contentWidget) {
        m_contentWidget->setGeometry(contentRect());
    }
}

void HistogramWidget::updateData(BacktestResults* results)
{
    m_currentResults = results;
    updateHistogram();
}

void HistogramWidget::clear()
{
    if (m_chart) {
        const auto allSeries = m_chart->series();
        for (auto series : allSeries) {
            m_chart->removeSeries(series);
        }
    }
    m_currentResults = nullptr;
}

void HistogramWidget::updateHistogram()
{
    if (!m_currentResults || !m_timeUnitCombo) {
        m_chart->setTitle("Aucun trade à afficher");
        return;
    }
    
    // Extraire les trades des résultats
    std::vector<TradeInfo> trades = extractTradesFromResults(m_currentResults);
    
    if (trades.empty()) {
        m_chart->setTitle("Aucun trade à afficher");
        return;
    }
    
    // Récupérer l'unité de temps sélectionnée
    QString timeUnit = m_timeUnitCombo->currentText();

    // Regrouper les données par unité de temps
    GroupedData groupedData = groupDataByTimeUnit(trades, timeUnit);
    
    if (groupedData.categories.isEmpty()) {
        m_chart->setTitle("Impossible de regrouper les données");
        return;
    }
    
    // Créer le graphique
    createChart(groupedData);
}

std::vector<HistogramWidget::TradeInfo> HistogramWidget::extractTradesFromResults(BacktestResults* results)
{
    std::vector<TradeInfo> trades;
    
    if (!results) {
        return trades;
    }
    
    for (const auto& trade : results->stats.trades) {
        TradeInfo info;
        info.exitTime = QDateTime(
            QDate(trade.exitDate.year, trade.exitDate.month, trade.exitDate.day),
            QTime(trade.exitDate.hour, trade.exitDate.minute, trade.exitDate.second)
        );
        info.pnl = trade.pl;
        trades.push_back(info);
    }
    
    return trades;
}

HistogramWidget::GroupedData HistogramWidget::groupDataByTimeUnit(
    const std::vector<TradeInfo>& trades, const QString& timeUnit)
{
    GroupedData result;
    
    if (trades.empty()) {
        return result;
    }
    
    // Map pour stocker les PnL par période
    QMap<QString, double> periodPnL;
    QMap<QString, QDateTime> periodDates;
    
    for (const TradeInfo& trade : trades) {
        QDateTime exitTime = trade.exitTime;
        
        if (!exitTime.isValid()) {
            continue;
        }
        
        // Générer la clé de période
        QString periodKey = generatePeriodKey(exitTime, timeUnit);
        
        if (periodKey.isEmpty()) {
            continue;
        }
        
        // Accumuler le PnL
        periodPnL[periodKey] += trade.pnl;
        
        // Stocker une date représentative
        if (!periodDates.contains(periodKey)) {
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
    
    return result;
}

void HistogramWidget::createChart(const GroupedData& data)
{
    if (!m_chart) {
        return;
    }

    // Désactiver les animations pour les grands ensembles
    if (data.categories.size() > 50) {
        m_chart->setAnimationOptions(QChart::NoAnimation);
    } else {
        m_chart->setAnimationOptions(QChart::SeriesAnimations);
    }
    
    // Nettoyer le graphique
    const auto seriesToRemove = m_chart->series();
    for (auto series : seriesToRemove) {
        m_chart->removeSeries(series);
    }
    
    const auto axesToRemove = m_chart->axes();
    for (auto axis : axesToRemove) {
        m_chart->removeAxis(axis);
    }
    
    // Créer la série
    QStackedBarSeries* barSeries = new QStackedBarSeries();
    
    // Créer les ensembles de barres
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
    
    barSeries->append(gainsSet);
    barSeries->append(lossesSet);
    m_chart->addSeries(barSeries);
    
    // Créer les axes
    QBarCategoryAxis* axisX = new QBarCategoryAxis();

    // Limiter le nombre de labels affichés pour garder une bonne lisibilité
    const int maxLabels = 10;  // Nombre maximum de labels à afficher
    int categoryCount = data.categories.size();
    
    // Afficher toutes les catégories si leur nombre est inférieur à maxLabels
    // int off = 28;
    // axisX->append(data.categories[0 + off]);
    // axisX->append(data.categories[1 + off]);
    // axisX->append(data.categories[2 + off]);
    // axisX->append(data.categories[3 + off]);

    axisX->append(data.categories);

    // Toujours garder les labels horizontaux
    // if (categoryCount > maxLabels) {
    //     axisX->setLabelsAngle(-45.0);
    // }

    axisX->setLabelsVisible(false);

    
    QValueAxis* axisY = new QValueAxis();
    
    // Configurer l'échelle Y
    if (!data.values.isEmpty()) {
        double minValue = *std::min_element(data.values.begin(), data.values.end());
        double maxValue = *std::max_element(data.values.begin(), data.values.end());
        
        double range = maxValue - minValue;
        double margin = range > 0 ? range * 0.1 : 100.0;
        
        axisY->setRange(minValue - margin, maxValue + margin);
    }
    
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignRight);
    
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    
    // Configurer le titre
    QString timeUnit = m_timeUnitCombo->currentText();
    double totalPnL = std::accumulate(data.values.begin(), data.values.end(), 0.0);
    
    // Configurer l'apparence
    m_chart->setBackgroundRoundness(0);
    
    // Configurer le tooltip
    if (m_chartView) {
        m_chartView->setTooltipData(data.categories, data.values, data.fullDates);
    }
}

QDateTime HistogramWidget::getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit)
{
    QDate date = dateTime.date();
    
    if (timeUnit == "Jour") {
        return QDateTime(date, QTime(0, 0, 0));
    }
    else if (timeUnit == "Semaine") {
        QDate weekStart = date.addDays(-(date.dayOfWeek() - 1));
        return QDateTime(weekStart, QTime(0, 0, 0));
    }
    else if (timeUnit == "Mois") {
        return QDateTime(QDate(date.year(), date.month(), 1), QTime(0, 0, 0));
    }
    else if (timeUnit == "Trimestre") {
        int quarter = (date.month() - 1) / 3 + 1;
        int firstMonthOfQuarter = (quarter - 1) * 3 + 1;
        return QDateTime(QDate(date.year(), firstMonthOfQuarter, 1), QTime(0, 0, 0));
    }
    else if (timeUnit == "Année") {
        return QDateTime(QDate(date.year(), 1, 1), QTime(0, 0, 0));
    }
    
    return dateTime;
}

QString HistogramWidget::generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit)
{
    if (!dateTime.isValid()) {
        return QString();
    }
    
    QDate date = dateTime.date();
    
    if (timeUnit == "Jour") {
        // Si c'est le premier du mois on affiche le mois en toute lettre
        // si on est le premier janvier on affiche l'année
        // sinon on affiche juste le jour

        // if (date == QDate(date.year(), 1, 1)) {
        //     return date.toString("yyyy");
        // }
        // else if (date.day() == 1) {
        //     return date.toString("MMM");
        // }

        return date.toString("dd/MM/yyyy");
    }
    else if (timeUnit == "Semaine") {
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