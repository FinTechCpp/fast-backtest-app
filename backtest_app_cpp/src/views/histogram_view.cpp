#include "histogram_view.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>

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
    
    m_chartView = new QChartView(m_chart);
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
    
    // TODO: Implémenter la récupération des données depuis PyBindingManager
    // Pour l'instant, créer des données de test
    GroupedData testData;
    testData.categories = QStringList({"Jan", "Feb", "Mar", "Apr", "May"});
    testData.values = QList<double>({100.0, -50.0, 75.0, -25.0, 150.0});
    
    createChart(testData);
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
    
    // Créer la série de barres
    QBarSeries* series = new QBarSeries();
    QBarSet* set = new QBarSet("P&L");
    
    // Ajouter les données
    for (double value : data.values) {
        set->append(value);
    }
    
    // Colorer les barres selon les gains/pertes
    for (int i = 0; i < data.values.size(); ++i) {
        if (data.values[i] >= 0) {
            set->setColor(QColor(0, 255, 0)); // Vert pour les gains
        } else {
            set->setColor(QColor(255, 0, 0)); // Rouge pour les pertes
        }
    }
    
    series->append(set);
    m_chart->addSeries(series);
    
    // Créer les axes
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(data.categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("P&L");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    // Configurer le titre
    QString timeUnit = m_timeUnitCombo->currentText();
    m_chart->setTitle(QString("Histogramme des gains/pertes par %1").arg(timeUnit.toLower()));
}

HistogramView::GroupedData HistogramView::groupDataByTimeUnit(const QList<QVariantMap>& trades, const QString& timeUnit)
{
    GroupedData result;
    
    // TODO: Implémenter le regroupement des trades par unité de temps
    Q_UNUSED(trades);
    Q_UNUSED(timeUnit);
    
    return result;
}