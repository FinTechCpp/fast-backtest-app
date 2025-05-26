#ifndef HISTOGRAM_VIEW_H
#define HISTOGRAM_VIEW_H

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
#include "../binding/pybinding.h"
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

QT_CHARTS_USE_NAMESPACE

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

    // Implémentation des méthodes virtuelles de BaseView
    void updateData(void* data, void* stats) override;
    void clear() override;

protected:
    void setupUI() override;

private slots:  
    void updateHistogram();

private:
    // Widgets de l'interface
    QComboBox* m_timeUnitCombo;
    InteractiveChartView* m_chartView;  // Changement ici
    QChart* m_chart;
    
    // Données stockées pour la mise à jour
    void* m_currentStats;
    
    // Méthode utilitaire pour grouper les données par unité de temps
    struct GroupedData {
        QStringList categories;
        QList<double> values;
        QMap<QString, QDateTime> fullDates;  // AJOUT pour les dates complètes
    };
    
    GroupedData groupDataByTimeUnit(const QList<QVariantMap>& trades, const QString& timeUnit);
    void createChart(const GroupedData& data);
    
    // Nouvelles méthodes utilitaires
    QDateTime parseDateTime(const QString& dateTimeStr);
    QString generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit);
    QString formatPeriodLabel(const QDateTime& dateTime, const QString& timeUnit);
    QString getFullDateLabel(const QString& periodKey, const QString& timeUnit, const QDateTime& dateTime);
    QDateTime getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit);  // AJOUT
};

#endif // HISTOGRAM_VIEW_H