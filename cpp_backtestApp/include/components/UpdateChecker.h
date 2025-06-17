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

    // Structure pour les chemins de l'application
    struct AppPaths {
        QString applicationDir;     // Dossier de l'exécutable
        QString userDataDir;       // Dossier des données utilisateur
        QString configDir;         // Dossier de configuration
        QString tempUpdateDir;     // Dossier temporaire pour les mises à jour
        QString backupDir;         // Dossier de sauvegarde
    };

    // Configuration de mise à jour étendue
    struct UpdateConfig {
        QString currentVersion;
        QString newVersion;
        QString downloadUrl;
        QString checksum;          // Vérification d'intégrité
        qint64 downloadSize;
        
        UpdateConfig() : downloadSize(0) {}
    };

    // Méthodes existantes
    void checkForUpdates();
    void downloadAndInstallUpdate();
    static QString currentVersion();
    bool waitForUpdateCheck(int timeout = 30000);

    // Nouvelles méthodes pour la mise à jour avancée
    void downloadAndInstallUpdateAdvanced(const UpdateConfig& config);
    bool rollbackUpdate();
    const AppPaths& getPaths() const { return m_paths; }

signals:
    // Signaux existants
    void updateAvailable(const QString& version, const QString& downloadUrl);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);
    void downloadProgress(int percentage);
    void updateCompleted();
    void updateFailed(const QString& error);
    
    // Nouveaux signaux pour la mise à jour avancée
    void updateAvailableAdvanced(const UpdateConfig& config);
    void extractionProgress(const QString& message);
    void rollbackCompleted();

private slots:
    void onVersionCheckFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    // Méthodes existantes
    void parseReleaseInfo(const QByteArray& data);
    void parseReleaseObject(const QJsonObject& releaseObj);
    bool isNewerVersion(const QString& latestVersion);
    void installUpdate(const QString& filePath);

    // Nouvelles méthodes pour la gestion avancée
    void initializePaths();
    void createDirectoryStructure();
    
    // Sauvegarde et restauration
    bool backupUserData();
    bool restoreUserData();
    bool mergeUserData(const QString& newAppDir);
    
    // Installation avancée
    bool extractUpdate(const QString& zipPath, const QString& extractDir);
    bool validateUpdate(const QString& updateDir, const QString& expectedChecksum);
    bool installUpdateAdvanced(const QString& updateDir);
    QString createUpdateScript(const QString& newAppDir);
    
    // Utilitaires
    QString calculateChecksum(const QString& filePath);
    bool copyDirectoryRecursively(const QString& source, const QString& destination, bool overwrite = true);
    QStringList getUserDataFiles();

    // Variables existantes
    QNetworkAccessManager* m_networkManager;
    QString m_latestVersion;
    QString m_downloadUrl;
    QSemaphore m_waitSemaphore;
    bool m_updateAvailable;
    bool m_checkCompleted;
    QString m_errorMessage;
    
    // Nouvelles variables
    AppPaths m_paths;
    UpdateConfig m_currentUpdate;
    QNetworkReply* m_downloadReply;
    QProgressDialog* m_progressDialog;
    QString m_tempUpdatePath;
    bool m_advancedModeEnabled;
};