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
    
    // Minimal download method
    void downloadLatestRelease();

signals:
    void downloadCompleted(bool success, const QString& filePath);
    void downloadError(const QString& errorMessage);

private slots:
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReadyRead();

private:    
    // Constants
    static const QUrl GITHUB_PAGES_RELEASES_URL;
    
    // Network components
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QFile* m_downloadFile;
    int m_progressCount;
};