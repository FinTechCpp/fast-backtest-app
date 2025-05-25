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
#include "baseview.h"
#include "../binding/pybinding.h"

QT_CHARTS_USE_NAMESPACE

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
    QChartView* m_chartView;
    QChart* m_chart;
    
    // Données stockées pour la mise à jour
    void* m_currentStats;
    
    // Méthode utilitaire pour grouper les données par unité de temps
    struct GroupedData {
        QStringList categories;
        QList<double> values;
    };
    
    GroupedData groupDataByTimeUnit(const QList<QVariantMap>& trades, const QString& timeUnit);
    void createChart(const GroupedData& data);
};

#endif // HISTOGRAM_VIEW_H