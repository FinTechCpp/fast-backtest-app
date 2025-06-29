#include "components/UpdateChecker.h"
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
#include "version.h"  

QString GITHUB_TOKEN = "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";
QUrl GITHUB_API_URL = QUrl("https://api.github.com/repos/FinTechCpp/fast-backtest-app/releases/latest");

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_updateAvailable(false),
      m_checkCompleted(false),
      m_downloadReply(nullptr),
      m_progressDialog(nullptr),
      m_advancedModeEnabled(true) // Activer le mode avancé par défaut
{
    initializePaths();
    createDirectoryStructure();
}

UpdateChecker::~UpdateChecker()
{
    if (m_progressDialog) {
        delete m_progressDialog;
    }
}

void UpdateChecker::initializePaths()
{
    // Dossier de l'application
    m_paths.applicationDir = QCoreApplication::applicationDirPath();
    
    // Dossiers utilisateur (séparés de l'application)
    QString appName = QCoreApplication::applicationName();
    
#ifdef Q_OS_WIN
    m_paths.userDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    m_paths.configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#elif defined(Q_OS_MAC)
    m_paths.userDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_paths.configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else // Linux
    m_paths.userDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    m_paths.configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + appName;
#endif
    
    // Dossiers temporaires
    m_paths.tempUpdateDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + appName + "_update";
    m_paths.backupDir = m_paths.userDataDir + "/backup";
    
    qInfo() << "Chemins initialisés:";
    qInfo() << "  App:" << m_paths.applicationDir;
    qInfo() << "  UserData:" << m_paths.userDataDir;
    qInfo() << "  Config:" << m_paths.configDir;
    qInfo() << "  TempUpdate:" << m_paths.tempUpdateDir;
    qInfo() << "  Backup:" << m_paths.backupDir;
}

void UpdateChecker::createDirectoryStructure()
{
    QDir().mkpath(m_paths.userDataDir);
    QDir().mkpath(m_paths.configDir);
    QDir().mkpath(m_paths.tempUpdateDir);
    QDir().mkpath(m_paths.backupDir);
}

QString UpdateChecker::currentVersion()
{
    return APP_VERSION;
}

void UpdateChecker::checkForUpdates()
{
    // Éviter les vérifications multiples simultanées
    if (!m_checkCompleted && !m_errorMessage.isEmpty()) {
        qInfo() << "Vérification des mises à jour déjà en cours, ignoré";
        return;
    }
    
    qInfo() << "Vérification des mises à jour...";
    m_checkCompleted = false;
    m_updateAvailable = false;
    m_errorMessage.clear();
    
    QNetworkRequest request(GITHUB_API_URL);
    
    // Configuration des en-têtes
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    
    QString token = GITHUB_TOKEN;
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    }
    
    qDebug() << "URL de requête:" << request.url().toString();
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &UpdateChecker::onVersionCheckFinished);
}

bool UpdateChecker::waitForUpdateCheck(int timeout)
{
    if (m_checkCompleted) {
        return m_updateAvailable;
    }
    
    return m_waitSemaphore.tryAcquire(1, timeout);
}

void UpdateChecker::onVersionCheckFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    
    qDebug() << "Réponse HTTP:" << statusCode;
    qDebug() << "Taille de la réponse:" << responseData.size() << "octets";
    
    if (reply->error() == QNetworkReply::NoError) {
        if (responseData.isEmpty() || responseData == "[]") {
            qInfo() << "Aucune release trouvée sur GitHub";
            emit noUpdateAvailable();
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isArray()) {
                QJsonArray releases = doc.array();
                if (releases.isEmpty()) {
                    qInfo() << "Aucune release trouvée";
                    emit noUpdateAvailable();
                } else {
                    QJsonObject latestRelease = releases.at(0).toObject();
                    parseReleaseObject(latestRelease);
                }
            } else if (doc.isObject()) {
                parseReleaseObject(doc.object());
            } else {
                m_errorMessage = "Format de réponse inattendu";
                emit updateCheckFailed(m_errorMessage);
            }
        }
    } else {
        m_errorMessage = reply->errorString();
        qWarning() << "Échec de la vérification des mises à jour. Code:" << statusCode;
        qWarning() << "Message d'erreur:" << m_errorMessage;
        
        if (statusCode == 404) {
            qInfo() << "Aucune release trouvée sur GitHub";
            emit noUpdateAvailable();
        } else {
            emit updateCheckFailed(m_errorMessage);
        }
    }
    
    reply->deleteLater();
    m_checkCompleted = true;
    m_waitSemaphore.release();
}

void UpdateChecker::parseReleaseObject(const QJsonObject& releaseObj)
{
    // Récupération de la version (tag_name)
    m_latestVersion = releaseObj["tag_name"].toString();
    if (m_latestVersion.startsWith('v') || m_latestVersion.startsWith('V')) {
        m_latestVersion = m_latestVersion.mid(1);
    }
    
    qInfo() << "Version actuelle:" << currentVersion() << "- Dernière version:" << m_latestVersion;
    
    // Vérification si une version plus récente est disponible
    if (isNewerVersion(m_latestVersion)) {
        // Récupération de l'URL de téléchargement et des métadonnées
        QJsonArray assets = releaseObj["assets"].toArray();
        for (const QJsonValue& asset : assets) {
            QJsonObject assetObj = asset.toObject();
            QString name = assetObj["name"].toString();
            
            // Sélection de l'asset en fonction de la plateforme
            #ifdef Q_OS_WIN
            if (name.endsWith(".zip") || name.endsWith(".exe")) {
            #elif defined(Q_OS_MAC)
            if (name.endsWith(".dmg") || name.endsWith(".zip")) {
            #else // Linux
            if (name.endsWith(".AppImage") || name.endsWith(".tar.gz") || name.endsWith(".zip")) {
            #endif
                m_downloadUrl = assetObj["browser_download_url"].toString();
                
                // Préparer la configuration de mise à jour avancée
                if (m_advancedModeEnabled) {
                    UpdateConfig config;
                    config.currentVersion = currentVersion();
                    config.newVersion = m_latestVersion;
                    config.downloadUrl = m_downloadUrl;
                    config.downloadSize = assetObj["size"].toVariant().toLongLong();
                    
                    // Calculer le checksum depuis la description si disponible
                    QString releaseBody = releaseObj["body"].toString();
                    QRegularExpression checksumRegex(R"(SHA256:\s*`([a-fA-F0-9]{64})`)");
                    QRegularExpressionMatch match = checksumRegex.match(releaseBody);
                    if (match.hasMatch()) {
                        config.checksum = match.captured(1);
                    }
                    
                    m_currentUpdate = config;
                    
                    m_updateAvailable = true;
                    qInfo() << "Mise à jour disponible (mode avancé):" << m_latestVersion;
                    
                    // Émettre les deux signaux pour compatibilité
                    emit updateAvailable(m_latestVersion, m_downloadUrl);
                    emit updateAvailableAdvanced(config);
                } else {
                    // Mode simple (existant)
                    m_updateAvailable = true;
                    qInfo() << "Mise à jour disponible:" << m_latestVersion;
                    emit updateAvailable(m_latestVersion, m_downloadUrl);
                }
                break;
            }
        }
        
        if (m_downloadUrl.isEmpty()) {
            m_errorMessage = "Aucun téléchargement adapté trouvé dans la dernière release";
            emit updateCheckFailed(m_errorMessage);
        }
    } else {
        qInfo() << "Aucune mise à jour disponible";
        emit noUpdateAvailable();
    }
}

// Méthode existante pour compatibilité
void UpdateChecker::downloadAndInstallUpdate()
{
    if (!m_updateAvailable || m_downloadUrl.isEmpty()) {
        emit updateFailed("Aucune mise à jour disponible à télécharger");
        return;
    }

    if (m_advancedModeEnabled && !m_currentUpdate.downloadUrl.isEmpty()) {
        // Utiliser le mode avancé
        downloadAndInstallUpdateAdvanced(m_currentUpdate);
    } else {
        // Mode simple existant - ouvrir dans le navigateur
        qInfo() << "Téléchargement de la mise à jour depuis:" << m_downloadUrl;
        bool success = QDesktopServices::openUrl(QUrl(m_downloadUrl));
        
        if (success) {
            emit updateCompleted();
        } else {
            emit updateFailed("Impossible d'ouvrir le navigateur pour le téléchargement");
        }
    }
}

// Nouvelle méthode pour le mode avancé
void UpdateChecker::downloadAndInstallUpdateAdvanced(const UpdateConfig& config)
{
    m_currentUpdate = config;
    
    // Créer la boîte de dialogue de progression
    if (m_progressDialog) {
        delete m_progressDialog;
    }
    
    m_progressDialog = new QProgressDialog(
        tr("Téléchargement de la mise à jour..."),
        tr("Annuler"),
        0, 100,
        qobject_cast<QWidget*>(parent())
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->show();
    
    // Préparer le téléchargement
    QString fileName = QString("update_v%1.zip").arg(config.newVersion);
    m_tempUpdatePath = m_paths.tempUpdateDir + "/" + fileName;
    
    // Supprimer l'ancien fichier s'il existe
    QFile::remove(m_tempUpdatePath);
    
    // Lancer le téléchargement
    QNetworkRequest request(config.downloadUrl);
    request.setRawHeader("User-Agent", "fast-backtest-app-Updater/1.0");
    
    m_downloadReply = m_networkManager->get(request);
    
    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &UpdateChecker::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &UpdateChecker::onDownloadFinished);
    
    connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
        if (m_downloadReply) {
            m_downloadReply->abort();
        }
    });
}

bool UpdateChecker::backupUserData()
{
    qInfo() << "Sauvegarde des données utilisateur...";
    
    // Nettoyer les anciennes sauvegardes
    QDir backupDir(m_paths.backupDir);
    backupDir.removeRecursively();
    QDir().mkpath(m_paths.backupDir);
    
    // Sauvegarder le dossier de configuration
    if (QDir(m_paths.configDir).exists()) {
        QString configBackup = m_paths.backupDir + "/config";
        if (!copyDirectoryRecursively(m_paths.configDir, configBackup)) {
            qWarning() << "Échec de la sauvegarde du dossier de configuration";
            return false;
        }
    }
    
    // Sauvegarder le dossier de données utilisateur
    if (QDir(m_paths.userDataDir).exists()) {
        QString dataBackup = m_paths.backupDir + "/userdata";
        if (!copyDirectoryRecursively(m_paths.userDataDir, dataBackup)) {
            qWarning() << "Échec de la sauvegarde du dossier de données";
            return false;
        }
    }
    
    // Sauvegarder les fichiers de config dans le dossier de l'app (legacy)
    QStringList configFiles = {"backtest_config.ini", "settings.ini", "user_profiles.json"};
    for (const QString& file : configFiles) {
        QString sourcePath = m_paths.applicationDir + "/" + file;
        if (QFile::exists(sourcePath)) {
            QString destPath = m_paths.backupDir + "/" + file;
            QFile::copy(sourcePath, destPath);
        }
    }
    
    qInfo() << "Sauvegarde terminée";
    return true;
}

bool UpdateChecker::extractUpdate(const QString& zipPath, const QString& extractDir)
{
    qInfo() << "Extraction de la mise à jour:" << zipPath << "vers" << extractDir;
    emit extractionProgress("Extraction en cours...");
    
    // Nettoyer le dossier de destination
    QDir extractDirObj(extractDir);
    if (extractDirObj.exists()) {
        extractDirObj.removeRecursively();
    }
    QDir().mkpath(extractDir);
    
    // Utiliser QProcess pour extraire
#ifdef Q_OS_WIN
    QProcess unzipProcess;
    QStringList args;
    args << "x" << zipPath << "-o" + extractDir << "-y";
    unzipProcess.start("7z", args);
    
    if (!unzipProcess.waitForStarted()) {
        // Fallback vers PowerShell
        QStringList psArgs;
        psArgs << "-Command" 
               << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                    .arg(zipPath).arg(extractDir);
        unzipProcess.start("powershell", psArgs);
    }
    
    if (!unzipProcess.waitForFinished(30000)) {
        qWarning() << "Timeout lors de l'extraction";
        return false;
    }
    
    if (unzipProcess.exitCode() != 0) {
        qWarning() << "Erreur lors de l'extraction:" << unzipProcess.readAllStandardError();
        return false;
    }
#else
    QProcess unzipProcess;
    QStringList args;
    args << "-o" << zipPath << "-d" << extractDir;
    unzipProcess.start("unzip", args);
    
    if (!unzipProcess.waitForFinished(30000)) {
        qWarning() << "Timeout lors de l'extraction";
        return false;
    }
    
    if (unzipProcess.exitCode() != 0) {
        qWarning() << "Erreur lors de l'extraction:" << unzipProcess.readAllStandardError();
        return false;
    }
#endif
    
    qInfo() << "Extraction terminée avec succès";
    return true;
}

bool UpdateChecker::mergeUserData(const QString& newAppDir)
{
    qInfo() << "Fusion des données utilisateur avec la nouvelle version";
    
    // 1. Migrer les fichiers de config depuis l'ancien dossier app
    QStringList configFiles = {"backtest_config.ini", "settings.ini", "user_profiles.json"};
    for (const QString& file : configFiles) {
        QString oldPath = m_paths.applicationDir + "/" + file;
        QString newPath = m_paths.configDir + "/" + file;
        
        if (QFile::exists(oldPath) && !QFile::exists(newPath)) {
            QDir().mkpath(QFileInfo(newPath).dir().path());
            QFile::copy(oldPath, newPath);
            qInfo() << "Migré:" << file << "vers le dossier de configuration utilisateur";
        }
    }
    
    // 2. Restaurer les sauvegardes
    if (QDir(m_paths.backupDir + "/config").exists()) {
        copyDirectoryRecursively(m_paths.backupDir + "/config", m_paths.configDir, false);
    }
    
    if (QDir(m_paths.backupDir + "/userdata").exists()) {
        copyDirectoryRecursively(m_paths.backupDir + "/userdata", m_paths.userDataDir, false);
    }
    
    // 3. Copier les fichiers de config directement sauvegardés
    for (const QString& file : configFiles) {
        QString backupPath = m_paths.backupDir + "/" + file;
        QString configPath = m_paths.configDir + "/" + file;
        
        if (QFile::exists(backupPath) && !QFile::exists(configPath)) {
            QDir().mkpath(QFileInfo(configPath).dir().path());
            QFile::copy(backupPath, configPath);
        }
    }
    
    qInfo() << "Fusion des données terminée";
    return true;
}

QString UpdateChecker::createUpdateScript(const QString& newAppDir)
{
#ifdef Q_OS_WIN
    QString scriptPath = m_paths.tempUpdateDir + "/update.bat";
    QFile scriptFile(scriptPath);
    
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "@echo off\n";
        out << "echo Mise a jour en cours...\n";
        out << "timeout /t 2 /nobreak >nul\n";
        
        // Sauvegarder l'ancien dossier
        out << QString("if exist \"%1_old\" rmdir /s /q \"%1_old\"\n").arg(m_paths.applicationDir);
        out << QString("move \"%1\" \"%1_old\"\n").arg(m_paths.applicationDir);
        
        // Installer la nouvelle version
        out << QString("move \"%1\" \"%2\"\n").arg(newAppDir).arg(m_paths.applicationDir);
        
        // Redémarrer l'application
        QString exePath = m_paths.applicationDir + "/" + QCoreApplication::applicationName() + ".exe";
        out << QString("start \"\" \"%1\"\n").arg(exePath);
        
        // Auto-destruction du script
        out << QString("del \"%1\"\n").arg(scriptPath);
        
        scriptFile.close();
        return scriptPath;
    }
#else
    QString scriptPath = m_paths.tempUpdateDir + "/update.sh";
    QFile scriptFile(scriptPath);
    
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "#!/bin/bash\n";
        out << "echo \"Mise à jour en cours...\"\n";
        out << "sleep 2\n";
        
        // Sauvegarder l'ancien dossier
        out << QString("if [ -d \"%1_old\" ]; then rm -rf \"%1_old\"; fi\n").arg(m_paths.applicationDir);
        out << QString("mv \"%1\" \"%1_old\"\n").arg(m_paths.applicationDir);
        
        // Installer la nouvelle version
        out << QString("mv \"%1\" \"%2\"\n").arg(newAppDir).arg(m_paths.applicationDir);
        
        // Rendre exécutable et redémarrer
        QString exePath = m_paths.applicationDir + "/" + QCoreApplication::applicationName();
        out << QString("chmod +x \"%1\"\n").arg(exePath);
        out << QString("\"%1\" &\n").arg(exePath);
        
        // Auto-destruction du script
        out << QString("rm \"%1\"\n").arg(scriptPath);
        
        scriptFile.close();
        
        QProcess::execute("chmod", {"+x", scriptPath});
        return scriptPath;
    }
#endif
    
    return QString();
}

bool UpdateChecker::installUpdateAdvanced(const QString& updateDir)
{
    qInfo() << "Installation avancée de la mise à jour...";
    
    // 1. Sauvegarder les données utilisateur
    if (!backupUserData()) {
        return false;
    }
    
    // 2. Trouver le dossier contenant les fichiers de l'application
    QString appSourceDir = updateDir;
    QDir updateDirObj(updateDir);
    QStringList subdirs = updateDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& subdir : subdirs) {
        QString subdirPath = updateDir + "/" + subdir;
        QDir subdirObj(subdirPath);
        QStringList exeFiles = subdirObj.entryList(QStringList() << "*.exe" << QCoreApplication::applicationName(), QDir::Files);
        if (!exeFiles.isEmpty()) {
            appSourceDir = subdirPath;
            break;
        }
    }
    
    // 3. Remplacer les fichiers de l'application
    QString tempAppDir = m_paths.applicationDir + "_new";
    
    if (!copyDirectoryRecursively(appSourceDir, tempAppDir)) {
        qWarning() << "Échec de la copie de la nouvelle version";
        return false;
    }
    
    // 4. Créer un script de mise à jour
    QString scriptPath = createUpdateScript(tempAppDir);
    if (scriptPath.isEmpty()) {
        return false;
    }
    
    // 5. Fusionner les données utilisateur
    if (!mergeUserData(tempAppDir)) {
        qWarning() << "Échec de la fusion des données utilisateur";
        return false;
    }
    
    // 6. Lancer le script et fermer l'application
    QProcess::startDetached(scriptPath);
    
    qInfo() << "Mise à jour installée, redémarrage de l'application...";
    return true;
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0 && m_progressDialog) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressDialog->setValue(percentage);
        
        QString progressText = tr("Téléchargement: %1 MB / %2 MB (%3%)")
            .arg(bytesReceived / 1024 / 1024)
            .arg(bytesTotal / 1024 / 1024)
            .arg(percentage);
        m_progressDialog->setLabelText(progressText);
    }
    
    if (bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        emit downloadProgress(percentage);
    }
}

void UpdateChecker::onDownloadFinished()
{
    if (!m_downloadReply) {
        return;
    }
    
    if (m_progressDialog) {
        m_progressDialog->setLabelText(tr("Installation en cours..."));
        m_progressDialog->setValue(100);
    }
    
    if (m_downloadReply->error() == QNetworkReply::NoError) {
        // Sauvegarder le fichier téléchargé
        QFile updateFile(m_tempUpdatePath);
        if (updateFile.open(QIODevice::WriteOnly)) {
            updateFile.write(m_downloadReply->readAll());
            updateFile.close();
            
            if (m_advancedModeEnabled) {
                // Mode avancé - extraction et installation automatique
                QString extractDir = m_paths.tempUpdateDir + "/extracted";
                if (extractUpdate(m_tempUpdatePath, extractDir)) {
                    if (installUpdateAdvanced(extractDir)) {
                        emit updateCompleted();
                        
                        if (m_progressDialog) {
                            m_progressDialog->close();
                        }
                        
                        // Fermer l'application pour permettre la mise à jour
                        QApplication::quit();
                        return;
                    }
                }
            } else {
                // Mode simple - utiliser l'ancienne méthode
                installUpdate(m_tempUpdatePath);
                return;
            }
        }
        
        emit updateFailed(tr("Échec de l'installation de la mise à jour"));
    } else {
        emit updateFailed(tr("Échec du téléchargement: %1").arg(m_downloadReply->errorString()));
    }
    
    if (m_progressDialog) {
        m_progressDialog->close();
    }
    
    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;
}

bool UpdateChecker::copyDirectoryRecursively(const QString& source, const QString& destination, bool overwrite)
{
    QDir sourceDir(source);
    if (!sourceDir.exists()) {
        return false;
    }
    
    QDir destDir(destination);
    if (!destDir.exists()) {
        QDir().mkpath(destination);
    }
    
    QStringList files = sourceDir.entryList(QDir::Files);
    for (const QString& file : files) {
        QString sourcePath = source + "/" + file;
        QString destPath = destination + "/" + file;
        
        if (QFile::exists(destPath)) {
            if (overwrite) {
                QFile::remove(destPath);
            } else {
                continue;
            }
        }
        
        if (!QFile::copy(sourcePath, destPath)) {
            qWarning() << "Échec de la copie:" << sourcePath << "vers" << destPath;
            return false;
        }
    }
    
    QStringList dirs = sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dir : dirs) {
        QString sourcePath = source + "/" + dir;
        QString destPath = destination + "/" + dir;
        
        if (!copyDirectoryRecursively(sourcePath, destPath, overwrite)) {
            return false;
        }
    }
    
    return true;
}

// Méthodes restantes (existantes, conservées)
void UpdateChecker::parseReleaseInfo(const QByteArray& data)
{
    // Conserver pour compatibilité, mais rediriger vers parseReleaseObject
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        m_errorMessage = "Réponse JSON invalide";
        emit updateCheckFailed(m_errorMessage);
        return;
    }
    
    parseReleaseObject(doc.object());
}

bool UpdateChecker::isNewerVersion(const QString& latestVersion)
{
    QVersionNumber current = QVersionNumber::fromString(currentVersion());
    QVersionNumber latest = QVersionNumber::fromString(latestVersion);
    
    return latest > current;
}

void UpdateChecker::installUpdate(const QString& filePath)
{
    // Méthode existante conservée pour compatibilité en mode simple
    #ifdef Q_OS_WIN
    if (filePath.endsWith(".exe")) {
        if (QProcess::startDetached(filePath)) {
            qInfo() << "Installation lancée, fermeture de l'application...";
            emit updateCompleted();
            QCoreApplication::quit();
        } else {
            emit updateFailed("Impossible de lancer l'installateur");
        }
    } else if (filePath.endsWith(".zip")) {
        emit updateCompleted();
        QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(filePath)});
    }
    #elif defined(Q_OS_MAC)
    if (filePath.endsWith(".dmg")) {
        QProcess::startDetached("open", {filePath});
        emit updateCompleted();
        QCoreApplication::quit();
    }
    #else // Linux
    if (filePath.endsWith(".AppImage")) {
        QProcess::execute("chmod", {"+x", filePath});
        QProcess::startDetached(filePath);
        emit updateCompleted();
        QCoreApplication::quit();
    }
    #endif
    else {
        QProcess::startDetached("xdg-open", {QFileInfo(filePath).dir().path()});
        emit updateCompleted();
    }
}

bool UpdateChecker::rollbackUpdate()
{
    // Implémenter le rollback si nécessaire
    QString oldAppDir = m_paths.applicationDir + "_old";
    if (QDir(oldAppDir).exists()) {
        // Restaurer l'ancienne version
        qInfo() << "Rollback de la mise à jour...";
        // TODO: Implémenter la logique de rollback
        emit rollbackCompleted();
        return true;
    }
    return false;
}

QString UpdateChecker::calculateChecksum(const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(&file);
        return hash.result().toHex();
    }
    return QString();
}

QStringList UpdateChecker::getUserDataFiles()
{
    return {"backtest_config.ini", "settings.ini", "user_profiles.json"};
}

bool UpdateChecker::validateUpdate(const QString& updateDir, const QString& expectedChecksum)
{
    if (expectedChecksum.isEmpty()) {
        return true; // Pas de validation si pas de checksum
    }
    
    // TODO: Implémenter la validation du checksum
    return true;
}

bool UpdateChecker::restoreUserData()
{
    // TODO: Implémenter la restauration des données
    return true;
}