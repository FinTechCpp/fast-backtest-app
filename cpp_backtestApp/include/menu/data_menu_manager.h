#pragma once

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QString>

// Forward declarations
class DataLoader;
class App;

/**
 * @brief Gestionnaire de menu des données dans la barre de menu
 * 
 * Cette classe gère le menu "Données" dans la barre de menu principale
 * avec les actions pour importer et gérer les données OHLC.
 */
class DataMenuManager : public QObject
{
    Q_OBJECT

public:
    explicit DataMenuManager(QObject* parent = nullptr);
    ~DataMenuManager();
    
    void createDataMenu(QMenuBar* menuBar);

private slots:
    void onImportCSV();
    void onImportFromAPI();
    void onValidateData();
    void onCleanData();
    void onShowDataInfo();
    void onSetCustomDirectory(); 

private:
    // Menu principal
    QMenu* m_dataMenu;
    
    // Actions
    QAction* m_importCSVAction;
    QAction* m_importAPIAction;
    QAction* m_validateDataAction;
    QAction* m_cleanDataAction;
    QAction* m_dataInfoAction;
    QAction* m_setDirectoryAction;
    
    void createActions();
};