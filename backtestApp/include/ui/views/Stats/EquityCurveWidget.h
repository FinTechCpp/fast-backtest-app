#pragma once

#include <QWidget>
#include <QChartView>
#include <QLineSeries>
#include <QScatterSeries>
#include <QValueAxis>
#include <QPen>
#include <QGraphicsTextItem>
#include "stats.hpp"

// QT_CHARTS_USE_NAMESPACE

class EquityCurveWidget : public QWidget {
    Q_OBJECT

public:
    explicit EquityCurveWidget(QWidget* parent = nullptr);
    
    void updateData(const be::Stats& stats);
    void clear();
    
private:
    QChartView* m_chartView;
};