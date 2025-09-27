#include "components/updateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QDesktopServices>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextStream>
#include <QTimer>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVersionNumber>
#include "version.h"  

// DOWNLOAD_URL example for version 1.0.0.19 : https://fintechcpp.github.io/fast-backtest-app-releases/downloads/fast-backtest-app-windows-v1.0.0.19.zip

// Constant centralized for GitHub Pages URL
const QString UpdateChecker::GITHUB_PAGES_BASE_URL = QStringLiteral("https://fintechcpp.github.io/fast-backtest-app-releases/");

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_downloadFile(nullptr)
    , m_progressCount(0)
    , m_versionCheckReply(nullptr)
{
    qDebug() << "UpdateChecker constructor called";

    // Create the network manager
    m_networkManager = new QNetworkAccessManager(this);
    qDebug() << "Network manager created";
}

UpdateChecker::~UpdateChecker()
{
    qDebug() << "UpdateChecker destructor called";

    // Clean up download resources
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    // Clean up version check resources
    if (m_versionCheckReply) {
        m_versionCheckReply->abort();
        m_versionCheckReply->deleteLater();
        m_versionCheckReply = nullptr;
    }
    
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }
}



void UpdateChecker::downloadLatestRelease(const QString& version)
{
    qDebug() << "Starting download latest release...";
    qDebug() << "Version to download:" << version;

    // Download URL construction with centralized method
    QString url = buildDownloadUrl(version);
    qDebug() << "URL:" << url;

    // Create the request
    QUrl downloadUrl(url);
    QNetworkRequest request;
    request.setUrl(downloadUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 Test App");
    request.setRawHeader("Accept", "*/*");
    
    qDebug() << "Starting download...";

    // Create a file to save the download
    m_downloadFile = new QFile("downloaded_update.zip");
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot open file for writing:" << m_downloadFile->fileName();
        emit downloadError("Cannot open file for writing");
        return;
    }

    // Reset progress counter
    m_progressCount = 0;

    // Start the download
    m_currentReply = m_networkManager->get(request);

    // Connect signals
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);
    
    connect(m_currentReply, &QNetworkReply::readyRead, 
            this, &UpdateChecker::onReadyRead);
    
    connect(m_currentReply, &QNetworkReply::errorOccurred, 
            this, &UpdateChecker::onDownloadError);
    
    connect(m_currentReply, &QNetworkReply::finished, 
            this, &UpdateChecker::onDownloadFinished);

    // Show request information (as in the example)
    qDebug() << "Request URL:" << request.url().toString();
    qDebug() << "User-Agent:" << request.header(QNetworkRequest::UserAgentHeader).toString();
    qDebug() << "Waiting for download to complete...";
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    // Same logic as in the working example
    m_progressCount++;
    if (m_progressCount % 10 == 0) { // Show only every 10 events
        if (bytesTotal > 0) {
            int percent = (bytesReceived * 100) / bytesTotal;
            qDebug() << "Progress:" << bytesReceived << "/" << bytesTotal << "(" << percent << "%)";
        } else {
            qDebug() << "Progress:" << bytesReceived << "bytes received";
        }
    }
}

void UpdateChecker::onReadyRead()
{
    if (!m_currentReply || !m_downloadFile) {
        return;
    }

    // Same logic as in the working example
    QByteArray data = m_currentReply->readAll();
    if (!data.isEmpty()) {
        qint64 written = m_downloadFile->write(data);
        qDebug() << "Wrote" << written << "bytes to file";
    }
}

void UpdateChecker::onDownloadError(QNetworkReply::NetworkError error)
{
    // Same logic as in the working example
    qWarning() << "Network error occurred:" << error;
    qWarning() << "Error string:" << (m_currentReply ? m_currentReply->errorString() : "Unknown error");
    emit downloadError(m_currentReply ? m_currentReply->errorString() : "Unknown network error");
}

void UpdateChecker::onDownloadFinished()
{
    qDebug() << "Download finished signal received";
    
    if (!m_currentReply || !m_downloadFile) {
        qWarning() << "Download finished but reply or file is null";
        return;
    }

    // Close the file
    m_downloadFile->close();

    // Check the result (same logic as in the working example)
    QNetworkReply::NetworkError error = m_currentReply->error();
    if (error == QNetworkReply::NoError) {
        QFileInfo fileInfo(m_downloadFile->fileName());
        qDebug() << "Download completed successfully!";
        qDebug() << "File saved as:" << fileInfo.absoluteFilePath();
        qDebug() << "File size:" << fileInfo.size() << "bytes";
        
        if (fileInfo.size() == 0) {
            qWarning() << "Warning: File size is 0 bytes!";
            emit downloadCompleted(false, fileInfo.absoluteFilePath());
        } else {
            emit downloadCompleted(true, fileInfo.absoluteFilePath());
        }
    } else {
        qCritical() << "Download failed with error code:" << error;
        qCritical() << "Error description:" << m_currentReply->errorString();

        // Remove empty file in case of error
        m_downloadFile->remove();
        emit downloadCompleted(false, "");
    }

    // Clean up
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    delete m_downloadFile;
    m_downloadFile = nullptr;
}

QString UpdateChecker::currentVersion()
{
    return QString(APP_VERSION);
}

// New method to check for updates
void UpdateChecker::checkForUpdates()
{
    qDebug() << "Starting update check...";
    
    if (m_versionCheckReply) {
        qWarning() << "Version check already in progress";
        return;
    }
    
    // URL of the GitHub Pages page
    qDebug() << "Checking version at URL:" << GITHUB_PAGES_BASE_URL;
    
    // Create the request
    QUrl checkUrl(GITHUB_PAGES_BASE_URL);
    QNetworkRequest request;
    request.setUrl(checkUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Fast-Backtest-App UpdateChecker");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

    // Start the request
    m_versionCheckReply = m_networkManager->get(request);

    // Connect signals
    connect(m_versionCheckReply, &QNetworkReply::finished,
            this, &UpdateChecker::onVersionCheckFinished);
    
    connect(m_versionCheckReply, &QNetworkReply::errorOccurred, 
            this, &UpdateChecker::onVersionCheckError);
    
    qDebug() << "Version check request started";
}

void UpdateChecker::onVersionCheckFinished()
{
    qDebug() << "Version check finished";
    
    if (!m_versionCheckReply) {
        qWarning() << "Version check reply is null";
        return;
    }

    // Check for errors
    QNetworkReply::NetworkError error = m_versionCheckReply->error();
    if (error != QNetworkReply::NoError) {
        qCritical() << "Version check failed with error:" << error;
        qCritical() << "Error description:" << m_versionCheckReply->errorString();
        emit updateCheckError(m_versionCheckReply->errorString());
        m_versionCheckReply->deleteLater();
        m_versionCheckReply = nullptr;
        return;
    }
    
    // Read the HTML content
    QByteArray htmlData = m_versionCheckReply->readAll();
    QString html = QString::fromUtf8(htmlData);
    
    qDebug() << "Received HTML data, size:" << htmlData.size() << "bytes";

    // Extract the version from the HTML
    QString latestVersion = extractVersionFromHtml(html);
    QString currentVersionStr = currentVersion();
    
    qDebug() << "Current version:" << currentVersionStr;
    qDebug() << "Latest version:" << latestVersion;
    
    if (latestVersion.isEmpty()) {
        qWarning() << "Could not extract version from HTML";
        emit updateCheckError("Could not extract version information from server");
    } else {
        // Compare versions
        bool updateAvailable = isNewerVersion(currentVersionStr, latestVersion);
        qDebug() << "Update available:" << updateAvailable;
        emit updateCheckCompleted(updateAvailable, latestVersion, currentVersionStr);
    }

    // Clean up
    m_versionCheckReply->deleteLater();
    m_versionCheckReply = nullptr;
}

void UpdateChecker::onVersionCheckError(QNetworkReply::NetworkError error)
{
    qWarning() << "Version check network error occurred:" << error;
    qWarning() << "Error string:" << (m_versionCheckReply ? m_versionCheckReply->errorString() : "Unknown error");
    emit updateCheckError(m_versionCheckReply ? m_versionCheckReply->errorString() : "Unknown network error");
}

QString UpdateChecker::extractVersionFromHtml(const QString& html)
{
    qDebug() << "Extracting version from HTML...";

    // Search for the pattern: <span class="version-tag">v1.0.0.19</span>
    QRegularExpression regex("<span class=\"version-tag\">v([\\d\\.]+)</span>");
    QRegularExpressionMatch match = regex.match(html);
    
    if (match.hasMatch()) {
        QString version = match.captured(1);  // Capture without the 'v'
        qDebug() << "Extracted version:" << version;
        return version;
    } else {
        qWarning() << "Version pattern not found in HTML";
        qDebug() << "HTML preview (first 500 chars):" << html.left(500);
        return QString();
    }
}

bool UpdateChecker::isNewerVersion(const QString& currentVersion, const QString& latestVersion)
{
    qDebug() << "Comparing versions: current=" << currentVersion << "latest=" << latestVersion;

    // Use QVersionNumber for comparison
    QVersionNumber currentVer = QVersionNumber::fromString(currentVersion);
    QVersionNumber latestVer = QVersionNumber::fromString(latestVersion);
    
    qDebug() << "Parsed current version:" << currentVer.toString();
    qDebug() << "Parsed latest version:" << latestVer.toString();
    
    bool isNewer = QVersionNumber::compare(latestVer, currentVer) > 0;
    qDebug() << "Is newer version available:" << isNewer;
    
    return isNewer;
}

QString UpdateChecker::buildDownloadUrl(const QString& version)
{
    // Build the download URL from the version
    QString downloadUrl = GITHUB_PAGES_BASE_URL + "downloads/fast-backtest-app-windows-v" + version + ".zip";
    qDebug() << "Built download URL:" << downloadUrl;
    return downloadUrl;
}