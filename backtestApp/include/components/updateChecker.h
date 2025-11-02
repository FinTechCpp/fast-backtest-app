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
#include <QUrl>
#include <QTemporaryDir>
#include <QTimer>

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();

    // Static method to get current app version
    static QString currentVersion();
    
    // Version checking methods
    void checkForUpdates();
    
    // Minimal download method
    void downloadLatestRelease(const QString& version);
    
    // Auto-install methods
    void downloadAndInstallUpdate(const QString& version);
    bool extractZipFile(const QString& zipFilePath, const QString& extractPath);
    void performAutoInstall(const QString& extractedPath);
    
    // Version comparison
    static bool isNewerVersion(const QString& currentVersion, const QString& latestVersion);
    
    // Utility methods
    static QString buildDownloadUrl(const QString& version);
    
    // Constants public
    static const QString GITHUB_PAGES_BASE_URL;

signals:
    void downloadCompleted(bool success, const QString& filePath);
    void downloadError(const QString& errorMessage);
    void updateCheckCompleted(bool updateAvailable, const QString& latestVersion, const QString& currentVersion);
    void updateCheckError(const QString& errorMessage);
    void installationProgress(const QString& message);
    void installationCompleted(bool success, const QString& message);
    void installationError(const QString& errorMessage);

private slots:
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReadyRead();
    
    // Version check slots
    void onVersionCheckFinished();
    void onVersionCheckError(QNetworkReply::NetworkError error);

private:    
    // Network components
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QFile* m_downloadFile;
    int m_progressCount;
    
    // Version checking
    QNetworkReply* m_versionCheckReply;
    QString extractVersionFromHtml(const QString& html);
    
    // Installation variables
    bool m_autoInstallMode;
    QString m_downloadedFilePath;
};