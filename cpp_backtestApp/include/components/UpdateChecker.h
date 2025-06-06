#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>
#include <QSemaphore>

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker();

    // Vérifie les mises à jour disponibles
    void checkForUpdates();
    
    // Télécharge et installe la mise à jour
    void downloadAndInstallUpdate();
    
    // Version actuelle de l'application
    static QString currentVersion();
    
    // Attend la fin de la vérification (pour usage synchrone si nécessaire)
    bool waitForUpdateCheck(int timeout = 30000);

signals:
    // Signaux émis pendant le processus de mise à jour
    void updateAvailable(const QString& version, const QString& downloadUrl);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);
    void downloadProgress(int percentage);
    void updateCompleted();
    void updateFailed(const QString& error);

private slots:
    void onVersionCheckFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    void parseReleaseInfo(const QByteArray& data);
    void parseReleaseObject(const QJsonObject& releaseObj);
    bool isNewerVersion(const QString& latestVersion);
    void installUpdate(const QString& filePath);

    QNetworkAccessManager* m_networkManager;
    QString m_latestVersion;
    QString m_downloadUrl;
    QSemaphore m_waitSemaphore;
    bool m_updateAvailable;
    bool m_checkCompleted;
    QString m_errorMessage;
};