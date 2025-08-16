#include "ui/views/Stats/PLDistributionWidget.h"
#include <algorithm>
#include <numeric>
#include <QGridLayout>
#include <QLineSeries>
#include <QScatterSeries>
#include <QPen>
#include <QSplineSeries>
#include <cmath>
#include <QDebug>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

PLDistributionWidget::PLDistributionWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PLDistributionWidget::setupUI()
{
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Layout supérieur avec contrôles
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    
    QLabel* displayLabel = new QLabel("Afficher:");
    m_displayModeCombo = new QComboBox();
    m_displayModeCombo->addItem("Tous les trades");
    m_displayModeCombo->addItem("Trades gagnants");
    m_displayModeCombo->addItem("Trades perdants");
    
    controlsLayout->addWidget(displayLabel);
    controlsLayout->addWidget(m_displayModeCombo);
    controlsLayout->addStretch();
    
    mainLayout->addLayout(controlsLayout);
    
    // Création du graphique - utiliser directement QChartView
    m_chartView = new QChartView(new QChart());  // QChartView prend possession du QChart
    m_chartView->chart()->setTitle("Distribution des Profits/Pertes par Trade");
    m_chartView->chart()->setAnimationOptions(QChart::SeriesAnimations);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(250);
    
    mainLayout->addWidget(m_chartView);
    
    // Résumé des statistiques
    m_summaryBox = new QGroupBox("Analyse de la distribution");
    QGridLayout* statsLayout = new QGridLayout(m_summaryBox);
    
    statsLayout->addWidget(new QLabel("Gain moyen:"), 0, 0);
    m_avgWinLabel = new QLabel("N/A");
    statsLayout->addWidget(m_avgWinLabel, 0, 1);
    
    statsLayout->addWidget(new QLabel("Perte moyenne:"), 0, 2);
    m_avgLossLabel = new QLabel("N/A");
    statsLayout->addWidget(m_avgLossLabel, 0, 3);
    
    statsLayout->addWidget(new QLabel("Ratio gain/perte:"), 1, 0);
    m_ratioLabel = new QLabel("N/A");
    statsLayout->addWidget(m_ratioLabel, 1, 1);
    
    statsLayout->addWidget(new QLabel("Médiane P&L:"), 1, 2);
    m_medianLabel = new QLabel("N/A");
    statsLayout->addWidget(m_medianLabel, 1, 3);
    
    statsLayout->addWidget(new QLabel("Caractéristique:"), 2, 0);
    m_distributionLabel = new QLabel("N/A");
    m_distributionLabel->setWordWrap(true);
    statsLayout->addWidget(m_distributionLabel, 2, 1, 1, 3);
    
    mainLayout->addWidget(m_summaryBox);
    
    // Connecter le changement de mode d'affichage
    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            [this](int index) {
                m_currentMode = static_cast<DisplayMode>(index);
                updateChart();
            });
}

void PLDistributionWidget::computeStatistics(const std::vector<be::TradeData>& trades)
{
    m_plValues.clear();
    m_positiveValues.clear();
    m_negativeValues.clear();
    
    // Extraire les valeurs de PL
    for (const auto& trade : trades) {
        double plPct = trade.pl;
        m_plValues.push_back(plPct);
        
        if (plPct >= 0) {
            m_positiveValues.push_back(plPct);
        } else if (plPct < 0) {
            m_negativeValues.push_back(plPct);
        }
    }
    
    m_totalTrades = m_plValues.size();
    
    // Calculer les statistiques
    if (!m_positiveValues.empty()) {
        m_avgWin = std::accumulate(m_positiveValues.begin(), m_positiveValues.end(), 0.0) / m_positiveValues.size();
        m_maxWin = *std::max_element(m_positiveValues.begin(), m_positiveValues.end());
    } else {
        m_avgWin = 0.0;
        m_maxWin = 0.0;
    }
    
    if (!m_negativeValues.empty()) {
        m_avgLoss = std::accumulate(m_negativeValues.begin(), m_negativeValues.end(), 0.0) / m_negativeValues.size();
        m_maxLoss = *std::min_element(m_negativeValues.begin(), m_negativeValues.end());
    } else {
        m_avgLoss = 0.0;
        m_maxLoss = 0.0;
    }
    
    // Calculer la médiane
    if (!m_plValues.empty()) {
        std::vector<double> sortedValues = m_plValues;
        std::sort(sortedValues.begin(), sortedValues.end());
        
        if (sortedValues.size() % 2 == 0) {
            m_median = (sortedValues[sortedValues.size() / 2 - 1] + 
                       sortedValues[sortedValues.size() / 2]) / 2.0;
        } else {
            m_median = sortedValues[sortedValues.size() / 2];
        }
    } else {
        m_median = 0.0;
    }
}

void PLDistributionWidget::updateChart()
{
    // Créer un tout nouveau graphique
    QChart* newChart = new QChart();
    newChart->setTitle("Distribution des Profits/Pertes par Trade");
    newChart->setAnimationOptions(QChart::SeriesAnimations);
    
    // Sélectionner les données selon le mode d'affichage
    std::vector<double> dataToShow;
    QString chartTitle;
    
    switch (m_currentMode) {
        case DisplayMode::AllTrades:
            dataToShow = m_plValues;
            chartTitle = "Distribution des Profits/Pertes par Trade";
            break;
        case DisplayMode::WinningTrades:
            dataToShow = m_positiveValues;
            chartTitle = "Distribution des Trades Gagnants";
            break;
        case DisplayMode::LosingTrades:
            dataToShow = m_negativeValues;
            chartTitle = "Distribution des Trades Perdants";
            break;
    }
    
    if (dataToShow.empty()) {
        newChart->setTitle(chartTitle + " (pas de données)");
        m_chartView->setChart(newChart);
        return;
    }
    
    newChart->setTitle(chartTitle);
    
    // Calculer min/max pour les bins
    double minValue = *std::min_element(dataToShow.begin(), dataToShow.end());
    double maxValue = *std::max_element(dataToShow.begin(), dataToShow.end());
    
    // Ajouter une marge
    double margin = (maxValue - minValue) * 0.1;
    minValue -= margin;
    maxValue += margin;
    
    // Créer les bins pour l'histogramme
    int numBins = m_numBins;
    double binWidth = (maxValue - minValue) / numBins;
    
    // Préparer deux séries distinctes pour les valeurs positives et négatives
    QBarSet* positiveBarSet = new QBarSet("Gains");
    QBarSet* negativeBarSet = new QBarSet("Pertes");
    
    // Définir les couleurs
    positiveBarSet->setColor(QColor(70, 200, 70));  // Vert
    negativeBarSet->setColor(QColor(200, 70, 70));  // Rouge
    
    // Compter les occurrences dans chaque bin pour valeurs positives et négatives
    std::vector<int> positiveBinCounts(numBins, 0);
    std::vector<int> negativeBinCounts(numBins, 0);
    
    for (double value : dataToShow) {
        int binIndex = std::min(static_cast<int>((value - minValue) / binWidth), numBins - 1);
        binIndex = std::max(0, binIndex); // Pour gérer les valeurs en dehors de la plage
        
        if (value >= 0) {
            positiveBinCounts[binIndex]++;
        } else {
            negativeBinCounts[binIndex]++;
        }
    }
    
    // Créer une liste de catégories (étiquettes de l'axe X)
    QStringList categories;
    for (int i = 0; i < numBins; ++i) {
        double binStart = minValue + i * binWidth;
        if (i % 3 == 0) {  // Espacer les étiquettes pour lisibilité
            categories << QString::number(binStart, 'f', 1);
        } else {
            categories << "";
        }
    }
    
    // Ajouter les données aux barsets
    for (int i = 0; i < numBins; ++i) {
        *positiveBarSet << positiveBinCounts[i];
        *negativeBarSet << negativeBinCounts[i];
    }
    
    // Utiliser une seule série mais avec des couleurs par barre
    QBarSeries* series = new QBarSeries();
    
    // Compter les occurrences dans chaque bin
    std::vector<int> binCounts(numBins, 0);
    std::vector<double> binValues(numBins, 0.0); // Pour stocker la valeur centrale de chaque bin
    
    for (int i = 0; i < numBins; ++i) {
        double binStart = minValue + i * binWidth;
        double binCenter = binStart + binWidth / 2;
        binValues[i] = binCenter;
        
        if (i % 3 == 0) {
            categories << QString::number(binStart, 'f', 1);
        } else {
            categories << "";
        }
    }
    
    // Compter les occurrences
    for (double value : dataToShow) {
        int binIndex = std::min(static_cast<int>((value - minValue) / binWidth), numBins - 1);
        binIndex = std::max(0, binIndex);
        binCounts[binIndex]++;
    }
    
    // Créer une barre pour chaque bin avec couleur selon sa valeur
    for (int i = 0; i < numBins; ++i) {
        QBarSet* barSet = new QBarSet(QString::number(binValues[i], 'f', 1));
        *barSet << binCounts[i];
        
        // Couleur selon valeur positive/négative
        if (binValues[i] < 0) {
            barSet->setColor(QColor(200, 70, 70));  // Rouge
        } else {
            barSet->setColor(QColor(70, 200, 70));  // Vert
        }
        
        series->append(barSet);
    }
    
    // Ajouter la série au graphique
    newChart->addSeries(series);
    
    // Créer un axe des catégories
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("Profit/Perte");
    newChart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    // Créer l'axe des valeurs
    QValueAxis* axisY = new QValueAxis();
    int maxCount = *std::max_element(binCounts.begin(), binCounts.end());
    axisY->setRange(0, maxCount * 1.1);
    axisY->setTitleText("Fréquence");
    newChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    // Configurer la légende pour ne pas montrer tous les barsets
    newChart->legend()->setVisible(false);
    
    // Installer le nouveau graphique
    m_chartView->setChart(newChart);
}
void PLDistributionWidget::updateSummary()
{
    // Mettre à jour les labels de statistiques
    m_avgWinLabel->setText(QString("%1%").arg(m_avgWin, 0, 'f', 2));
    m_avgLossLabel->setText(QString("%1%").arg(m_avgLoss, 0, 'f', 2));
    
    double gainLossRatio = m_avgLoss != 0 ? std::abs(m_avgWin / m_avgLoss) : 0;
    m_ratioLabel->setText(QString("%1").arg(gainLossRatio, 0, 'f', 2));
    m_medianLabel->setText(QString("%1%").arg(m_median, 0, 'f', 2));
    
    // Analyse de la distribution
    QString distributionAnalysis;
    
    if (m_plValues.empty()) {
        distributionAnalysis = "Pas assez de données pour l'analyse.";
    } else {
        // Calculer le skewness pour voir si la distribution est asymétrique
        double mean = std::accumulate(m_plValues.begin(), m_plValues.end(), 0.0) / m_plValues.size();
        double sumCubed = 0.0;
        double sumSquared = 0.0;
        
        for (double val : m_plValues) {
            double diff = val - mean;
            sumCubed += diff * diff * diff;
            sumSquared += diff * diff;
        }
        
        double variance = sumSquared / m_plValues.size();
        double skewness = sumCubed / (m_plValues.size() * std::pow(std::sqrt(variance), 3));
        
        if (skewness > 0.5) {
            distributionAnalysis = "Distribution asymétrique positive: quelques grands gagnants.";
        } else if (skewness < -0.5) {
            distributionAnalysis = "Distribution asymétrique négative: quelques grands perdants.";
        } else {
            distributionAnalysis = "Distribution relativement symétrique.";
        }
        
        // Ajouter l'analyse du ratio gain/perte
        if (gainLossRatio > 2.0) {
            distributionAnalysis += " Excellent ratio gain/perte.";
        } else if (gainLossRatio > 1.0) {
            distributionAnalysis += " Bon ratio gain/perte.";
        } else {
            distributionAnalysis += " Ratio gain/perte à améliorer.";
        }
    }
    
    m_distributionLabel->setText(distributionAnalysis);
}

void PLDistributionWidget::updateData(const be::Stats& stats)
{
    if (stats.trades.empty()) {
        clear();
        return;
    }
    
    computeStatistics(stats.trades);
    updateChart();
    updateSummary();
}

void PLDistributionWidget::clear()
{
    m_plValues.clear();
    m_positiveValues.clear();
    m_negativeValues.clear();
    m_totalTrades = 0;
    
    m_chartView->chart()->removeAllSeries();
    m_chartView->chart()->setTitle("Distribution des Profits/Pertes par Trade (pas de données)");
    
    m_avgWinLabel->setText("N/A");
    m_avgLossLabel->setText("N/A");
    m_ratioLabel->setText("N/A");
    m_medianLabel->setText("N/A");
    m_distributionLabel->setText("N/A");
}