#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QCheckBox>
#include "ui/views/Stats/TitledWidget.h"

#include "stats.hpp"

// We can calculate the gain-loss ratio and display it above the widget, why not

class VerticalGaugeRenderWidget : public TitledWidget {
    Q_OBJECT
    
public:
    explicit VerticalGaugeRenderWidget(const QString& title = QString(), QWidget* parent = nullptr);

    void updateContent(const be::Stats& stats);
    void clear();
    
protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    
private:
    // Conversion value -> Y position
    int valueToY(double value, double minValue, double maxValue, int height);
    
    // Values to display
    double m_tpAvg;     // Average of winning trades
    double m_tpAvgPrc;  // Average of winning trades in %
    double m_tpMax;     // Maximum of winning trades
    double m_tpMaxPrc;  // Maximum of winning trades in %
    double m_tpMedian;  // Median of winning trades
    double m_tpMedianPrc;  // Median of winning trades in %

    double m_slAvg;     // Average of losing trades
    double m_slAvgPrc;  // Average of losing trades in %
    double m_slMin;     // Minimum of losing trades
    double m_slMinPrc;  // Minimum of losing trades in %
    double m_slMedian;  // Median of losing trades
    double m_slMedianPrc;  // Median of losing trades in %
    
    // Colors
    QColor m_tpAvgColor;    // Dark green for the average of TP
    QColor m_tpMaxColor;    // Light green for the max of TP
    QColor m_slAvgColor;    // Dark red for the average of SL
    QColor m_slMinColor;    // Light red for the min of SL
    QColor m_medianTpColor; // Color of the TP median line
    QColor m_medianSlColor; // Color of the SL median line
    QColor m_lineColor;     // Color of the lines
    QColor m_textColor;     // Color of the text

    QCheckBox* m_showPercentageCheckbox; // Checkbox to display in percentage or absolute value
};