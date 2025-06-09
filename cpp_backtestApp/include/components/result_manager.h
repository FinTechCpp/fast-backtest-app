#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include <QResizeEvent>
#include "views/baseview.h"  

// Forward declarations
class StatsView;
class ChartView;
class HistogramView;
class QTimer;
struct BacktestResults;  

/**
 * @brief Gestionnaire principal des résultats - GÈRE les vues, n'EST PAS une vue
 */
class ResultManager : public QWidget  
{
    Q_OBJECT

public:
    explicit ResultManager(QWidget* parent = nullptr);
    
    // Méthodes de gestion (pas d'interface BaseView)
    // Signature mise à jour pour utiliser BacktestResults*
    void updateAllViews(BacktestResults* results);
    void clearAllViews();
    void setCurrentTab(int index);
    QTabWidget* getTabWidget() const { return m_tabWidget; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTabResized();
    void onTabChanged(int index);

private:
    // Layout principal
    QVBoxLayout* m_mainLayout;
    
    // Widget principal des onglets
    QTabWidget* m_tabWidget;
    
    // Les vues (qui sont des widgets)
    StatsView* m_statsView;
    ChartView* m_chartView;
    HistogramView* m_histogramView;
    
    // Map pour accès facile aux vues
    QMap<QString, BaseView*> m_views;
    
    // Timer pour les redimensionnements
    QTimer* m_resizeTimer;
    
    // Méthodes privées d'initialisation
    void setupUI();
    void setupViews();
    void setupConnections();
};

