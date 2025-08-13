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
 * @brief Gestionnaire de menu des résultats de backtest dans la barre de menu
 * 
 * Cette classe gère le menu "Résultats" dans la barre de menu principale
 * avec les actions pour gérer les résultats de backtest.
 */
class BacktestResultMenuManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'application principale
     */
    BacktestResultMenuManager(App* parent = nullptr);
    
    /**
     * @brief Crée et ajoute le menu des résultats à la barre de menu
     * @param menuBar Barre de menu où ajouter le menu des résultats
     */
    void createResultMenu(QMenuBar* menuBar);
    
    /**
     * @brief Initialise le gestionnaire avec le BacktestResultManager
     * @param resultManager Pointeur vers le gestionnaire de résultats
     */
    void setResultManager(BacktestResultManager* resultManager);
    
    /**
     * @brief Met à jour la liste des résultats dans le menu
     */
    void updateResultList();

public slots:
    /**
     * @brief Déclenché quand un résultat est sélectionné ou chargé
     * @param resultName Nom du résultat chargé
     */
    void onResultLoaded(const QString& resultName);

private slots:
    void onSaveCurrentResult();
    void onDeleteResult();
    void onImportResult();
    void onExportResult();
    void onLoadResult();
    void onOpenResultsDirectory();
    void onViewResultDetails();

private:
    App* m_mainWindow;
    BacktestResultManager* m_resultManager;
    
    // Menu et actions
    QMenu* m_resultMenu;
    QMenu* m_loadResultSubmenu;
    
    // Actions principales
    QAction* m_saveResultAction;
    QAction* m_deleteResultAction;
    QAction* m_importAction;
    QAction* m_exportAction;
    QAction* m_openDirectoryAction;
    QAction* m_viewDetailsAction;
    
    // Actions dynamiques pour les résultats
    QMap<QString, QAction*> m_resultActions;
    QString m_currentResult;
    
    void createActions();
};