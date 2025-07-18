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
    
    // Public methods
    void checkForUpdates();
    void downloadAndInstallUpdate(const QString& downloadUrl);
    void abortDownload();

signals:
    void updateAvailable(const QString& version, const QString& downloadUrl);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);
    void downloadProgress(int percentage);
    void updateCompleted();
    void updateFailed(const QString& error);

private slots:
    void onUpdateCheckFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onTimeoutOccurred();

private:
    // Private methods
    bool isNewerVersion(const QString& latestVersion, const QString& currentVersion);
    QString extractVersionFromHtml(const QString& html);
    QString extractDownloadUrlFromHtml(const QString& html);
    bool verifyChecksum(const QString& filePath, const QString& expectedChecksum);
    bool installUpdate(const QString& zipPath);
    void preserveUserFiles(const QString& oldPath, const QString& newPath);
    void restoreUserFiles(const QString& oldPath, const QString& newPath);
    bool copyDirectoryRecursively(const QString& sourceDir, const QString& targetDir);

    // Private members
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QString m_latestVersion;
    QString m_downloadUrl;
    QString m_downloadPath;
    QTemporaryDir* m_tempDir;
    QTimer* m_timeoutTimer;
    
    // Constants
    static const QUrl GITHUB_PAGES_RELEASES_URL;
    static const QStringList USER_PRESERVE_DIRS;
    static const QStringList USER_PRESERVE_FILES;
};