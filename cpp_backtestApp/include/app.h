#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QDebug>
#include "components/backtest_results.h"


// Forward declarations
class ConfigManager;
class GeneralParamsPanel;
class StrategyBasePanel;
class StatsView;
class ChartView;
class HistogramView;
class ResultManager;
class BacktestRunner;
class BasePanel;
class ProfileMenuManager;

class App : public QMainWindow
{
    Q_OBJECT

public:
    App();
    ~App();
    
    // Getter pour ConfigManager
    ConfigManager* getConfigManager() const { return m_configManager; }
    
    GeneralParamsPanel* getGeneralParamsPanel() const { return m_generalParamsPanel; }
    StrategyBasePanel* getStrategyBasePanel() const { return m_strategyBasePanel; }
    BasePanel* getStrategySpecificPanel() const { return m_strategySpecificPanel; }
    
    // Getters pour les configurations
    QMap<QString, QVariant> getStrategyConfig() const;
    QMap<QString, QVariant> getIndicatorConfig() const;
    
    // Mise à jour des vues de résultats
    void updateResultViews(BacktestResults* results);

private slots:
    void onStrategyChanged(const QString& strategy);
    void onRunBacktest();
    void onBacktestCompleted();
    void onBacktestError(const QString& error);
    void updateStrategySpecificPanel();
    void onAbout();

private:
    // Menus
    QMenuBar* m_menuBar;
    QMenu* m_helpMenu;
    QAction* m_aboutAction;
    
    // Gestionnaire de menu des profils
    ProfileMenuManager* m_profileMenuManager;  // AJOUT

    void createMenus();
    void createActions();

    // UI Components
    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;
    QSplitter* m_splitter;
    
    // Control Panel
    QScrollArea* m_controlPanelScrollArea;
    QWidget* m_controlPanel;
    QVBoxLayout* m_controlPanelLayout;
    
    // Strategy map
    QMap<QString, QString> m_strategyMap;
    
    // Strategy-specific panel container
    QStackedWidget* m_strategyPanelStack;
    QMap<QString, QWidget*> m_strategyPanels;
    
    // Results Area
    QTabWidget* m_resultsTabWidget;
    
    // Managers and Configuration
    ConfigManager* m_configManager;
    ResultManager* m_resultManager;
    
    // Panels - SUPPRESSION de ProfilePanel
    GeneralParamsPanel* m_generalParamsPanel = nullptr;
    StrategyBasePanel* m_strategyBasePanel = nullptr;
    BasePanel* m_strategySpecificPanel = nullptr;
    
    // Components
    BacktestRunner* m_backtestRunner;
    
    // Views
    StatsView* m_statsView;
    ChartView* m_chartView;
    HistogramView* m_histogramView;
    
    // Control buttons and status
    QPushButton* m_runButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    // Private methods
    void initStrategyMap();
    void createControlPanel();
    void createResultsArea();
    void setupConnections();
    void updateUI();
    void showResults();
    void clearResults();
    QWidget* createStrategySpecificPanel(const QString& strategy);

protected:
    void resizeEvent(QResizeEvent* event) override;
};

