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

Carte de performance vs volatilité: Positionnement de votre stratégie par rapport à d'autres dans un graphique risque/rendement

Indicateurs d'amélioration: Des suggestions visuelles sur les aspects à améliorer dans la stratégie

Widget d'analyse global : affiche seulement les statistiques critiques (celles qui ne sont pas bonnes) cela permet de montrer a l'utilisateur les faiblaisses de la stratégie. cela permet de ne pas passer a coté d'un red flag : par exemple si tout est vert mais par exemple un sharp ratio extremement mauvais : on le montrera a l'utilisateur comme un voyant rouge sur une voiture pour dire qu'il y a un probleme.
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
    m_tradingHeatmapWidget = new TradingHeatmapWidget();
    // m_drawdownComparisonWidget = new DrawdownComparisonWidget();
    
    tradesClosureLayout->addWidget(m_tradeClosureWidget, /*stretch=*/2);
    tradesClosureLayout->addWidget(m_pnlGaugeWidget, /*stretch=*/1);
    tradesClosureLayout->addWidget(m_tradingHeatmapWidget, /*stretch=*/2);
    // tradesClosureLayout->addWidget(m_drawdownComparisonWidget, /*stretch=*/2);

    m_statsLayout->addLayout(tradesClosureLayout);

    // m_plDistributionWidget = new PLDistributionWidget();
    // m_statsLayout->addWidget(m_plDistributionWidget);


    m_monthlyPerformanceWidget = new MonthlyPerformanceWidget();
    m_statsLayout->addWidget(m_monthlyPerformanceWidget);

    m_reportWidget = new ReportWidget();
    m_statsLayout->addWidget(m_reportWidget);


    // m_riskReturnMapWidget = new RiskReturnMapWidget();
    // m_statsLayout->addWidget(m_riskReturnMapWidget);

    m_tradesTableWidget = new TradesTableWidget();
    m_statsLayout->addWidget(m_tradesTableWidget);
    
    // Connecter le signal du clic sur trade
    connect(m_tradesTableWidget, &TradesTableWidget::tradeClicked,
            this, &StatsView::tradeClicked);

    // 3. FINALISATION
    // --------------------------------------
    // Configurer le scroll area et l'ajouter au layout principal
    m_scrollStats->setWidget(m_statsContent);
    m_mainLayout->addWidget(m_scrollStats);

    // 4. STOCKAGE DES WIDGETS POUR MISES À JOUR/RESET
    // m_statsWidgets.push_back(m_equityCurveWidget);
    m_statsWidgets.push_back(m_timelineWidget);
    m_statsWidgets.push_back(m_metricsWidget);
    m_statsWidgets.push_back(m_ratioGaugesWidget);
    m_statsWidgets.push_back(m_tradeClosureWidget);
    // m_statsWidgets.push_back(m_drawdownComparisonWidget);
    m_statsWidgets.push_back(m_pnlGaugeWidget);
    // m_statsWidgets.push_back(m_plDistributionWidget);
    m_statsWidgets.push_back(m_tradingHeatmapWidget);
    // m_statsWidgets.push_back(m_riskReturnMapWidget);
    m_statsWidgets.push_back(m_tradesTableWidget);
    m_statsWidgets.push_back(m_monthlyPerformanceWidget);
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

    for (auto widget : m_statsWidgets) {
        widget->updateContent(m_currentResults->stats);
    }
    
    // Update ReportWidget with stats and strategy configuration
    if (m_reportWidget) {
        m_reportWidget->updateContent(m_currentResults->stats);
        
        // Get strategy configurations from the app if available
        if (m_app) {
            StrategyConfig Config = m_app->getStrategyConfig();
            m_reportWidget->setStrategyConfigurations(Config);
        }
    }
}

void StatsView::clear() {
    qDebug() << "StatsView::clear() appelé";

    for (auto widget : m_statsWidgets) {
        widget->clear();
    }
    
    // Clear ReportWidget
    if (m_reportWidget) {
        m_reportWidget->clear();
    }
    
    m_currentResults = nullptr;
}