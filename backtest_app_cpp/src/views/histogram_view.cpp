#include "histogram_view.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QFont>

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
    
    // Créer les lignes de crosshair
    QPen crosshairPen(Qt::gray, 1, Qt::DashLine);
    
    m_horizontalLine = new QGraphicsLineItem();
    m_horizontalLine->setPen(crosshairPen);
    m_horizontalLine->setVisible(false);
    scene()->addItem(m_horizontalLine);
    
    m_verticalLine = new QGraphicsLineItem();
    m_verticalLine->setPen(crosshairPen);
    m_verticalLine->setVisible(false);
    scene()->addItem(m_verticalLine);
    
    // Créer l'élément de tooltip
    m_tooltipItem = new QGraphicsTextItem();
    m_tooltipItem->setFont(QFont("Arial", 10));
    m_tooltipItem->setDefaultTextColor(Qt::black);
    
    // Style du tooltip
    QBrush tooltipBrush(QColor(255, 255, 224, 200)); // Jaune clair semi-transparent
    m_tooltipItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_tooltipItem->setVisible(false);
    scene()->addItem(m_tooltipItem);
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
    if (!chart()) return;
    
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
    , m_currentStats(nullptr)
{
    qDebug() << "HistogramView créée avec parent:" << parent;

    setupUI();
}

HistogramView::~HistogramView()
{
    // Les widgets enfants sont détruits automatiquement par Qt
}

void HistogramView::setupUI()
{
    QTime start = QTime::currentTime();
    
    // Créer les contrôles pour sélectionner l'unité de temps
    QWidget* controlsWidget = new QWidget();
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 10);
    
    // Label pour l'unité de temps
    controlsLayout->addWidget(new QLabel("Unité de temps:"));
    
    // ComboBox pour sélectionner l'unité de temps
    m_timeUnitCombo = new QComboBox();
    m_timeUnitCombo->addItems({"Jour", "Semaine", "Mois", "Trimestre", "Année"});
    m_timeUnitCombo->setCurrentIndex(0);
    connect(m_timeUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &HistogramView::updateHistogram);
    controlsLayout->addWidget(m_timeUnitCombo);
    
    controlsLayout->addStretch();
    
    // Ajouter les contrôles au layout principal (hérité de BaseView)
    m_mainLayout->addWidget(controlsWidget);
    
    // Créer le widget pour le graphique
    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setVisible(false);
    
    m_chartView = new InteractiveChartView(m_chart);  
    m_chartView->setRenderHint(QPainter::Antialiasing);
    
    // Message placeholder initial
    m_chart->setTitle("Exécutez le backtest pour afficher l'histogramme des gains/pertes");
    
    m_mainLayout->addWidget(m_chartView);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "HistogramView::setupUI() took" << elapsed << "ms";
}

void HistogramView::updateData(void* data, void* stats)
{
    Q_UNUSED(data);
    QTime start = QTime::currentTime();
    
    // Stocker les données pour les mises à jour ultérieures
    m_currentStats = stats;
    
    qDebug() << "HistogramView::updateData() appelé avec stats:" << stats;
    
    // Mettre à jour l'histogramme
    if (stats) {
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
        m_chart->removeAllSeries();
        m_chart->setTitle("Exécutez le backtest pour afficher l'histogramme des gains/pertes");
    }
    m_currentStats = nullptr;
}

void HistogramView::updateHistogram()
{
    if (!m_currentStats || !m_timeUnitCombo) {
        return;
    }
    
    qDebug() << "Mise à jour de l'histogramme avec les données réelles...";
    
    // Récupérer les données de trades depuis PyBindingManager
    PyBindingManager& pyManager = PyBindingManager::getInstance();
    QList<QVariantMap> trades = pyManager.getTradesData(m_currentStats);
    
    if (trades.isEmpty()) {
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

void HistogramView::createChart(const GroupedData& data)
{
    if (!m_chart) {
        return;
    }
    
    // Nettoyer le graphique existant
    m_chart->removeAllSeries();
    
    const auto axes = m_chart->axes();
    for (auto axis : axes) {
        m_chart->removeAxis(axis);
    }
    
    // Utiliser QStackedBarSeries pour éviter le chevauchement
    QStackedBarSeries* series = new QStackedBarSeries();
    QBarSet* gainsSet = new QBarSet("Gains");
    QBarSet* lossesSet = new QBarSet("Pertes");
    
    // Configurer les couleurs
    gainsSet->setColor(QColor(76, 175, 80));      // Vert pour les gains
    gainsSet->setBorderColor(QColor(56, 142, 60));
    
    lossesSet->setColor(QColor(244, 67, 54));     // Rouge pour les pertes
    lossesSet->setBorderColor(QColor(198, 40, 40));
    
    // Séparer les données en gains et pertes
    for (int i = 0; i < data.values.size(); ++i) {
        double value = data.values[i];
        
        if (value >= 0) {
            gainsSet->append(value);
            lossesSet->append(0);  // Valeur zéro pour les pertes
        } else {
            gainsSet->append(0);   // Valeur zéro pour les gains
            lossesSet->append(value);
        }
    }
    
    // Ajouter les sets à la série
    series->append(gainsSet);
    series->append(lossesSet);
    
    m_chart->addSeries(series);
    
    // Créer les axes
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(data.categories);
    axisX->setTitleText("Période");
    
    // Rotation des labels si nécessaire
    if (data.categories.size() > 10) {
        axisX->setLabelsAngle(-45);
    }
    
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("P&L ($)");
    
    // Configurer l'échelle Y pour inclure zéro
    double minValue = *std::min_element(data.values.begin(), data.values.end());
    double maxValue = *std::max_element(data.values.begin(), data.values.end());
    
    double range = maxValue - minValue;
    double margin = range * 0.1; // 10% de marge
    
    axisY->setRange(minValue - margin, maxValue + margin);
    
    // Ajouter une ligne de référence à zéro
    if (minValue < 0 && maxValue > 0) {
        axisY->setGridLineVisible(true);
        
        // Optionnel : ajouter une ligne horizontale à zéro plus visible
        QLineSeries* zeroLine = new QLineSeries();
        for (int i = 0; i < data.categories.size(); ++i) {
            zeroLine->append(i, 0);
        }
        zeroLine->setPen(QPen(Qt::black, 1, Qt::DashLine));
        m_chart->addSeries(zeroLine);
        zeroLine->attachAxis(axisX);
        zeroLine->attachAxis(axisY);
    }
    
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
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
    m_chartView->setTooltipData(data.categories, data.values, data.fullDates);
}

HistogramView::GroupedData HistogramView::groupDataByTimeUnit(const QList<QVariantMap>& trades, const QString& timeUnit)
{
    GroupedData result;
    
    if (trades.isEmpty()) {
        return result;
    }
    
    qDebug() << "Regroupement des données par" << timeUnit;
    
    // Map pour stocker les PnL par période
    QMap<QString, double> periodPnL;
    QMap<QString, QDateTime> periodDates; // Pour trier chronologiquement
    
    for (const QVariantMap& trade : trades) {
        // Extraire le PnL
        double pnl = trade.value("PnL").toDouble();
        
        // Extraire la date de sortie (ExitTime)
        QString exitTimeStr = trade.value("ExitTime").toString();
        QDateTime exitTime = parseDateTime(exitTimeStr);
        
        if (!exitTime.isValid()) {
            qWarning() << "Date de sortie invalide pour un trade:" << exitTimeStr;
            continue;
        }
        
        // Générer la clé de période selon l'unité de temps
        QString periodKey = generatePeriodKey(exitTime, timeUnit);
        
        if (periodKey.isEmpty()) {
            continue;
        }
        
        // Accumuler le PnL pour cette période
        periodPnL[periodKey] += pnl;
        
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