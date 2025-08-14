#pragma once

#include <QWidget>
#include <QChartView>
#include <QPieSeries>
#include <QPieSlice>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "stats.hpp"

// QT_CHARTS_USE_NAMESPACE

class TradeClosureWidget : public QWidget {
    Q_OBJECT

public:
    explicit TradeClosureWidget(QWidget* parent = nullptr);
    
    void updateData(const be::Stats& stats);
    void clear();
    
private:
    QChartView* m_chartView;
    QWidget* m_legendContainer;
    QVBoxLayout* m_legendLayout;
    
    void updateLegend(const be::Stats& stats);
};