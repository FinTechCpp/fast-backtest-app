#pragma once

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QColor>
#include <QEvent>
#include <QResizeEvent>
#include <QMap>

#include "ui/views/Stats/TitledWidget.h"
#include "stats.hpp"

class TradingHeatmapWidget : public TitledWidget {
    Q_OBJECT

public:
    explicit TradingHeatmapWidget(const QString& title = QString(), QWidget* parent = nullptr);

    void updateContent(const std::vector<be::TradeData>& trades);
    void clear();

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // Calculate the day of the week from a date (0 = Monday, 6 = Sunday)
    int getDayOfWeek(const be::Date& date);
    
    // Analyze trades to extract performance by hour and day
    void analyzeTradesByTimeAndDay(const std::vector<be::TradeData>& trades);
    
    // Return a color based on the value (red-white-green gradient)
    QColor getColorForValue(double value);

    // UI components
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    
    // Data for the heatmap
    QVector<QVector<double>> m_performanceData; // [hour][day] -> average performance (%)
    QVector<QVector<int>> m_tradeCountData;     // [hour][day] -> number of trades
    QVector<QVector<double>> m_squaredSumData;  // [hour][day] -> sum of squared performances
    
    // Min/max values for the color gradient
    double m_minValue;
    double m_maxValue;
    int m_minHour;   // First hour with trades
    int m_maxHour;   // Last hour with trades

    // Constants
    static constexpr int HOURS_IN_DAY = 24;
    static constexpr int DAYS_IN_WEEK = 7;
    static constexpr int CELL_SIZE = 45;
    static constexpr int CELL_SPACING = 0;
    
    // Day names for display
    QStringList m_dayNames{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    QVector<bool> m_activeDays; // Indicates which days have trades
    QVector<int> m_activeDayIndices; // Indices of active days for display

    // Variables for mouse tracking
    QPoint m_mousePos;
    bool m_mouseOver = false;
    int m_activeCell_col = -1;
    int m_activeCell_row = -1;
    QRect m_cellsArea; // Rectangle encompassing the entire cell area
};
