#pragma once
#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QChartView>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QtCharts>
#include <QToolTip>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include "baseview.h"

#include <QDateTime>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries> 
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>        
#include <algorithm>
#include <numeric>
#include <vector>

class App;  // Forward declaration pour éviter les dépendances circulaires

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
    void setupCrosshairElements(); // AJOUT de la méthode manquante
    
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
 * @brief Vue pour afficher l'histogramme des profits et pertes
 */
class HistogramView : public BaseView
{
    Q_OBJECT

public:
    explicit HistogramView(QWidget* parent = nullptr);
    ~HistogramView();

    // Mise à jour de la signature pour correspondre à BaseView
    void updateData(BacktestResults* results) override;
    void clear() override;

protected:
    void setupUI() override;

private slots:  
    void updateHistogram();

private:
    // Widgets de l'interface
    QComboBox* m_timeUnitCombo;
    InteractiveChartView* m_chartView;
    QChart* m_chart;
    
    // Référence à l'application (au lieu de stocker les résultats)
    App* m_app;
    
    // Structure pour représenter un trade (pour le groupement)
    struct TradeInfo {
        QDateTime exitTime;
        double pnl;
        bool isLong;
    };
    
    // Méthode utilitaire pour grouper les données par unité de temps
    struct GroupedData {
        QStringList categories;
        QList<double> values;
        QMap<QString, QDateTime> fullDates;
    };
    
    // Méthode pour extraire les trades depuis les données C++
    std::vector<TradeInfo> extractTradesFromResults(BacktestResults* results);
    
    // Mise à jour: utiliser vector<TradeInfo> au lieu de QList<QVariantMap>
    GroupedData groupDataByTimeUnit(const std::vector<TradeInfo>& trades, const QString& timeUnit);
    void createChart(const GroupedData& data);
    
    // Méthodes utilitaires
    QDateTime parseDateTime(const QString& dateTimeStr);
    QString generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit);
    QString formatPeriodLabel(const QDateTime& dateTime, const QString& timeUnit);
    QString getFullDateLabel(const QString& periodKey, const QString& timeUnit, const QDateTime& dateTime);
    QDateTime getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit);
};

