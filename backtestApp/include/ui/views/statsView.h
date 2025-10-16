#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QHeaderView>
#include <QChartView>
#include <QVariant>
#include <memory>
#include "ui/views/baseView.h"
#include "ui/metricWidget.h"
#include "ui/views/Stats/MetricsContainerWidget.h"
#include "ui/views/Stats/TradingHeatmapWidget.h"
#include "ui/views/Stats/RiskReturnMapWidget.h"
#include "ui/views/Stats/TradesTableWidget.h"
#include "ui/views/Stats/RatioGaugesContainerWidget.h"
#include "ui/views/Stats/PLDistributionWidget.h"
#include "ui/views/Stats/MonthlyPerformanceWidget.h"

#include "ui/views/Stats/FlexiblePieWidget.h"
#include "ui/views/Stats/SimpleTextWidget.h"
#include "ui/views/Stats/KeyValueListWidget.h"
#include "ui/views/Stats/VerticalGaugeRenderWidget.h"
#include "ui/views/Stats/HistogramWidget.h"
#include "ui/views/Stats/EquityWidget.h"



#include <cmath>

class App;


/**
 * @brief Vue pour afficher les statistiques de backtest et les trades.
 * Organise les métriques en catégories et permet de visualiser les transactions.
 */
class StatsView : public BaseView {
    Q_OBJECT

signals:
    void tradeClicked(const be::TradeData& trade);

public:
    /**
     * @brief Constructeur de la vue des statistiques
     * @param parent Le parent widget
     */
    StatsView(QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur
     */
    ~StatsView();

    /**
     * @brief Met à jour l'affichage avec de nouveaux résultats de backtest
     * @param results Les résultats à afficher
     */
    void updateData(BacktestResults* results) override;
    
    /**
     * @brief Efface toutes les données et réinitialise l'affichage
     */
    void clear() override;

protected:
    /**
     * @brief Configure l'interface utilisateur
     */
    void setupUI() override;

private:
    // ==================== Membres privés ====================
    MetricsContainerWidget* m_metricsWidget = nullptr;
    TradingHeatmapWidget* m_tradingHeatmapWidget = nullptr;
    PLDistributionWidget* m_plDistributionWidget = nullptr;
    RiskReturnMapWidget* m_riskReturnMapWidget = nullptr;
    TradesTableWidget* m_tradesTableWidget = nullptr;
    MonthlyPerformanceWidget* m_monthlyPerformanceWidget = nullptr;


    EquityWidget* m_equityWidget = nullptr;
    
    FlexiblePieWidget* m_profitFactorWidget = nullptr;
    FlexiblePieWidget* m_exposureWidget = nullptr;
    FlexiblePieWidget* m_tradeDistributionWidget = nullptr;
    
    SimpleTextWidget* m_netProfitWidget = nullptr;
    SimpleTextWidget* m_grossProfitWidget = nullptr;
    SimpleTextWidget* m_grossLossWidget = nullptr;

    SimpleTextWidget* m_maxDDWidget = nullptr;
    SimpleTextWidget* m_averageDDWidget = nullptr;
    SimpleTextWidget* m_maxDDDurationWidget = nullptr;
    SimpleTextWidget* m_averageDDDurationWidget = nullptr;
    SimpleTextWidget* m_maxTradeDurationWidget = nullptr;
    SimpleTextWidget* m_averageTradeDurationWidget = nullptr;

    SimpleTextWidget* m_averageTradePerDayWidget = nullptr;
    
    KeyValueListWidget* m_keyValueListWidget = nullptr;
    KeyValueListWidget* m_timeInfoWidget = nullptr;
    KeyValueListWidget* m_returnsWidget = nullptr;
    KeyValueListWidget* m_equityDetailsWidget = nullptr;
    KeyValueListWidget* m_buyHoldWidget = nullptr;

    VerticalGaugeRenderWidget* m_pnlGaugeWidget = nullptr;

    HistogramWidget* m_histogramWidget = nullptr;

    std::vector<StatsBaseWidget*> m_statsWidgets;

    
    // --- UI: Conteneurs principaux ---
    QScrollArea* m_scrollStats;
    QWidget* m_statsContent;
    QVBoxLayout* m_statsLayout;

    // --- État ---
    App* m_app;  // Référence à l'application principale
};