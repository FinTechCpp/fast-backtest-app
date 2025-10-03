#include "ui/views/Stats/EquityCurveWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QDebug>
#include <QChart>

EquityCurveWidget::EquityCurveWidget(QWidget* parent)
    : StatsBaseWidget(parent), m_chartView(new QChartView())
{
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(250);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    
    QLabel* titleLabel = new QLabel("Évolution du capital");
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    
    layout->addWidget(titleLabel);
    layout->addWidget(m_chartView);
    
    // Initialiser avec un graphique vide mais valide
    QChart* emptyChart = new QChart();
    emptyChart->setTitle("En attente des données...");
    m_chartView->setChart(emptyChart);
}

void EquityCurveWidget::updateContent(const be::Stats& stats) {
    if (stats.equityCurve.empty()) {
        qDebug() << "Equity curve vide, impossible d'afficher le graphique";
        return;
    }
    
    // Créer une nouvelle série pour la courbe d'équité
    QLineSeries *equitySeries = new QLineSeries();
    equitySeries->setName("Capital");
    
    // Points clés à marquer
    double initialEquity = stats.equityInitial;
    double finalEquity = stats.equityFinal;
    
    // Ajouter les points de la courbe d'équité
    auto pointCount = stats.equityCurve.size();
    for (size_t i = 0; i < pointCount; ++i) {
        // equitySeries->append(static_cast<qreal>(i), static_cast<qreal>(stats.equityCurve[i]));
    }
    
    // equitySeries->append((qreal)0, (qreal)initialEquity);
    // equitySeries->append((qreal)1, (qreal)finalEquity);

    // Créer un nouveau graphique (pour éviter les problèmes avec l'ancien)
    QChart *chart = new QChart();
    
    // Configurer les axes
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Barres");
    axisX->setLabelFormat("%i");
    chart->addAxis(axisX, Qt::AlignBottom);
    equitySeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Capital ($)");
    axisY->setLabelFormat("%.2f");
    chart->addAxis(axisY, Qt::AlignLeft);
    equitySeries->attachAxis(axisY);
    
    // Style de la courbe principale
    QPen equityPen(QColor(41, 128, 185)); // Bleu
    equityPen.setWidth(2);
    equitySeries->setPen(equityPen);

    // Ajouter la série au graphique
    chart->addSeries(equitySeries);
    
    // Ajouter uniquement les points de départ et d'arrivée
    // QScatterSeries *initialPoint = new QScatterSeries();
    // initialPoint->setName("Initial");
    // initialPoint->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    // initialPoint->setMarkerSize(10);
    // initialPoint->setColor(QColor(52, 152, 219)); // Bleu
    // initialPoint->append(0, initialEquity);
    // chart->addSeries(initialPoint);
    // initialPoint->attachAxis(axisX);
    // initialPoint->attachAxis(axisY);
    
    // QScatterSeries *finalPoint = new QScatterSeries();
    // finalPoint->setName("Final");
    // finalPoint->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    // finalPoint->setMarkerSize(10);
    // finalPoint->setColor(QColor(46, 204, 113)); // Vert
    // finalPoint->append(pointCount - 1, finalEquity);
    // chart->addSeries(finalPoint);
    // finalPoint->attachAxis(axisX);
    // finalPoint->attachAxis(axisY);
    
    // Légende en bas avec seulement les 2 séries importantes
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    // Ajouter des informations en titre
    QString titleText = QString("Capital initial: $%1 | Capital final: $%2 | Performance: %3%")
                        .arg(QString::number(initialEquity, 'f', 2))
                        .arg(QString::number(finalEquity, 'f', 2))
                        .arg(QString::number(stats.returnPct, 'f', 2));
    chart->setTitle(titleText);
    
    // Remplacer l'ancien graphique par le nouveau
    if (m_chartView) {
        // Supprimer l'ancien graphique pour éviter les fuites mémoire
        QChart* oldChart = m_chartView->chart();
        m_chartView->setChart(chart);
        if (oldChart) {
            delete oldChart;
        }
    }
    
    qDebug() << "Equity curve mise à jour avec" << pointCount << "points";
}

void EquityCurveWidget::clear() {
    try {
        if (m_chartView) {
            // Créer un nouveau graphique vide
            QChart* emptyChart = new QChart();
            emptyChart->setTitle("En attente des données...");
            
            // Supprimer l'ancien graphique pour éviter les fuites mémoire
            QChart* oldChart = m_chartView->chart();
            m_chartView->setChart(emptyChart);
            if (oldChart) {
                delete oldChart;
            }
        }
    } catch (...) {
        qCritical() << "Exception lors du nettoyage de EquityCurveWidget";
    }
}