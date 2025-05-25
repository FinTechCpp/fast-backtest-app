#ifndef APP_H
#define APP_H

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

// Forward declarations
class ConfigManager;
class GeneralParamsPanel;
class StrategyBasePanel;
class ProfilePanel;
class StatsView;
class ChartView;
class HistogramView;
class ResultManager;
class BacktestRunner;
class BasePanel;

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
    ProfilePanel* getProfilePanel() const { return m_profilePanel; }
    BasePanel* getStrategySpecificPanel() const { return m_strategySpecificPanel; }
    
    // Getters pour les configurations
    QMap<QString, QVariant> getStrategyConfig() const;
    QMap<QString, QVariant> getIndicatorConfig() const;
    
    // Mise à jour des vues de résultats
    void updateResultViews(void* data, void* stats);

private slots:
    void onStrategyChanged(const QString& strategy);
    void onRunBacktest();
    void onBacktestCompleted();
    void onBacktestError(const QString& error);
    void updateStrategySpecificPanel();

private:
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
    
    // Panels - AJOUT MANQUANT
    GeneralParamsPanel* m_generalParamsPanel;
    StrategyBasePanel* m_strategyBasePanel;
    ProfilePanel* m_profilePanel;
    BasePanel* m_strategySpecificPanel;  // Panel spécifique à la stratégie actuelle
    
    // Components - AJOUT MANQUANT
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
};

#endif // APP_H