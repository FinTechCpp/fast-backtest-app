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
#include "ui/views/Stats/EquityCurveWidget.h"
#include "ui/views/Stats/TimeLineWidget.h"
#include "ui/views/Stats/MetricsContainerWidget.h"
#include "ui/views/Stats/TradeClosureWidget.h"
#include "ui/views/Stats/TradingHeatmapWidget.h"
#include "ui/views/Stats/RiskReturnMapWidget.h"
#include "ui/views/Stats/TradesTableWidget.h"
#include "ui/views/Stats/RatioGaugesContainerWidget.h"
#include "ui/views/Stats/PLDistributionWidget.h"
#include "ui/views/Stats/PnLGaugeWidget.h"
#include "ui/views/Stats/MonthlyPerformanceWidget.h"



#include <cmath>

class App;


/**
 * @brief Vue pour afficher les statistiques de backtest et les trades.
 * Organise les métriques en catégories et permet de visualiser les transactions.
 */
class StatsView : public BaseView {
    Q_OBJECT

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
    EquityCurveWidget* m_equityCurveWidget = nullptr;
    TimelineWidget* m_timelineWidget = nullptr;
    MetricsContainerWidget* m_metricsWidget = nullptr;
    TradingHeatmapWidget* m_tradingHeatmapWidget = nullptr;
    TradeClosureWidget* m_tradeClosureWidget = nullptr;
    PnLGaugeWidget* m_pnlGaugeWidget;
    PLDistributionWidget* m_plDistributionWidget = nullptr;
    RiskReturnMapWidget* m_riskReturnMapWidget = nullptr;
    RatioGaugesContainerWidget* m_ratioGaugesWidget = nullptr;
    TradesTableWidget* m_tradesTableWidget = nullptr;
    MonthlyPerformanceWidget* m_monthlyPerformanceWidget = nullptr;

    
    // --- UI: Conteneurs principaux ---
    QScrollArea* m_scrollStats;
    QWidget* m_statsContent;
    QVBoxLayout* m_statsLayout;

    // --- État ---
    App* m_app;  // Référence à l'application principale
};