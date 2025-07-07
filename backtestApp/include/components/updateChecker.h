#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>
#include <QSemaphore>
#include <QJsonObject>
#include <QProgressDialog>
#include <QDir>

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();

    // Structure for application paths
    struct AppPaths {
        QString applicationDir;     // Application executable directory
        QString userDataDir;       // User data directory
        QString configDir;         // Configuration directory
        QString tempUpdateDir;     // Temporary directory for updates
        QString backupDir;         // Backup directory
    };

    // Extended update configuration
    struct UpdateConfig {
        QString currentVersion;
        QString newVersion;
        QString downloadUrl;
        QString checksum;          // Integrity checksum for the update
        qint64 downloadSize;
        
        UpdateConfig() : downloadSize(0) {}
    };

    // Existing methods
    void checkForUpdates();
    void downloadAndInstallUpdate();
    static QString currentVersion();
    bool waitForUpdateCheck(int timeout = 30000);

    // New methods for advanced update
    void downloadAndInstallUpdateAdvanced(const UpdateConfig& config);
    bool rollbackUpdate();
    const AppPaths& getPaths() const { return m_paths; }

signals:
    // Existing signals
    void updateAvailable(const QString& version, const QString& downloadUrl);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);
    void downloadProgress(int percentage);
    void updateCompleted();
    void updateFailed(const QString& error);
    
    // New signals for advanced update
    void updateAvailableAdvanced(const UpdateConfig& config);
    void extractionProgress(const QString& message);
    void rollbackCompleted();

private slots:
    void onVersionCheckFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    // Existing methods
    void parseReleaseInfo(const QByteArray& data);
    void parseReleaseObject(const QJsonObject& releaseObj);
    bool isNewerVersion(const QString& latestVersion);
    void installUpdate(const QString& filePath);

    // New methods for advanced update management
    void initializePaths();
    void createDirectoryStructure();

    // Backup and restore
    bool backupUserData();
    bool restoreUserData();
    bool mergeUserData(const QString& newAppDir);

    // Advanced installation
    bool extractUpdate(const QString& zipPath, const QString& extractDir);
    bool validateUpdate(const QString& updateDir, const QString& expectedChecksum);
    bool installUpdateAdvanced(const QString& updateDir);
    QString createUpdateScript(const QString& newAppDir);

    // Utilities
    QString calculateChecksum(const QString& filePath);
    bool copyDirectoryRecursively(const QString& source, const QString& destination, bool overwrite = true);
    QStringList getUserDataFiles();

    // Existing variables
    QNetworkAccessManager* m_networkManager;
    QString m_latestVersion;
    QString m_downloadUrl;
    QSemaphore m_waitSemaphore;
    bool m_updateAvailable;
    bool m_checkCompleted;
    QString m_errorMessage;
    
    // New variables
    AppPaths m_paths;
    UpdateConfig m_currentUpdate;
    QNetworkReply* m_downloadReply;
    QProgressDialog* m_progressDialog;
    QString m_tempUpdatePath;
    bool m_advancedModeEnabled;
};