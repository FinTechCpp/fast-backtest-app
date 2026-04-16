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
#include "components/backtestResults.h"
#include "common.h"
#include "components/StrategyAdapter.hpp"

// Forward declarations
class ProfileManager;
class GeneralParamsPanel;
struct GeneralParamsConfig;
class StrategyPanel;
struct StrategyConfig;
class StatsView;
class ChartView;
class HistogramView;
class ResultManager;
class BacktestRunner;
class BasePanel;
class ProfileMenuManager;
class DataMenuManager;
class UpdateMenuManager;
class BacktestResultMenuManager;
class BacktestResultManager;

class App : public QMainWindow
{
    Q_OBJECT

public:
    App();
    ~App();
    
    GeneralParamsConfig getGeneralParamsConfig() const;
    std::vector<StrategyConfig> getStrategyConfigs() const;

    void setGeneralParamsConfig(const GeneralParamsConfig& config);
    void setStrategyConfigs(const std::vector<StrategyConfig>& configs);

    // Add accessors for backtest results
    const BacktestResults& getBacktestResults() const { return *m_backtestResults.get(); }
    void setBacktestResults(std::unique_ptr<BacktestResults> results);

private slots:
    void onRunBacktest();
    void onBacktestCompleted();
    void onBacktestError(const QString& error);
    void onBacktestCanceled();
    void onAbout();

private:
    // Menus
    QMenuBar* m_menuBar;
    QMenu* m_helpMenu;
    QAction* m_aboutAction;
    
    // Profile menu manager
    ProfileMenuManager* m_profileMenuManager; 

    // Data menu manager
    DataMenuManager* m_dataMenuManager;

    // Update menu manager
    UpdateMenuManager* m_updateMenuManager;

    // Results menu manager
    BacktestResultMenuManager* m_backtestResultMenuManager;

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
    
    // Results Area
    QTabWidget* m_resultsTabWidget;
    
    // Managers and Configuration
    ProfileManager* m_configManager;
    BacktestResultManager* m_backtestResultManager;
    ResultManager* m_resultManager;
    
    // Panels - REMOVAL of ProfilePanel
    GeneralParamsPanel* m_generalParamsPanel = nullptr;
    StrategyPanel* m_strategyPanel = nullptr;
    
    // Components
    BacktestRunner* m_backtestRunner;

    // Backtest results
    std::unique_ptr<BacktestResults> m_backtestResults;
    
    // Views
    StatsView* m_statsView;
    ChartView* m_chartView;
    HistogramView* m_histogramView;
    
    // Control buttons and status
    QPushButton* m_runButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    // Profile indicator shown in the control panel
    QLabel* m_profileIndicator = nullptr;
    // Small button to reset/reload the current profile
    QPushButton* m_profileResetButton = nullptr;
    
    // Private methods
    void createControlPanel();
    void createResultsArea();
    void setupConnections();
    void updateUI();
    void showResults();
    void clearResults();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void windowResizeStarted();
    void windowResizeFinished(QSize newSize);
};
