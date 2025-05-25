#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include <QObject>
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include <QResizeEvent>  // AJOUT
#include "baseview.h"

// Forward declarations
class StatsView;
class ChartView;
class HistogramView;

class ResultManager : public BaseView  
{
    Q_OBJECT

public:
    explicit ResultManager(QWidget* parent = nullptr);
    ~ResultManager();
    
    QWidget* create(QWidget* parentWidget = nullptr) override;
    void update(void* data, void* stats) override;
    void clear() override;
    void updateAll(void* data, void* stats);
    void setCurrentTab(int index);
    QTabWidget* getTabWidget() const;

protected:
    void resizeEvent(QResizeEvent* event) override;  // AJOUT DE LA DÉCLARATION

private slots:
    void onTabResized();  // AJOUT DE LA DÉCLARATION

private:
    // Widgets UI
    QWidget* m_resultsWidget;
    QVBoxLayout* m_resultsLayout;
    QTabWidget* m_tabWidget;
    
    // Vues
    StatsView* m_statsView;
    ChartView* m_chartView;
    HistogramView* m_histogramView;
    
    // AJOUT : Références aux widgets créés
    QWidget* m_chartWidget;     // AJOUT
    QWidget* m_statsWidget;     // AJOUT
    QWidget* m_histogramWidget; // AJOUT
    
    // Map pour accès facile aux vues
    QMap<QString, BaseView*> m_views;
};

#endif // RESULT_MANAGER_H