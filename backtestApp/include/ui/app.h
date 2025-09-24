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
#include "buy_heikin_green.hpp"
#include "sell_heikin_red.hpp"
#include "components/strategiesAdapters/genericStrategyAdapter.hpp"
#include "ui/panels/strategySpecificPanels/buyHeikinGreenPanel.h"
#include "ui/panels/strategySpecificPanels/sellHeikinRedPanel.h"
#include "ui/panels/strategySpecificPanels/GenericStrategyPanel.h"
// #include "ui/menu/BacktestResultMenuManager.h"


// Forward declarations
class ProfileManager;
class GeneralParamsPanel;
struct GeneralParamsConfig;
class StrategyBasePanel;
struct StrategyBaseConfig;
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
    
    // Getters pour les configurations
    std::vector<std::unique_ptr<indicators::IndicatorBase>> readFromStrategyPanelToIndicatorInstances() const;

    GeneralParamsConfig getGeneralParamsConfig() const;
    StrategyBaseConfig getStrategyBaseConfig() const;
    BuyHeikinGreenConfig getBuyHeikinGreenConfig() const;
    SellHeikinRedConfig getSellHeikinRedConfig() const;
    GenericStrategyConfig getGenericStrategyConfig() const;

    void setGeneralParamsConfig(const GeneralParamsConfig& config);
    void setStrategyBaseConfig(const StrategyBaseConfig& config);
    void setBuyHeikinGreenConfig(const BuyHeikinGreenConfig& config);
    void setSellHeikinRedConfig(const SellHeikinRedConfig& config);
    void setGenericStrategyConfig(const GenericStrategyConfig& config);

    // Ajout d'accesseurs pour les résultats de backtest
    BacktestResults* getBacktestResults() const { return m_backtestResults.get(); }
    void setBacktestResults(std::unique_ptr<BacktestResults> results);

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
    ProfileMenuManager* m_profileMenuManager; 

    // Gestionnaire de menu des données
    DataMenuManager* m_dataMenuManager;

    //Gestionnaire de menu des mises à jour
    UpdateMenuManager* m_updateMenuManager;

    // Gestionnaire de résultats
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
    
    // Strategy map
    QMap<QString, QString> m_strategyMap;
    
    // Strategy-specific panel container
    QStackedWidget* m_strategyPanelStack;
    QMap<QString, QWidget*> m_strategyPanels;
    
    // Results Area
    QTabWidget* m_resultsTabWidget;
    
    // Managers and Configuration
    ProfileManager* m_configManager;
    BacktestResultManager* m_backtestResultManager;
    ResultManager* m_resultManager;
    
    // Panels - SUPPRESSION de ProfilePanel
    GeneralParamsPanel* m_generalParamsPanel = nullptr;
    StrategyBasePanel* m_strategyBasePanel = nullptr;
    BuyHeikinGreenPanel* m_buyHeikinGreenPanel = nullptr;
    SellHeikinRedPanel* m_sellHeikinRedPanel = nullptr;
    GenericStrategyPanel* m_genericStrategyPanel = nullptr;
    
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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void windowResizeStarted();
    void windowResizeFinished(QSize newSize);
};

