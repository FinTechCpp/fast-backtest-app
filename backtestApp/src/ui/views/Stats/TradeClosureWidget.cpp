#include "ui/views/Stats/TradeClosureWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDebug>

TradeClosureWidget::TradeClosureWidget(QWidget* parent)
    : StatsBaseWidget(parent), m_chartView(new QChartView())
{
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(350);
    m_chartView->setMinimumWidth(350);
    
    // Créer un layout horizontal pour contenir le graphique et la légende
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Ajouter le chart à gauche
    mainLayout->addWidget(m_chartView);
}

void TradeClosureWidget::updateContent(const be::Stats& stats) {
    if (!m_chartView) {
        return;
    }
    
    // Créer une nouvelle série
    QPieSeries *series = new QPieSeries();
    
    // Ajouter les données
    if (stats.numTPTrades > 0) {
        QPieSlice *slice = series->append("TP", stats.numTPTrades);
        slice->setColor(QColor(46, 204, 113)); // Vert
        slice->setLabelVisible(true);
    }
    
    if (stats.numSLTrades > 0) {
        QPieSlice *slice = series->append("SL", stats.numSLTrades);
        slice->setColor(QColor(231, 76, 60)); // Rouge
        slice->setLabelVisible(true);
    }
    
    if (stats.numBETrades > 0) {
        QPieSlice *slice = series->append("BE", stats.numBETrades);
        slice->setColor(QColor(52, 152, 219)); // Bleu
        slice->setLabelVisible(true);
    }
    
    if (stats.numManualTrades > 0) {
        QPieSlice *slice = series->append("Manuel", stats.numManualTrades);
        slice->setColor(QColor(127, 140, 141)); // Gris foncé
        slice->setLabelVisible(true);
    }
    
    if (stats.numUnknownTrades > 0) {
        QPieSlice *slice = series->append("Inconnu", stats.numUnknownTrades);
        slice->setColor(QColor(44, 62, 80)); // Presque noir
        slice->setLabelVisible(true);
    }
    
    // Ajouter des détails aux étiquettes
    int totalTrades = stats.numTrades;
    for (QPieSlice *slice : series->slices()) {
        int count = slice->value();
        double percentage = (count * 100.0) / totalTrades;
        slice->setLabel(QString("%1: %2% (%3)").arg(slice->label()).arg(QString::number(percentage, 'f', 1)).arg(count));
        slice->setLabelFont(QFont("Arial", 12));
        slice->setLabelColor(Qt::black); // Set label color to black
    }
    
    // Créer le graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    // chart->setTitle("Distribution des clôtures de trades");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Arial", 11));
    
    // Animation
    series->setLabelsVisible(true);
    series->setLabelsPosition(QPieSlice::LabelOutside);
    
    // Mise à jour du graphique
    m_chartView->setChart(chart);

    // Options avancées pour la lisibilité du graphique
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(10, 10, 10, 10));
    // chart->layout()->setContentsMargins(0, 0, 0, 0);
}

void TradeClosureWidget::updateLegend(const be::Stats& stats) {    
    // Ajouter de nouvelles entrées de légende avec des détails
    QStringList types = {"Take Profit (TP)", "Stop Loss (SL)", "Break Even (BE)", "Manuel", "Inconnu"};
    QColor colors[] = {
        QColor(46, 204, 113),  // Vert
        QColor(231, 76, 60),   // Rouge
        QColor(52, 152, 219),  // Bleu
        QColor(127, 140, 141), // Gris foncé
        QColor(44, 62, 80)     // Noir
    };
    unsigned int counts[] = {
        stats.numTPTrades,
        stats.numSLTrades,
        stats.numBETrades,
        stats.numManualTrades,
        stats.numUnknownTrades
    };
    
    int totalTrades = stats.numTrades;
    
    for (int i = 0; i < 5; i++) {
        if (counts[i] <= 0)
            continue;

        // Créer un widget contenant un indicateur de couleur et des labels
        QWidget* legendEntry = new QWidget();
        QHBoxLayout* entryLayout = new QHBoxLayout(legendEntry);
        entryLayout->setContentsMargins(0, 5, 0, 5);
        
        // Indicateur de couleur
        QLabel* colorIndicator = new QLabel();
        colorIndicator->setFixedSize(20, 20);
        colorIndicator->setStyleSheet(QString("background-color: %1; border-radius: 10px;").arg(colors[i].name()));
        entryLayout->addWidget(colorIndicator);
        
        // Description
        QLabel* description = new QLabel(types[i]);
        entryLayout->addWidget(description);
        
        // Ajouter l'entrée à la légende
        m_legendLayout->addWidget(legendEntry);
        
        // Ajouter une ligne avec les statistiques détaillées
        QWidget* statsEntry = new QWidget();
        QHBoxLayout* statsLayout = new QHBoxLayout(statsEntry);
        statsLayout->setContentsMargins(25, 0, 0, 10);
        
        double percentage = (counts[i] * 100.0) / totalTrades;
        QLabel* statsLabel = new QLabel(QString("%1 trades (%2%)").arg(counts[i]).arg(QString::number(percentage, 'f', 1)));
        statsLabel->setStyleSheet("color: #666;");
        statsLayout->addWidget(statsLabel);
        
        m_legendLayout->addWidget(statsEntry);
    }
    
    // Ajouter le total
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    m_legendLayout->addWidget(line);
    
    QWidget* totalEntry = new QWidget();
    QHBoxLayout* totalLayout = new QHBoxLayout(totalEntry);
    totalLayout->setContentsMargins(0, 10, 0, 0);
    
    QLabel* totalLabel = new QLabel("<b>Total:</b>");
    totalLayout->addWidget(totalLabel);
    
    QLabel* totalCount = new QLabel(QString("<b>%1 trades</b>").arg(totalTrades));
    totalCount->setAlignment(Qt::AlignRight);
    totalLayout->addWidget(totalCount);
    
    m_legendLayout->addWidget(totalEntry);
    m_legendLayout->addStretch();
}

void TradeClosureWidget::clear() {
    if (m_chartView && m_chartView->chart()) {
        m_chartView->chart()->removeAllSeries();
    }
    
    // Effacer les anciens widgets de légende (sauf le titre)
    while (m_legendLayout->count() > 1) {
        QLayoutItem* item = m_legendLayout->takeAt(1);
        delete item->widget();
        delete item;
    }
}