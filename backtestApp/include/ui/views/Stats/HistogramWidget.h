#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <vector>
#include <map>
#include "ui/views/Stats/TitledWidget.h"
#include "components/backtestResults.h"

// Classe personnalisée pour QChartView avec tooltip interactif
class InteractiveChartView : public QChartView
{
    Q_OBJECT

public:
    InteractiveChartView(QChart* chart, QWidget* parent = nullptr);
    
    void setTooltipData(const QStringList& categories, const QList<double>& values, 
                       const QMap<QString, QDateTime>& fullDates);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void showCrosshair(const QPointF& position);
    void hideCrosshair();
    void updateTooltip(const QPointF& position);
    QString createTooltipText(const QString& category, double value);
    void setupCrosshairElements();
    
    // Données pour le tooltip
    QStringList m_categories;
    QList<double> m_values;
    QMap<QString, QDateTime> m_fullDates;
    
    // Éléments visuels du crosshair
    QGraphicsLineItem* m_horizontalLine;
    QGraphicsLineItem* m_verticalLine;
    QGraphicsTextItem* m_tooltipItem;
    
    bool m_crosshairVisible;
};

/**
 * @brief Widget pour afficher l'histogramme des profits et pertes
 */
class HistogramWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit HistogramWidget(const QString& title = "Histogramme PnL", QWidget* parent = nullptr);
    ~HistogramWidget();

    // Mise à jour avec les résultats de backtest
    void updateData(BacktestResults* results);
    void clear();

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    void resizeEvent(QResizeEvent* event) override;
    
private slots:  
    void updateHistogram();

private:
    // Widgets de l'interface
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QComboBox* m_timeUnitCombo;
    InteractiveChartView* m_chartView;
    QChart* m_chart;
    
    // Données actuelles
    BacktestResults* m_currentResults;
    
    // Structure pour représenter un trade
    struct TradeInfo {
        QDateTime exitTime;
        double pnl;
    };
    
    // Structure pour stocker les données groupées
    struct GroupedData {
        QStringList categories;
        QList<double> values;
        QMap<QString, QDateTime> fullDates;
    };
    
    // Méthodes utilitaires
    std::vector<TradeInfo> extractTradesFromResults(BacktestResults* results);
    GroupedData groupDataByTimeUnit(const std::vector<TradeInfo>& trades, const QString& timeUnit);
    void createChart(const GroupedData& data);
    QDateTime getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit);
    QString generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit);
};