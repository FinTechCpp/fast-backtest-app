#pragma once

#include <QChartView>
#include <QPieSeries>
#include <QPieSlice>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "ui/views/Stats/StatsBaseWidget.h"

class TradeClosureWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit TradeClosureWidget(QWidget* parent = nullptr);
    
    void updateContent(const be::Stats& stats) override;
    void clear() override;
    
private:
    QChartView* m_chartView;
    QWidget* m_legendContainer;
    QVBoxLayout* m_legendLayout;
    
    void updateLegend(const be::Stats& stats);
};