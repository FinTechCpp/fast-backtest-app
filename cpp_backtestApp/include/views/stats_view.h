#pragma once

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
#include "views/baseview.h"
#include "metric_widget.h"
#include <cmath>

class App;  

/**
 * @brief Modèle de données pour la table des trades
 */
class TradesTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    TradesTableModel(QObject* parent = nullptr);
    
    // Modification: la méthode accepte maintenant un vecteur de pointeurs de trades C++
    void updateData(const std::vector<std::shared_ptr<be::Trade>>& trades);
    void clear();
    
    static QString formatNumber(double value, int precision = 2);
    static QDateTime dateToQDateTime(const be::Date& date);
    
private:
    static QString formatDuration(const QString& duration);
    static QString formatDateTime(const QDateTime& dateTime);
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

    // Modification: signature mise à jour pour utiliser BacktestResults*
    void updateData(BacktestResults* results) override;
    void clear() override;

protected:
    void setupUI() override;

private slots:
    void refreshTradesTable();

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

    QGroupBox* m_timeGroup;
    QGridLayout* m_timeLayout;
    
    // Section trades
    QGroupBox* m_tradesGroup;
    QVBoxLayout* m_tradesLayout;
    QTableView* m_tradesTable;
    QComboBox* m_tradesLimitCombo;
    QPushButton* m_showAllTradesBtn;
    
    // Section equity
    QGroupBox* m_equityGroup;
    QVBoxLayout* m_equityLayout;
    QPushButton* m_showEquityBtn;
    QComboBox* m_equityLimitCombo;
    QStackedWidget* m_equityStack;

    // Map des widgets de métriques
    QMap<QString, MetricWidget*> m_metricWidgets;

    App* m_app;  // Référence à l'application principale
    
    // État
    bool m_tablesCreated;
    BacktestResults* m_currentResults; // Modification: changement de type
    
    // Méthodes privées
    void createTimeSection(QGridLayout* layout);
    void createStatsWidgets();
    void createPerformanceSection(QGridLayout* layout);
    void createRiskSection(QGridLayout* layout);
    void createGeneralSection(QGridLayout* layout);
    void createTradesTable();
    
    // Mise à jour de la signature de createMetricWidget pour retourner le pointeur
    MetricWidget* createMetricWidget(const QString& key, const QString& label, 
                                     const QString& value, int row, int col, 
                                     QGridLayout* layout);
                                     
    // Nouvelles signatures pour les méthodes d'extraction des données C++
    void populateMetrics(const be::Stats& stats);
    void populateTrades(const std::vector<std::shared_ptr<be::Trade>>& trades);
    void populateEquity(const be::Stats& stats);
    
    // Méthodes utilitaires
    QString formatCurrency(double value);
    QString formatPercentage(double value);
    QString formatDuration(const QString& value);
    QString formatDetailedDuration(int days, int hours, int minutes, int seconds);
    void updateMetricWidget(const QString& key, const QString& label, const QString& value);
    
    // Helper pour gérer les valeurs qui pourraient être NaN ou infinies
    bool isValidNumber(double value) const {
        return !std::isnan(value) && !std::isinf(value);
    }
};

