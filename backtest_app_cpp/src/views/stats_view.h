#ifndef STATS_VIEW_H
#define STATS_VIEW_H

// IMPORTANT: Protéger contre le conflit slots
#ifdef slots
#undef slots
#endif

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QHeaderView>
#include <QVariant>  
#include <memory>
#include "baseview.h"
#include "../metric_widget.h"
#include "../binding/pybinding.h"

// Redéfinir slots pour Qt
#ifndef QT_NO_KEYWORDS
#define slots Q_SLOTS
#endif

/**
 * @brief Modèle de données pour la table des trades
 */
class TradesTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TradesTableModel(QObject* parent = nullptr);
    void updateData(const QList<QVariantMap>& trades);
    static QString formatNumber(double value, int precision = 2);
};

/**
 * @brief Vue pour afficher les statistiques du backtest
 */
class StatsView : public BaseView  
{
    Q_OBJECT

public:
    StatsView(QWidget* parent = nullptr);  
    ~StatsView();

    void updateData(void* data, void* stats) override;
    void clear() override;

protected:
    void setupUI() override;

private slots:
    void refreshTradesTable();
    void refreshEquityTable();
    void toggleEquityTable();

private:
    // Modèles de données
    TradesTableModel* m_tradesModel;
    TradesTableModel* m_equityModel;
    
    // Widgets d'interface
    QScrollArea* m_scrollStats;
    QWidget* m_statsContent;
    QVBoxLayout* m_statsContentLayout;
    QLabel* m_statsPlaceholder;
    
    // Groupes de métriques
    QGroupBox* m_performanceGroup;
    QGridLayout* m_performanceLayout;
    QGroupBox* m_riskGroup;
    QGridLayout* m_riskLayout;
    QGroupBox* m_generalGroup;
    QGridLayout* m_generalLayout;
    
    // Section trades
    QGroupBox* m_tradesGroup;
    QVBoxLayout* m_tradesLayout;
    QTableView* m_tradesTable;
    QComboBox* m_tradesLimitCombo;
    QPushButton* m_showAllTradesBtn;
    
    // Section equity
    QGroupBox* m_equityGroup;
    QVBoxLayout* m_equityLayout;
    QTableView* m_equityTable;
    QPushButton* m_showEquityBtn;
    QComboBox* m_equityLimitCombo;
    QStackedWidget* m_equityStack;

    // Map des widgets de métriques
    QMap<QString, MetricWidget*> m_metricWidgets;
    
    // État
    bool m_tablesCreated;
    void* m_currentStats;
    
    // Méthodes privées
    void createStatsWidgets();
    void createPerformanceSection(QGridLayout* layout);
    void createRiskSection(QGridLayout* layout);
    void createGeneralSection(QGridLayout* layout);
    void createTradesTable();
    void createEquityTable();
    MetricWidget* createMetricWidget(const QString& key, const QString& label, 
                                     const QString& value, int row, int col, 
                                     QGridLayout* layout);
    void populateMetrics(void* stats);
    void populateTrades(void* stats);
    void populateEquity(void* stats);
    QString formatCurrency(double value);
    QString formatPercentage(double value);
    QString formatDuration(const QVariant& value);  // AJOUT MANQUANT
    void updateMetricWidget(const QString& key, const QString& label, const QString& value);
};

#endif // STATS_VIEW_H