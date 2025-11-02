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
 * @brief Data menu manager in the menu bar
 * 
 * This class manages the "Data" menu in the main menu bar
 * with actions to import and manage OHLC data.
 */
class DataMenuManager : public QObject
{
    Q_OBJECT

public:
    explicit DataMenuManager(QObject* parent = nullptr);
    ~DataMenuManager();
    
    void createDataMenu(QMenuBar* menuBar);

    static const QString SERVER_URL;
    
private slots:
    void onImportCSV();
    void onImportFromAPI();
    void onValidateData();
    void onCleanData();
    void onShowDataInfo();
    void onSetCustomDirectory();
    
    // Slots for HTTP requests
    void onMarketDataListReceived();
    void onFileDownloadFinished();

private:
    // Main menu
    QMenu* m_dataMenu;
    
    // Actions
    QAction* m_importCSVAction;
    QAction* m_importAPIAction;
    QAction* m_validateDataAction;
    QAction* m_cleanDataAction;
    QAction* m_dataInfoAction;
    QAction* m_setDirectoryAction;
    
    // Network
    QNetworkAccessManager* m_networkManager;
    
    void createActions();
    void compareAndDownloadFiles(const QJsonArray& remoteFiles);
    QString getFileLastModified(const QString& filePath);
};