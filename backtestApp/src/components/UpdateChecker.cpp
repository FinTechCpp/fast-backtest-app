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
#include <QDirIterator>
#include <QThread>
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
    , m_autoInstallMode(false)
    , m_downloadedFilePath()
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

    // Show request information 
    qDebug() << "Request URL:" << request.url().toString();
    qDebug() << "User-Agent:" << request.header(QNetworkRequest::UserAgentHeader).toString();
    qDebug() << "Waiting for download to complete...";
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
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
    if (!m_currentReply || !m_downloadFile) 
        return;
    
    QByteArray data = m_currentReply->readAll();
    if (!data.isEmpty()) {
        qint64 written = m_downloadFile->write(data);
        qDebug() << "Wrote" << written << "bytes to file";
    }
}

void UpdateChecker::onDownloadError(QNetworkReply::NetworkError error)
{
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

    // Check the result
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
            m_downloadedFilePath = fileInfo.absoluteFilePath();
            emit downloadCompleted(true, fileInfo.absoluteFilePath());
            
            // If in auto-install mode, proceed with installation
            if (m_autoInstallMode) {
                qDebug() << "Auto-install mode enabled, starting installation...";
                QTimer::singleShot(1000, this, [this]() {
                    performAutoInstall(m_downloadedFilePath);
                });
            }
        }
    } else {
        QString errorMsg = QString("Erreur de téléchargement (Code: %1): %2")
                          .arg(static_cast<int>(error))
                          .arg(m_currentReply->errorString());
        
        qCritical() << "Download failed with error code:" << error;
        qCritical() << "Error description:" << m_currentReply->errorString();
        qCritical() << "HTTP status code:" << m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Remove empty file in case of error
        if (m_downloadFile) {
            m_downloadFile->remove();
        }
        emit downloadCompleted(false, "");
    
        if (m_autoInstallMode) {
            emit installationError("Échec du téléchargement : " + errorMsg);
        } else {
            emit downloadError(errorMsg);
        }
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

// New method for download and auto-install
void UpdateChecker::downloadAndInstallUpdate(const QString& version)
{
    qDebug() << "Starting download and install for version:" << version;
    
    // Enable auto-install mode
    m_autoInstallMode = true;
    
    // Start the download
    downloadLatestRelease(version);
}

bool UpdateChecker::extractZipFile(const QString& zipFilePath, const QString& extractPath)
{
    qDebug() << "Extracting ZIP file:" << zipFilePath << "to:" << extractPath;
    emit installationProgress("Extraction du fichier ZIP...");
    
    // Create the extraction directory
    QDir extractDir;
    if (!extractDir.mkpath(extractPath)) {
        qCritical() << "Cannot create extraction directory:" << extractPath;
        return false;
    }
    
#ifdef Q_OS_WIN
    // On Windows, use PowerShell to extract the ZIP
    QProcess extractProcess;
    QString command = "powershell.exe";
    QStringList arguments;
    arguments << "-Command" 
              << QString("Expand-Archive -Path \"%1\" -DestinationPath \"%2\" -Force")
                 .arg(zipFilePath)
                 .arg(extractPath);
    
    qDebug() << "Running extraction command:" << command << arguments.join(" ");
    
    extractProcess.start(command, arguments);
    if (!extractProcess.waitForStarted()) {
        qCritical() << "Failed to start extraction process";
        return false;
    }
    
    if (!extractProcess.waitForFinished(60000)) { // 60 seconds timeout
        qCritical() << "Extraction process timed out";
        extractProcess.kill();
        return false;
    }
    
    if (extractProcess.exitCode() != 0) {
        qCritical() << "Extraction failed with exit code:" << extractProcess.exitCode();
        qCritical() << "Error output:" << extractProcess.readAllStandardError();
        return false;
    }
    
#else
    // On Linux/Mac, use unzip command
    QProcess extractProcess;
    QString command = "unzip";
    QStringList arguments;
    arguments << "-o" << zipFilePath << "-d" << extractPath;
    
    qDebug() << "Running extraction command:" << command << arguments.join(" ");
    
    extractProcess.start(command, arguments);
    if (!extractProcess.waitForFinished(60000)) {
        qCritical() << "Extraction process timed out or failed";
        return false;
    }
    
    if (extractProcess.exitCode() != 0) {
        qCritical() << "Extraction failed with exit code:" << extractProcess.exitCode();
        return false;
    }
#endif
    
    qDebug() << "ZIP extraction completed successfully";
    emit installationProgress("Extraction terminée avec succès");
    return true;
}

void UpdateChecker::performAutoInstall(const QString& zipFilePath)
{
    qDebug() << "Starting auto-installation process for:" << zipFilePath;
    emit installationProgress("Début de l'installation...");
    
    try {
        // 1. Create temporary extraction directory
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString extractPath = tempDir + "/fast-backtest-app-update";
        
        qDebug() << "Temp extraction path:" << extractPath;
        
        // Clean up any existing temp directory
        QDir oldTempDir(extractPath);
        if (oldTempDir.exists()) {
            oldTempDir.removeRecursively();
        }
        
        // 2. Extract the ZIP file
        if (!extractZipFile(zipFilePath, extractPath)) {
            emit installationError("Échec de l'extraction du fichier ZIP");
            return;
        }
        
        // 3. Find the extracted executable and verify the structure
        QString extractedExePath;
        QString extractedDir;
        QDirIterator it(extractPath, QStringList() << "*.exe", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString exePath = it.next();
            QFileInfo fileInfo(exePath);
            if (fileInfo.baseName().contains("backtest", Qt::CaseInsensitive)) {
                extractedExePath = exePath;
                extractedDir = fileInfo.absolutePath();
                break;
            }
        }
        
        if (extractedExePath.isEmpty()) {
            emit installationError("Impossible de trouver l'exécutable dans le fichier extrait");
            return;
        }
        
        qDebug() << "Found extracted executable:" << extractedExePath;
        qDebug() << "Extracted directory:" << extractedDir;
        
        // Verify that we have a proper directory structure
        QDir sourceDir(extractedDir);
        QStringList sourceFiles = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << "Source files found:" << sourceFiles.size() << "items";
        
        if (sourceFiles.isEmpty()) {
            emit installationError("Le répertoire extrait est vide");
            return;
        }
        
        emit installationProgress("Structure vérifiée, préparation de l'installation...");
        
        // 4. Get current application info
        QString currentAppPath = QCoreApplication::applicationFilePath();
        QString currentAppDir = QCoreApplication::applicationDirPath();
        QString backupDir = currentAppDir + "_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        
        qDebug() << "Current app path:" << currentAppPath;
        qDebug() << "Current app dir:" << currentAppDir;
        qDebug() << "Backup dir:" << backupDir;
        
        // 5. Create installer script
        QString scriptPath;
        
#ifdef Q_OS_WIN
        scriptPath = tempDir + "/install_update.bat";
        QFile script(scriptPath);
        if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&script);
            out << "@echo off\n";
            out << "setlocal enabledelayedexpansion\n";
            out << "echo [INSTALLER] Waiting for application to close...\n";
            out << "timeout /t 5 /nobreak >nul\n";
            
            // Wait for the executable to be unlocked
            out << ":WAIT_LOOP\n";
            out << "tasklist /FI \"IMAGENAME eq " << QFileInfo(currentAppPath).fileName() << "\" 2>nul | find /I /N \"" << QFileInfo(currentAppPath).fileName() << "\">nul\n";
            out << "if \"%ERRORLEVEL%\"==\"0\" (\n";
            out << "    echo [INSTALLER] Application still running, waiting...\n";
            out << "    timeout /t 2 /nobreak >nul\n";
            out << "    goto WAIT_LOOP\n";
            out << ")\n";
            
            out << "echo [INSTALLER] Creating backup directory...\n";
            out << "if exist \"" << backupDir << "\" (\n";
            out << "    echo [INSTALLER] Removing old backup...\n";
            out << "    rmdir /s /q \"" << backupDir << "\"\n";
            out << ")\n";
            
            out << "echo [INSTALLER] Creating backup of current installation...\n";
            out << "mkdir \"" << backupDir << "\"\n";
            out << "if not exist \"" << backupDir << "\" (\n";
            out << "    echo [INSTALLER] ERROR: Failed to create backup directory\n";
            out << "    pause\n";
            out << "    exit /b 1\n";
            out << ")\n";
            
            // Copy current files to backup instead of moving the whole directory
            out << "xcopy \"" << currentAppDir << "\\*\" \"" << backupDir << "\\\" /E /I /H /Y >nul 2>&1\n";
            out << "if !errorlevel! neq 0 (\n";
            out << "    echo [INSTALLER] WARNING: Backup creation had issues, continuing anyway...\n";
            out << ")\n";
            
            out << "echo [INSTALLER] Installing new version...\n";
            
            // Get the source directory from the extracted executable
            QString sourceDir = extractedDir;
            
            // Remove old files (except backup) and copy new ones
            out << "for /f \"delims=\" %%i in ('dir /b \"" << currentAppDir << "\\\" 2^>nul') do (\n";
            out << "    if not \"%%i\"==\"" << QFileInfo(backupDir).fileName() << "\" (\n";
            out << "        if exist \"" << currentAppDir << "\\%%i\\\" (\n";
            out << "            rmdir /s /q \"" << currentAppDir << "\\%%i\" >nul 2>&1\n";
            out << "        ) else (\n";
            out << "            del /f /q \"" << currentAppDir << "\\%%i\" >nul 2>&1\n";
            out << "        )\n";
            out << "    )\n";
            out << ")\n";
            
            out << "echo [INSTALLER] Copying new files...\n";
            out << "xcopy \"" << sourceDir << "\\*\" \"" << currentAppDir << "\\\" /E /I /H /Y\n";
            out << "if !errorlevel! neq 0 (\n";
            out << "    echo [INSTALLER] ERROR: Failed to copy new files\n";
            out << "    echo [INSTALLER] Attempting to restore backup...\n";
            out << "    xcopy \"" << backupDir << "\\*\" \"" << currentAppDir << "\\\" /E /I /H /Y\n";
            out << "    pause\n";
            out << "    exit /b 1\n";
            out << ")\n";
            
            out << "echo [INSTALLER] Verifying installation...\n";
            out << "if not exist \"" << currentAppPath << "\" (\n";
            out << "    echo [INSTALLER] ERROR: New executable not found, restoring backup...\n";
            out << "    xcopy \"" << backupDir << "\\*\" \"" << currentAppDir << "\\\" /E /I /H /Y\n";
            out << "    pause\n";
            out << "    exit /b 1\n";
            out << ")\n";
            
            out << "echo [INSTALLER] Starting new version...\n";
            out << "start \"\" \"" << currentAppPath << "\"\n";
            out << "echo [INSTALLER] Cleaning up...\n";
            out << "timeout /t 3 /nobreak >nul\n";
            out << "rmdir /s /q \"" << extractPath << "\" >nul 2>&1\n";
            out << "del /f /q \"" << zipFilePath << "\" >nul 2>&1\n";
            out << "echo [INSTALLER] Update completed successfully!\n";
            out << "del \"%~f0\" >nul 2>&1\n";  // Delete the script itself
            script.close();
        }
#else
        scriptPath = tempDir + "/install_update.sh";
        QFile script(scriptPath);
        if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&script);
            out << "#!/bin/bash\n";
            out << "echo \"Waiting for application to close...\"\n";
            out << "sleep 3\n";
            out << "echo \"Creating backup...\"\n";
            out << "mv \"" << currentAppDir << "\" \"" << backupDir << "\"\n";
            out << "echo \"Copying new version...\"\n";
            out << "cp -r \"" << QFileInfo(extractedExePath).absolutePath() << "\" \"" << currentAppDir << "\"\n";
            out << "chmod +x \"" << currentAppPath << "\"\n";
            out << "echo \"Starting new version...\"\n";
            out << "\"" << currentAppPath << "\" &\n";
            out << "echo \"Cleaning up...\"\n";
            out << "sleep 2\n";
            out << "rm -rf \"" << extractPath << "\"\n";
            out << "rm \"" << zipFilePath << "\"\n";
            out << "rm \"$0\"\n";  // Delete the script itself
            script.close();
            
            // Make script executable
            QProcess::execute("chmod", QStringList() << "+x" << scriptPath);
        }
#endif
        
        if (!QFile::exists(scriptPath)) {
            emit installationError("Impossible de créer le script d'installation");
            return;
        }
        
        emit installationProgress("Script d'installation créé, fermeture de l'application...");
        
        // 6. Start the installer script
        qDebug() << "Starting installer script:" << scriptPath;
        
#ifdef Q_OS_WIN
        QProcess::startDetached("cmd.exe", QStringList() << "/C" << scriptPath);
#else
        QProcess::startDetached("bash", QStringList() << scriptPath);
#endif
        
        emit installationCompleted(true, "Installation en cours... L'application va redémarrer.");
        
        // 7. Close the current application after a longer delay to ensure script starts properly
        QTimer::singleShot(5000, []() {
            qDebug() << "Closing application for update installation...";
            QApplication::quit();
        });
        
    } catch (const std::exception& e) {
        qCritical() << "Exception during auto-install:" << e.what();
        emit installationError(QString("Erreur lors de l'installation: %1").arg(e.what()));
    }
    
    // Reset auto-install mode
    m_autoInstallMode = false;
}