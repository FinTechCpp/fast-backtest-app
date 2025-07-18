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
#include "version.h"  

// Constants
const QUrl UpdateChecker::GITHUB_PAGES_RELEASES_URL = QUrl(QStringLiteral("https://fintechcpp.github.io/fast-backtest-app-releases/"));

const QStringList UpdateChecker::USER_PRESERVE_DIRS = {
    "marketData",
    "logs", 
    "Notebooks",
    "images"
};

const QStringList UpdateChecker::USER_PRESERVE_FILES = {
    "backtest_config.ini"
};

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_currentReply(nullptr),
      m_tempDir(nullptr)
{
    qDebug() << "UpdateChecker created";
}

UpdateChecker::~UpdateChecker()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    if (m_tempDir) {
        delete m_tempDir;
    }
    qDebug() << "UpdateChecker destroyed";
}

QString UpdateChecker::currentVersion()
{
    return QString(APP_VERSION);
}

void UpdateChecker::checkForUpdates()
{
    qInfo() << "Vérification des mises à jour...";
    qInfo() << "Version actuelle:" << currentVersion();
    
    // Cancel any ongoing request
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    // Create network request
    QNetworkRequest request(GITHUB_PAGES_RELEASES_URL);
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     QString("FastBacktestApp/%1").arg(currentVersion()));
    
    // Set reasonable timeout
    request.setTransferTimeout(10000); // 10 seconds
    
    // Make the request
    m_currentReply = m_networkManager->get(request);
    
    // Connect signals
    connect(m_currentReply, &QNetworkReply::finished,
            this, &UpdateChecker::onUpdateCheckFinished);

    qDebug() << "Request sent to:" << GITHUB_PAGES_RELEASES_URL.toString();
}

void UpdateChecker::onUpdateCheckFinished()
{
    if (!m_currentReply) {
        qWarning() << "Null reply in onUpdateCheckFinished";
        return;
    }
    
    QNetworkReply::NetworkError error = m_currentReply->error();
    
    if (error != QNetworkReply::NoError) {
        QString errorString = m_currentReply->errorString();
        qWarning() << "Erreur réseau:" << errorString;
        emit updateCheckFailed(errorString);
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    // Read the response
    QByteArray responseData = m_currentReply->readAll();
    QString html = QString::fromUtf8(responseData);

    qDebug() << "RResponse received, size:" << responseData.size() << "bytes";

    // Extract version and download URL from HTML
    QString latestVersion = extractVersionFromHtml(html);
    QString downloadUrl = extractDownloadUrlFromHtml(html);
    
    if (latestVersion.isEmpty() || downloadUrl.isEmpty()) {
        qWarning() << "Impossible to extract version or download URL";
        emit updateCheckFailed("Unable to parse server response");
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    qInfo() << "Version found:" << latestVersion;
    qInfo() << "Download URL:" << downloadUrl;

    // Store the information
    m_latestVersion = latestVersion;
    m_downloadUrl = downloadUrl;
    
    // Compare versions
    if (isNewerVersion(latestVersion, currentVersion())) {
        qInfo() << "New version available:" << latestVersion;
        emit updateAvailable(latestVersion, downloadUrl);
    } else {
        qInfo() << "No update available";
        emit noUpdateAvailable();
    }
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

QString UpdateChecker::extractVersionFromHtml(const QString& html)
{
    // Look for version pattern like "v1.0.0.11" in the HTML
    QRegularExpression versionRegex("<span class=\"version-tag\">v([\\d\\.]+)</span>");
    QRegularExpressionMatch match = versionRegex.match(html);
    
    if (match.hasMatch()) {
        QString version = match.captured(1);
        qDebug() << "Version extracted:" << version;
        return version;
    }

    qWarning() << "Version not found in HTML";
    return QString();
}

QString UpdateChecker::extractDownloadUrlFromHtml(const QString& html)
{
    // Look for download URL pattern in the HTML
    QRegularExpression urlRegex("<a href=\"(https://github\\.com/FinTechCpp/fast-backtest-app/releases/download/[^\"]+\\.zip)\" class=\"button\">");
    QRegularExpressionMatch match = urlRegex.match(html);
    
    if (match.hasMatch()) {
        QString url = match.captured(1);
        qDebug() << "Download URL extracted:" << url;
        return url;
    }

    qWarning() << "Download URL not found in HTML";
    return QString();
}

bool UpdateChecker::isNewerVersion(const QString& latestVersion, const QString& currentVersion)
{
    QVersionNumber latest = QVersionNumber::fromString(latestVersion);
    QVersionNumber current = QVersionNumber::fromString(currentVersion);

    qDebug() << "Comparing versions:";
    qDebug() << "  Current:" << current.toString();
    qDebug() << "  Latest:" << latest.toString();

    bool isNewer = QVersionNumber::compare(latest, current) > 0;
    qDebug() << "  Result: new version =" << isNewer;

    return isNewer;
}

void UpdateChecker::downloadAndInstallUpdate(const QString& downloadUrl)
{
    qInfo() << "Starting download:" << downloadUrl;

    // Cancel any ongoing request
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    // Create temporary directory
    if (m_tempDir) {
        delete m_tempDir;
    }
    m_tempDir = new QTemporaryDir();
    
    if (!m_tempDir->isValid()) {
        qCritical() << "Impossible to create temporary directory";
        emit updateFailed("Impossible to create temporary directory");
        return;
    }
    
    // Set download path
    QString fileName = QUrl(downloadUrl).fileName();
    if (fileName.isEmpty()) {
        fileName = QString("fast-backtest-app-windows-v%1.zip").arg(m_latestVersion);
    }
    m_downloadPath = m_tempDir->filePath(fileName);

    qDebug() << "Downloading to:" << m_downloadPath;

    // Create network request
    QNetworkRequest request(downloadUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     QString("FastBacktestApp/%1").arg(currentVersion()));
    
    // Make the request
    m_currentReply = m_networkManager->get(request);
    
    // Connect signals
    connect(m_currentReply, &QNetworkReply::finished,
            this, &UpdateChecker::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);

    qInfo() << "Download started...";
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) {
        return;
    }
    
    int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
    emit downloadProgress(percentage);
    
    if (percentage % 10 == 0) { // Log every 10%
        qDebug() << "Download progress:" << percentage << "%";
    }
}

void UpdateChecker::onDownloadFinished()
{
    if (!m_currentReply) {
        qWarning() << "Null reply in onDownloadFinished";
        return;
    }
    
    QNetworkReply::NetworkError error = m_currentReply->error();
    
    if (error != QNetworkReply::NoError) {
        QString errorString = m_currentReply->errorString();
        qCritical() << "Download error:" << errorString;
        emit updateFailed(QString("Download error: %1").arg(errorString));
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    // Save the downloaded file
    QByteArray data = m_currentReply->readAll();
    QFile file(m_downloadPath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Impossible to write file:" << m_downloadPath;
        emit updateFailed("Impossible to save downloaded file");
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    qint64 written = file.write(data);
    file.close();
    
    if (written != data.size()) {
        qCritical() << "Incomplete file write";
        emit updateFailed("Incomplete download");
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }
    
    qInfo() << "Download finished:" << m_downloadPath;
    qInfo() << "File size:" << data.size() << "bytes";
    
    // Install the update
    if (installUpdate(m_downloadPath)) {
        emit updateCompleted();
    } else {
        emit updateFailed("Installation failed");
    }
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

bool UpdateChecker::verifyChecksum(const QString& filePath, const QString& expectedChecksum)
{
    // For now, we'll skip checksum verification since it's not easily available from the HTML
    // In a future version, we could add checksum information to the GitHub Pages site
    Q_UNUSED(filePath)
    Q_UNUSED(expectedChecksum)

    qDebug() << "Checksum verification skipped for now";
    return true;
}

bool UpdateChecker::installUpdate(const QString& zipPath)
{
    qInfo() << "Installing update from:" << zipPath;

    // Get current application directory
    QString currentDir = QCoreApplication::applicationDirPath();
    qDebug() << "Current application directory:" << currentDir;

    // Create backup directory
    QString backupDir = currentDir + "_backup_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    if (!QDir().mkpath(backupDir)) {
        qCritical() << "Impossible to create backup directory:" << backupDir;
        return false;
    }
    
    // Preserve user files
    preserveUserFiles(currentDir, backupDir);
    
    // Extract the ZIP file to a temporary location
    QString extractDir = m_tempDir->filePath("extracted");
    if (!QDir().mkpath(extractDir)) {
        qCritical() << "Impossible to create extract directory:" << extractDir;
        return false;
    }
    
    // Use PowerShell to extract the ZIP file (Windows)
    QProcess extractProcess;
    QString extractCommand = QString(
        "powershell.exe -Command \"Expand-Archive -Path '%1' -DestinationPath '%2' -Force\""
    ).arg(zipPath, extractDir);

    qDebug() << "Extraction command:" << extractCommand;

    extractProcess.start(extractCommand);
    if (!extractProcess.waitForFinished(30000)) { // 30 seconds timeout
        qCritical() << "Timeout while extracting ZIP";
        return false;
    }
    
    if (extractProcess.exitCode() != 0) {
        qCritical() << "Error while extracting:" << extractProcess.readAllStandardError();
        return false;
    }

    qInfo() << "Extraction finished";

    // Find the extracted directory (it should contain the application files)
    QDir extractedDir(extractDir);
    QStringList subDirs = extractedDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    QString sourceDir = extractDir;
    if (!subDirs.isEmpty()) {
        // If there's a subdirectory, use it as the source
        sourceDir = extractedDir.absoluteFilePath(subDirs.first());
    }

    qDebug() << "Source directory for installation:" << sourceDir;

    // Copy new files to application directory
    if (!copyDirectoryRecursively(sourceDir, currentDir)) {
        qCritical() << "Failed to copy new files";
        return false;
    }
    
    // Restore user files
    restoreUserFiles(backupDir, currentDir);

    qInfo() << "Installation completed successfully";
    return true;
}

void UpdateChecker::preserveUserFiles(const QString& oldPath, const QString& backupPath)
{
    qDebug() << "Preserving user files from" << oldPath << "to" << backupPath;

    // Preserve directories
    for (const QString& dirName : USER_PRESERVE_DIRS) {
        QString sourceDir = QDir(oldPath).absoluteFilePath(dirName);
        QString targetDir = QDir(backupPath).absoluteFilePath(dirName);
        
        if (QDir(sourceDir).exists()) {
            qDebug() << "Preserving directory:" << dirName;
            copyDirectoryRecursively(sourceDir, targetDir);
        }
    }
    
    // Preserve files
    for (const QString& fileName : USER_PRESERVE_FILES) {
        QString sourceFile = QDir(oldPath).absoluteFilePath(fileName);
        QString targetFile = QDir(backupPath).absoluteFilePath(fileName);
        
        if (QFile::exists(sourceFile)) {
            qDebug() << "Preserving file:" << fileName;
            QDir().mkpath(QFileInfo(targetFile).absolutePath());
            QFile::copy(sourceFile, targetFile);
        }
    }
}

void UpdateChecker::restoreUserFiles(const QString& backupPath, const QString& targetPath)
{
    qDebug() << "Restoring user files from" << backupPath << "to" << targetPath;
    
    // Restore directories
    for (const QString& dirName : USER_PRESERVE_DIRS) {
        QString sourceDir = QDir(backupPath).absoluteFilePath(dirName);
        QString targetDir = QDir(targetPath).absoluteFilePath(dirName);
        
        if (QDir(sourceDir).exists()) {
            qDebug() << "Restoring directory:" << dirName;
            // Remove target directory if it exists
            QDir(targetDir).removeRecursively();
            copyDirectoryRecursively(sourceDir, targetDir);
        }
    }
    
    // Restore files
    for (const QString& fileName : USER_PRESERVE_FILES) {
        QString sourceFile = QDir(backupPath).absoluteFilePath(fileName);
        QString targetFile = QDir(targetPath).absoluteFilePath(fileName);
        
        if (QFile::exists(sourceFile)) {
            qDebug() << "Restoring file:" << fileName;
            QFile::remove(targetFile); // Remove if exists
            QFile::copy(sourceFile, targetFile);
        }
    }
    
    // Clean up backup directory
    QDir(backupPath).removeRecursively();
}

bool UpdateChecker::copyDirectoryRecursively(const QString& sourceDir, const QString& targetDir)
{
    QDir source(sourceDir);
    if (!source.exists()) {
        return false;
    }
    
    QDir target(targetDir);
    if (!target.exists()) {
        target.mkpath(".");
    }
    
    // Copy files
    QStringList files = source.entryList(QDir::Files);
    for (const QString& fileName : files) {
        QString sourcePath = source.absoluteFilePath(fileName);
        QString targetPath = target.absoluteFilePath(fileName);
        
        // Remove target file if it exists
        if (QFile::exists(targetPath)) {
            QFile::remove(targetPath);
        }
        
        if (!QFile::copy(sourcePath, targetPath)) {
            qWarning() << "Failed to copy file:" << sourcePath << "->" << targetPath;
            return false;
        }
    }
    
    // Copy subdirectories recursively
    QStringList dirs = source.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dirName : dirs) {
        QString sourcePath = source.absoluteFilePath(dirName);
        QString targetPath = target.absoluteFilePath(dirName);
        
        if (!copyDirectoryRecursively(sourcePath, targetPath)) {
            return false;
        }
    }
    
    return true;
}