#pragma once

#include <QVBoxLayout>
#include <QGroupBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVector>
#include <QMap>
#include <QColor>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QGraphicsTextItem>
#include "ui/views/Stats/StatsBaseWidget.h"

class MonthlyPerformanceWidget : public StatsBaseWidget {
    Q_OBJECT
    
public:
    explicit MonthlyPerformanceWidget(QWidget* parent = nullptr);
    ~MonthlyPerformanceWidget();
    
    void updateContent(const be::Stats& stats) override;
    void clear() override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    
private:
    void analyzeTradesByMonthAndYear(const std::vector<be::TradeData>& trades);
    QColor getColorForValue(double value);
    void buildHeatmap();

    // Constants for drawing
    static constexpr int CELL_SIZE = 50;
    static constexpr int CELL_SPACING = 2;
    static constexpr int MONTHS_IN_YEAR = 12;
    
    // Month names
    const QStringList m_monthNames = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    // Data structures for performance
    QMap<int, QMap<int, double>> m_performanceData;  // [year][month] -> performance
    QMap<int, QMap<int, int>> m_tradeCountData;      // [year][month] -> number of trades
    QMap<int, QMap<int, double>> m_squaredSumData;   // [year][month] -> sum of squared performances
    
    // Min and max years to determine the display range
    int m_minYear;
    int m_maxYear;
    
    // Min and max values for the color scale
    double m_minValue;
    double m_maxValue;
    
    // Graphical interface
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_groupBox;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
};