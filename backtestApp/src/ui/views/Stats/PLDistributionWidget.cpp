#include "ui/views/Stats/PLDistributionWidget.h"
#include <algorithm>
#include <numeric>
#include <QGridLayout>
#include <QLineSeries>
#include <QPen>
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
    : QWidget(parent),
      m_totalTrades(0),
      m_avgWin(0.0),
      m_avgLoss(0.0),
      m_maxWin(0.0),
      m_maxLoss(0.0),
      m_median(0.0),
      m_chartView(nullptr),
      m_currentMode(DisplayMode::AllTrades),
      m_numBins(20)
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
    
    QLabel* displayLabel = new QLabel("Afficher:", this);
    m_displayModeCombo = new QComboBox(this);
    m_displayModeCombo->addItem("Tous les trades");
    m_displayModeCombo->addItem("Trades gagnants");
    m_displayModeCombo->addItem("Trades perdants");
    
    controlsLayout->addWidget(displayLabel);
    controlsLayout->addWidget(m_displayModeCombo);
    controlsLayout->addStretch();
    
    mainLayout->addLayout(controlsLayout);
    
    // Création du graphique - IMPORTANT: suivre l'ordre correct pour Qt 6
    // 1. Créer le chart SANS parent
    QChart* chart = new QChart();
    chart->setTitle("Distribution des Profits/Pertes par Trade");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    // 2. Créer le chartView avec le chart ET le parent
    m_chartView = new QChartView(chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(250);
    
    mainLayout->addWidget(m_chartView);
    
    // Résumé des statistiques
    m_summaryBox = new QGroupBox("Analyse de la distribution", this);
    QGridLayout* statsLayout = new QGridLayout(m_summaryBox);
    
    statsLayout->addWidget(new QLabel("Gain moyen:", m_summaryBox), 0, 0);
    m_avgWinLabel = new QLabel("N/A", m_summaryBox);
    statsLayout->addWidget(m_avgWinLabel, 0, 1);
    
    statsLayout->addWidget(new QLabel("Perte moyenne:", m_summaryBox), 0, 2);
    m_avgLossLabel = new QLabel("N/A", m_summaryBox);
    statsLayout->addWidget(m_avgLossLabel, 0, 3);
    
    statsLayout->addWidget(new QLabel("Ratio gain/perte:", m_summaryBox), 1, 0);
    m_ratioLabel = new QLabel("N/A", m_summaryBox);
    statsLayout->addWidget(m_ratioLabel, 1, 1);
    
    statsLayout->addWidget(new QLabel("Médiane P&L:", m_summaryBox), 1, 2);
    m_medianLabel = new QLabel("N/A", m_summaryBox);
    statsLayout->addWidget(m_medianLabel, 1, 3);
    
    statsLayout->addWidget(new QLabel("Caractéristique:", m_summaryBox), 2, 0);
    m_distributionLabel = new QLabel("N/A", m_summaryBox);
    m_distributionLabel->setWordWrap(true);
    statsLayout->addWidget(m_distributionLabel, 2, 1, 1, 3);
    
    mainLayout->addWidget(m_summaryBox);
    
    // Connecter le changement de mode d'affichage
    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PLDistributionWidget::updateChart);
}

void PLDistributionWidget::computeStatistics(const std::vector<be::TradeData>& trades)
{
    m_plValues.clear();
    m_positiveValues.clear();
    m_negativeValues.clear();
    
    // Extraire les valeurs de PL
    for (const auto& trade : trades) {
        double plValue = trade.pl; // Utiliser pl au lieu de plPercent
        m_plValues.push_back(plValue);
        
        if (plValue >= 0) {
            m_positiveValues.push_back(plValue);
        } else {
            m_negativeValues.push_back(plValue);
        }
    }
    
    m_totalTrades = static_cast<int>(m_plValues.size());
    
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
        m_chartView->setChart(newChart); // Le chart précédent sera automatiquement supprimé
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
    
    // Compter les occurrences dans chaque bin
    std::vector<int> binCounts(numBins, 0);
    std::vector<double> binValues(numBins, 0.0); // Pour stocker la valeur centrale de chaque bin
    
    // Précalculer les valeurs centrales des bins
    for (int i = 0; i < numBins; ++i) {
        double binStart = minValue + i * binWidth;
        binValues[i] = binStart + binWidth / 2; // Valeur centrale
    }
    
    // Compter les occurrences
    for (double value : dataToShow) {
        int binIndex = std::min(static_cast<int>((value - minValue) / binWidth), numBins - 1);
        binIndex = std::max(0, binIndex);
        binCounts[binIndex]++;
    }
    
    // Créer les étiquettes pour l'axe X
    QStringList categories;
    for (int i = 0; i < numBins; ++i) {
        double binStart = minValue + i * binWidth;
        if (i % 3 == 0) { // N'afficher qu'une étiquette sur trois pour la lisibilité
            categories << QString::number(binStart, 'f', 1);
        } else {
            categories << "";
        }
    }
    
    // IMPORTANT: Utiliser l'approche de HistogramView pour Qt 6
    // 1. Créer une série pour les barres positives et une pour les barres négatives
    QBarSeries* series = new QBarSeries();
    
    // 2. Créer deux ensembles de barres (un pour les valeurs positives, un pour les négatives)
    QBarSet* positiveSet = new QBarSet("Valeurs positives");
    QBarSet* negativeSet = new QBarSet("Valeurs négatives");
    
    positiveSet->setColor(QColor(70, 200, 70));  // Vert
    negativeSet->setColor(QColor(200, 70, 70));  // Rouge
    
    // 3. Remplir les ensembles
    for (int i = 0; i < numBins; ++i) {
        if (binValues[i] >= 0) {
            *positiveSet << binCounts[i];
            *negativeSet << 0;
        } else {
            *positiveSet << 0;
            *negativeSet << binCounts[i];
        }
    }
    
    // 4. Ajouter les ensembles à la série
    series->append(positiveSet);
    series->append(negativeSet);
    
    // 5. Ajouter la série au graphique
    newChart->addSeries(series);
    
    // 6. Créer et configurer les axes
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("Profit/Perte");
    
    QValueAxis* axisY = new QValueAxis();
    int maxCount = *std::max_element(binCounts.begin(), binCounts.end());
    axisY->setRange(0, maxCount * 1.1);
    axisY->setTitleText("Fréquence");
    
    // 7. Ajouter les axes au graphique AVANT d'attacher les séries
    newChart->addAxis(axisX, Qt::AlignBottom);
    newChart->addAxis(axisY, Qt::AlignLeft);
    
    // 8. Attacher les séries aux axes
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    
    // Ajouter des lignes pour la moyenne et la médiane (comme dans HistogramView)
    double mean = std::accumulate(dataToShow.begin(), dataToShow.end(), 0.0) / dataToShow.size();
    int meanBin = std::min(std::max(0, static_cast<int>((mean - minValue) / binWidth)), numBins - 1);
    
    // QLineSeries* meanLine = new QLineSeries();
    // meanLine->setName("Moyenne");
    // meanLine->append(meanBin + 0.5, 0);
    // meanLine->append(meanBin + 0.5, maxCount * 1.1);
    // meanLine->setPen(QPen(Qt::blue, 2, Qt::DashLine));
    
    // // Ajouter la ligne de moyenne au graphique
    // newChart->addSeries(meanLine);
    // meanLine->attachAxis(axisX);
    // meanLine->attachAxis(axisY);
    
    // Ajouter une ligne pour la médiane
    // int medianBin = std::min(std::max(0, static_cast<int>((m_median - minValue) / binWidth)), numBins - 1);
    
    // QLineSeries* medianLine = new QLineSeries();
    // medianLine->setName("Médiane");
    // medianLine->append(medianBin + 0.5, 0);
    // medianLine->append(medianBin + 0.5, maxCount * 1.1);
    // medianLine->setPen(QPen(Qt::darkGreen, 2, Qt::DotLine));
    
    // // Ajouter la ligne de médiane au graphique
    // newChart->addSeries(medianLine);
    // medianLine->attachAxis(axisX);
    // medianLine->attachAxis(axisY);
    
    // Configurer la légende
    newChart->legend()->setVisible(true);
    newChart->legend()->setAlignment(Qt::AlignBottom);
    
    // Remplacer l'ancien graphique par le nouveau
    m_chartView->setChart(newChart);
    
    // Mettre à jour les statistiques affichées
    updateSummary();
}

void PLDistributionWidget::updateSummary()
{
    // Mettre à jour les labels de statistiques
    m_avgWinLabel->setText(QString("%1").arg(m_avgWin, 0, 'f', 2));
    m_avgLossLabel->setText(QString("%1").arg(m_avgLoss, 0, 'f', 2));
    
    double gainLossRatio = m_avgLoss != 0 ? std::abs(m_avgWin / m_avgLoss) : 0;
    m_ratioLabel->setText(QString("%1").arg(gainLossRatio, 0, 'f', 2));
    m_medianLabel->setText(QString("%1").arg(m_median, 0, 'f', 2));
    
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
        double skewness = variance > 0 ? 
            sumCubed / (m_plValues.size() * std::pow(std::sqrt(variance), 3)) : 0;
        
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
}

void PLDistributionWidget::clear()
{
    m_plValues.clear();
    m_positiveValues.clear();
    m_negativeValues.clear();
    m_totalTrades = 0;
    
    // Créer un nouveau graphique vide
    QChart* newChart = new QChart();
    newChart->setTitle("Distribution des Profits/Pertes par Trade (pas de données)");
    m_chartView->setChart(newChart);
    
    // Réinitialiser les labels
    m_avgWinLabel->setText("N/A");
    m_avgLossLabel->setText("N/A");
    m_ratioLabel->setText("N/A");
    m_medianLabel->setText("N/A");
    m_distributionLabel->setText("N/A");
}