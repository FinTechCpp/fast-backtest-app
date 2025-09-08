#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include <QResizeEvent>
#include "ui/views/baseView.h"

// Forward declarations
class StatsView;
class ChartView;
class HistogramView;
class QTimer;
struct BacktestResults;  

/**
 * @brief Main results manager - MANAGES the views, is NOT a view itself
 */
class ResultManager : public QWidget  
{
    Q_OBJECT

public:
    explicit ResultManager(QWidget* parent = nullptr);
    
    // Management methods (not part of BaseView interface)
    void updateAllViews(BacktestResults* results);
    void clearAllViews();
    void setCurrentTab(int index);
    QTabWidget* getTabWidget() const { return m_tabWidget; }

public slots:
    void onTradeClicked(const be::TradeData& trade);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTabResized();
    void onTabChanged(int index);

private:
    // Main layout
    QVBoxLayout* m_mainLayout;
    
    // Main tab widget
    QTabWidget* m_tabWidget;
    
    // Views (which are widgets)
    StatsView* m_statsView;
    ChartView* m_chartView;
    HistogramView* m_histogramView;
    
    // Map for easy access to views
    QMap<QString, BaseView*> m_views;
    
    // Timer for resizing
    QTimer* m_resizeTimer;

    // Private initialization methods
    void setupUI();
    void setupViews();
    void setupConnections();
};

