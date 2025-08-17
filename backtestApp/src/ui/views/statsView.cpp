#include "ui/views/statsView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSlice>
#include <QScatterSeries>
#include <QLineSeries>


/*
gantt chart pour les trades
5. Ajouts supplémentaires pour une visualisation complète
Voici quelques idées supplémentaires qui pourraient être intégrées:

Equity curve avec bandes de drawdown: Montrer l'évolution du capital avec des zones colorées indiquant les drawdowns

Analyse des jours/heures de trading: Heatmap montrant les performances par jour de semaine/heure de la journée

Timeline des trades: Visualisation chronologique des trades avec des barres colorées pour TP/SL/BE

Carte de performance vs volatilité: Positionnement de votre stratégie par rapport à d'autres dans un graphique risque/rendement

Indicateurs d'amélioration: Des suggestions visuelles sur les aspects à améliorer dans la stratégie
*/


// Implémentation de StatsView
StatsView::StatsView(QWidget* parent)
    : BaseView(parent),
      m_app(nullptr)
{
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
    
    setupUI();
}

StatsView::~StatsView()
{
    // Les modèles et widgets sont automatiquement détruits par Qt
}

void StatsView::setupUI() {

    // 1. CRÉATION DE LA STRUCTURE DE BASE
    // --------------------------------------
    // Scroll area principal
    m_scrollStats = new QScrollArea();
    m_scrollStats->setWidgetResizable(true);
    
    // Widget de contenu principal
    m_statsContent = new QWidget();
    
    // Layout principal
    m_statsLayout = new QVBoxLayout(m_statsContent);
    m_statsLayout->setSpacing(10);

    // 2. CRÉATION DES WIDGETS
    // --------------------------------------
    // m_equityCurveWidget = new EquityCurveWidget();
    // m_statsLayout->addWidget(m_equityCurveWidget);

    m_timelineWidget = new TimelineWidget();
    m_statsLayout->addWidget(m_timelineWidget);

    m_ratioGaugesWidget = new RatioGaugesContainerWidget();
    m_statsLayout->addWidget(m_ratioGaugesWidget);

    m_metricsWidget = new MetricsContainerWidget();
    m_statsLayout->addWidget(m_metricsWidget);

    QHBoxLayout* tradesClosureLayout = new QHBoxLayout();
    tradesClosureLayout->setContentsMargins(0, 0, 0, 0);
    tradesClosureLayout->setSpacing(10);

    m_tradeClosureWidget = new TradeClosureWidget();
    m_pnlGaugeWidget = new PnLGaugeWidget();

    tradesClosureLayout->addWidget(m_tradeClosureWidget, /*stretch=*/2);
    tradesClosureLayout->addWidget(m_pnlGaugeWidget, /*stretch=*/1);

    m_statsLayout->addLayout(tradesClosureLayout);

    // m_plDistributionWidget = new PLDistributionWidget();
    // m_statsLayout->addWidget(m_plDistributionWidget);

    m_tradingHeatmapWidget = new TradingHeatmapWidget();
    m_statsLayout->addWidget(m_tradingHeatmapWidget);

    // m_riskReturnMapWidget = new RiskReturnMapWidget();
    // m_statsLayout->addWidget(m_riskReturnMapWidget);

    m_tradesTableWidget = new TradesTableWidget();
    m_statsLayout->addWidget(m_tradesTableWidget);

    // 3. FINALISATION
    // --------------------------------------
    // Configurer le scroll area et l'ajouter au layout principal
    m_scrollStats->setWidget(m_statsContent);
    m_mainLayout->addWidget(m_scrollStats);
}

void StatsView::updateData(BacktestResults* results)
{    
    // Stocker les résultats pour les mises à jour ultérieures
    m_currentResults = results;
    
    if (!m_currentResults) {
        qWarning() << "Résultats nuls reçus";
        clear();
        return;
    }  

    // m_equityCurveWidget->updateData(m_currentResults->stats);
    m_timelineWidget->setData(m_currentResults->stats.start, m_currentResults->stats.end, m_currentResults->stats.duration, m_currentResults->stats.exposureTimePct);
    m_metricsWidget->updateData(m_currentResults->stats);
    m_ratioGaugesWidget->updateData(m_currentResults->stats);
    m_tradeClosureWidget->updateData(m_currentResults->stats);
    m_pnlGaugeWidget->updateData(m_currentResults->stats);
    // m_plDistributionWidget->updateData(m_currentResults->stats);
    m_tradingHeatmapWidget->updateData(m_currentResults->stats);
    // m_riskReturnMapWidget->updateData(m_currentResults->stats);
    m_tradesTableWidget->updateData(m_currentResults->stats.trades);
}

void StatsView::clear() {
    qDebug() << "StatsView::clear() appelé";
    
    // m_equityCurveWidget->clear();
    m_timelineWidget->clear();
    m_metricsWidget->clear();
    m_ratioGaugesWidget->clear();
    m_tradeClosureWidget->clear();
    m_pnlGaugeWidget->clear();
    // m_plDistributionWidget->clear();
    m_tradingHeatmapWidget->clear();
    // m_riskReturnMapWidget->clear();
    m_tradesTableWidget->clear();
    
    m_currentResults = nullptr;
}