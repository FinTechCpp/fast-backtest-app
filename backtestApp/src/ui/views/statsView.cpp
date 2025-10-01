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

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);
    // m_statsContent->setStyleSheet("background-color: lightgray;");

    // --------------------------------------
    // Ligne 0
    // --------------------------------------
    m_timeInfoWidget = new KeyValueListWidget("Informations temporelles");
    m_timeInfoWidget->addItem("Début ", "--", Qt::black);
    m_timeInfoWidget->addItem("Fin   ", "--", Qt::black);
    m_timeInfoWidget->setMinimumHeight(m_timeInfoWidget->sizeHint().height());
    gridLayout->addWidget(m_timeInfoWidget, 0, 0, 1, 3); // S'étend sur les trois colonnes

    // Ajouter un stretch qui s'étend sur les 3 colonnes (par exemple à la ligne 9)
    // TODO a remplacer par le reste des widgets des statistiques
    // QSpacerItem* horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    // gridLayout->addItem(horizontalSpacer, 0, 4, 8, 3);  // ligne 0, colonne 4, hauteur 8, largeur 3

    m_equityWidget = new EquityWidget("Equity Curve");
    gridLayout->addWidget(m_equityWidget, 0, 4, 3, 3);

    // --------------------------------------
    // Ligne 1
    // --------------------------------------
    m_netProfitWidget = new SimpleTextWidget("Net Profit");
    m_netProfitWidget->setStatColors(QColor(0, 150, 0));
    m_netProfitWidget->setBackgroundColor(QColor(0, 150, 0).lighter(300));
    m_netProfitWidget->setSuffix(" %");
    gridLayout->addWidget(m_netProfitWidget, 1, 0, 1, 2); // S'étend sur 2 colonnes

    m_pnlGaugeWidget = new VerticalGaugeRenderWidget("PnL Distribution");
    gridLayout->addWidget(m_pnlGaugeWidget, 1, 2, 4, 1);


    // --------------------------------------
    // Ligne 2
    // --------------------------------------
    m_tradeDistributionWidget = new FlexiblePieWidget("Trade Distribution");
    m_tradeDistributionWidget->setCenterTextColor(QColor(0, 150, 0));
    m_tradeDistributionWidget->setCenterTextSuffix(" %");
    gridLayout->addWidget(m_tradeDistributionWidget, 2, 0);
    
    m_profitFactorWidget = new FlexiblePieWidget("Profit Factor");
    m_profitFactorWidget->setCenterTextColor(QColor(0, 150, 0));
    gridLayout->addWidget(m_profitFactorWidget, 2, 1);


    // --------------------------------------
    // Ligne 3 et 4
    // --------------------------------------
    m_keyValueListWidget = new KeyValueListWidget("Trade details");
    m_keyValueListWidget->addItem("Total ", "--", QColor(0, 0, 0));
    m_keyValueListWidget->addItem("Take Profit ", "-- %", QColor(0, 150, 0));
    m_keyValueListWidget->addItem("Break Even ", "-- %", QColor(0, 0, 150));
    m_keyValueListWidget->addItem("Stop Loss ", "-- %", QColor(150, 0, 0));
    m_keyValueListWidget->addItem("Manual ", "-- %", Qt::black);
    m_keyValueListWidget->setMinimumSize(m_keyValueListWidget->sizeHint());
    gridLayout->addWidget(m_keyValueListWidget, 3, 0, 2, 1); // S'étend sur 2 lignes

    m_grossProfitWidget = new SimpleTextWidget("Gross Profit");
    m_grossProfitWidget->setStatColors(QColor(0, 150, 0));
    m_grossProfitWidget->setBackgroundColor(QColor(0, 150, 0).lighter(300));
    m_grossProfitWidget->setSuffix(" €");
    gridLayout->addWidget(m_grossProfitWidget, 3, 1);

    m_grossLossWidget = new SimpleTextWidget("Gross Loss");
    m_grossLossWidget->setStatColors(QColor(150, 0, 0));
    m_grossLossWidget->setBackgroundColor(QColor(150, 0, 0).lighter(300));
    m_grossLossWidget->setSuffix(" €");
    gridLayout->addWidget(m_grossLossWidget, 4, 1);

    // --------------------------------------
    // Ligne 5
    // --------------------------------------
    m_maxDDWidget = new SimpleTextWidget("Max Drawdown");
    m_maxDDWidget->setStatColors(QColor(150, 0, 0));
    m_maxDDWidget->setSuffix(" %");
    gridLayout->addWidget(m_maxDDWidget, 5, 0);

    m_maxDDDurationWidget = new SimpleTextWidget("Max Drawdown Duration");
    m_maxDDDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_maxDDDurationWidget, 5, 1);

    m_maxTradeDurationWidget = new SimpleTextWidget("Max Trade Duration");
    m_maxTradeDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_maxTradeDurationWidget, 5, 2);


    // --------------------------------------
    // Ligne 6
    // --------------------------------------
    m_averageDDWidget = new SimpleTextWidget("Average Drawdown");
    m_averageDDWidget->setStatColors(QColor(150, 0, 0));
    m_averageDDWidget->setSuffix(" %");
    gridLayout->addWidget(m_averageDDWidget, 6, 0);

    m_averageDDDurationWidget = new SimpleTextWidget("Average Drawdown Duration");
    m_averageDDDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_averageDDDurationWidget, 6, 1);
    
    m_averageTradeDurationWidget = new SimpleTextWidget("Average Trade Duration");
    m_averageTradeDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_averageTradeDurationWidget, 6, 2);


    // --------------------------------------
    // Ligne 7
    // --------------------------------------
    m_exposureWidget = new FlexiblePieWidget("Exposure Time");
    m_exposureWidget->setStartAngle(180);
    m_exposureWidget->setAngleSpan(180);
    m_exposureWidget->setCenterTextColor(QColor(0, 0, 0));
    m_exposureWidget->setCenterTextSuffix(" %");
    gridLayout->addWidget(m_exposureWidget, 7, 0);


    m_histogramWidget = new HistogramWidget("Returns Distribution");
    gridLayout->addWidget(m_histogramWidget, 7, 1, 2, 2);

    // --------------------------------------
    // Ligne 8
    // --------------------------------------
    m_averageTradePerDayWidget = new SimpleTextWidget("Average Trade Per Day");
    m_averageTradePerDayWidget->setStatColors(QColor(0, 150, 0));
    m_averageTradePerDayWidget->setMinimumHeight(m_exposureWidget->sizeHint().height());
    m_averageTradePerDayWidget->setBackgroundIcon(QIcon(":/icons/24hClock7.png"));
    m_averageTradePerDayWidget->setBackgroundIconSize(QSize(120, 120));
    m_averageTradePerDayWidget->setBackgroundIconAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_averageTradePerDayWidget->setBackgroundIconOpacity(0.6);
    m_averageTradePerDayWidget->setFontSize(18);
    gridLayout->addWidget(m_averageTradePerDayWidget, 8, 0);




    m_statsLayout->addLayout(gridLayout);


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
    
    m_tradingHeatmapWidget = new TradingHeatmapWidget();
    // m_drawdownComparisonWidget = new DrawdownComparisonWidget();
    
    tradesClosureLayout->addWidget(m_tradeClosureWidget, /*stretch=*/2);
    tradesClosureLayout->addWidget(m_tradingHeatmapWidget, /*stretch=*/2);
    // tradesClosureLayout->addWidget(m_drawdownComparisonWidget, /*stretch=*/2);

    m_statsLayout->addLayout(tradesClosureLayout);

    // m_plDistributionWidget = new PLDistributionWidget();
    // m_statsLayout->addWidget(m_plDistributionWidget);


    m_monthlyPerformanceWidget = new MonthlyPerformanceWidget();
    m_statsLayout->addWidget(m_monthlyPerformanceWidget);


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

    m_timeInfoWidget->updateValue("Début ", "  " + QString::fromStdString(m_currentResults->stats.start.toString()));
    m_timeInfoWidget->updateValue("Fin   ", "  " + QString::fromStdString(m_currentResults->stats.end.toString()));



    m_netProfitWidget->setStatText(SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.returnPct));
    // Peut etre integrer cette mécanique dans le widget directement
    if (m_currentResults->stats.returnPct > 0) {
        m_netProfitWidget->setStatColors(QColor(0, 150, 0));
        m_netProfitWidget->setBackgroundColor(QColor(0, 150, 0).lighter(300));
    } else {
        m_netProfitWidget->setStatColors(QColor(150, 0, 0));
        m_netProfitWidget->setBackgroundColor(QColor(150, 0, 0).lighter(300));
    }

    m_profitFactorWidget->setCenterText(QString::number(m_currentResults->stats.profitFactor, 'f', 2));
    m_profitFactorWidget->clearSegments();
    m_profitFactorWidget->addSegment(
        std::clamp((m_currentResults->stats.grossProfit / (m_currentResults->stats.grossLoss + m_currentResults->stats.grossProfit)), 0.0, 1.0),
        QColor(0, 200, 0)
    );
    m_profitFactorWidget->addSegment(
        std::clamp((m_currentResults->stats.grossLoss / (m_currentResults->stats.grossLoss + m_currentResults->stats.grossProfit)), 0.0, 1.0),
        QColor(200, 0, 0)
    );

    m_exposureWidget->setCenterText(QString::number(std::clamp(m_currentResults->stats.exposureTimePct, 0.0, 100.0), 'f', 1));
    m_exposureWidget->clearSegments();
    m_exposureWidget->addSegment(std::clamp(m_currentResults->stats.exposureTimePct / 100.0, 0.0, 1.0), QColor(128, 179, 255)); // QColor(128, 179, 255), QColor(209, 212, 230)
    m_exposureWidget->addSegment(std::clamp(1.0 - m_currentResults->stats.exposureTimePct / 100.0, 0.0, 1.0), QColor(209, 212, 230));

    unsigned int totalTrades = m_currentResults->stats.numTrades;
    if (totalTrades > 0) {
        double TP = (static_cast<double>(m_currentResults->stats.numTPTrades) / totalTrades);
        double SL = (static_cast<double>(m_currentResults->stats.numSLTrades) / totalTrades);
        double BE = (static_cast<double>(m_currentResults->stats.numBETrades) / totalTrades);
        double manual = (static_cast<double>(m_currentResults->stats.numManualTrades) / totalTrades);
        double unknown = (static_cast<double>(m_currentResults->stats.numUnknownTrades) / totalTrades);
        
        m_tradeDistributionWidget->clearSegments();
        m_tradeDistributionWidget->addSegment(TP, QColor(0, 200, 0));
        m_tradeDistributionWidget->addSegment(SL, QColor(200, 0, 0));
        m_tradeDistributionWidget->addSegment(BE, QColor(10, 100, 200));
        m_tradeDistributionWidget->addSegment(manual, QColor(127, 140, 141));
        m_tradeDistributionWidget->addSegment(unknown, QColor(44, 62, 80));

        m_tradeDistributionWidget->setCenterText(QString::number(TP * 100.0, 'f', 1));
    }


    m_grossProfitWidget->setStatText(SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.grossProfit));
    m_grossLossWidget->setStatText(SimpleTextWidget::formatWithThousandsSeparator(-m_currentResults->stats.grossLoss));

    m_keyValueListWidget->updateValue("Total ", SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.numTrades, 0));
    m_keyValueListWidget->updateValue("Take Profit ", QString::number(m_currentResults->stats.pctTPTrades, 'f', 2) + " %");
    m_keyValueListWidget->updateValue("Break Even ", QString::number(m_currentResults->stats.pctBETrades, 'f', 2) + " %");
    m_keyValueListWidget->updateValue("Stop Loss ", QString::number(m_currentResults->stats.pctSLTrades, 'f', 2) + " %");
    m_keyValueListWidget->updateValue("Manual ", QString::number(m_currentResults->stats.pctManualTrades, 'f', 2) + " %");


    m_pnlGaugeWidget->updateContent(m_currentResults->stats);

    m_maxDDWidget->setStatText(QString::number(m_currentResults->stats.maxDrawdownPct, 'f', 2));
    m_averageDDWidget->setStatText(QString::number(m_currentResults->stats.avgDrawdownPct, 'f', 2));
    m_maxDDDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.maxDrawdownDuration.toString()));
    m_averageDDDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.avgDrawdownDuration.toString()));
    m_maxTradeDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.maxTradeDuration.toString()));
    m_averageTradeDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.avgTradeDuration.toString()));


    m_averageTradePerDayWidget->setStatText(QString::number(m_currentResults->stats.tradesPerDay, 'f', 2));

    m_histogramWidget->updateData(m_currentResults);

    m_equityWidget->setPoints(m_currentResults->stats.equityCurve);
}

void StatsView::clear() {
    qDebug() << "StatsView::clear() appelé";

    for (auto widget : m_statsWidgets) {
        widget->clear();
    }
    
    m_currentResults = nullptr;

    m_profitFactorWidget->setCenterText("--");
    m_profitFactorWidget->clearSegments();
    
    m_exposureWidget->setCenterText("--");
    m_exposureWidget->clearSegments();
    
    m_tradeDistributionWidget->setCenterText("--");
    m_tradeDistributionWidget->clearSegments();

    m_grossProfitWidget->setStatText("--");
    m_grossLossWidget->setStatText("--");

    m_pnlGaugeWidget->clear();
}