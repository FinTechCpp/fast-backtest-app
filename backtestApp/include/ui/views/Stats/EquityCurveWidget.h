#pragma once

#include <QChartView>
#include <QLineSeries>
#include <QScatterSeries>
#include <QValueAxis>
#include <QPen>
#include <QGraphicsTextItem>
#include "ui/views/Stats/StatsBaseWidget.h"

// QT_CHARTS_USE_NAMESPACE

class EquityCurveWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit EquityCurveWidget(QWidget* parent = nullptr);

    void updateContent(const be::Stats& stats) override;
    void clear() override;

private:
    QChartView* m_chartView;
};