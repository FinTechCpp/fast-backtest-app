#pragma once

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Forward declarations
class DataLoader;
class App;

#include <QNetworkInterface>

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
    
    // Slots pour les requêtes HTTP
    void onMarketDataListReceived();
    void onFileDownloadFinished();

    // Vérifier la connectivité VPN (basique)
    bool isVpnConnected();

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
    
    // Réseau
    QNetworkAccessManager* m_networkManager;
    
    void createActions();
    void compareAndDownloadFiles(const QJsonArray& remoteFiles);
    QString getFileLastModified(const QString& filePath);
};