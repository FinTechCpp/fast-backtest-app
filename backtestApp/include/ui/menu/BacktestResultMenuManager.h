#pragma once

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMap>
#include <QString>


// Forward declarations
class BacktestResultManager;
class App;
struct BacktestResultConfig;

/**
 * @brief Results menu handler 
 *
 * This class manages the "Results" menu in the main menu bar
 * with actions to manage backtest results.
 */
class BacktestResultMenuManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Pointer to the main application
     */
    BacktestResultMenuManager(App* parent = nullptr);
    
    /**
     * @brief Creates and adds the results menu to the menu bar
     * @param menuBar Menu bar to which the results menu will be added
     */
    void createResultMenu(QMenuBar* menuBar);
    
    /**
     * @brief Initializes the manager with the BacktestResultManager
     * @param resultManager Pointer to the result manager
     */
    void setResultManager(BacktestResultManager* resultManager);
    
    /**
     * @brief Updates the result list in the menu
     */
    void updateResultList();

public slots:
    /**
     * @brief Triggered when a result is selected or loaded
     * @param resultName Name of the loaded result
     */
    void onResultLoaded(const QString& resultName);

private slots:
    void onSaveCurrentResult();
    void onImportResult();
    void onImportExternalResult();  // Slot for importing external results
    void onOpenResultsDirectory();
    void onManageResults();  // Slot for opening the result management dialog

private:
    App* m_mainWindow;
    BacktestResultManager* m_resultManager;
    
    // Menu and actions
    QMenu* m_resultMenu;

    // Main actions
    QAction* m_saveResultAction;
    QAction* m_manageResultsAction;  //  action for managing results
    QAction* m_importAction;
    QAction* m_importExternalAction;  //  action for importing external results
    QAction* m_openDirectoryAction;

    // Dynamic actions for results (kept for internal use)
    QMap<QString, QAction*> m_resultActions;
    QString m_currentResult;
    
    void createActions();
};