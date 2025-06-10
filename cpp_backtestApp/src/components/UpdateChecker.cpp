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
#include "version.h"  

QString GITHUB_TOKEN = "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"; // Remplacer par votre token GitHub
QUrl GITHUB_API_URL = QUrl("https://api.github.com/repos/hugoMiCode/ig-trading-bot/releases/latest");

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_updateAvailable(false),
      m_checkCompleted(false)
{
}

UpdateChecker::~UpdateChecker()
{
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
    
    // Utiliser l'endpoint qui liste toutes les releases
    QNetworkRequest request(GITHUB_API_URL);
    
    // Configuration des en-têtes
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    
    // Récupérer le token depuis les paramètres ou l'environnement
    QString token = GITHUB_TOKEN;
    
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    }
    
    // Ajouter des logs pour déboguer
    qDebug() << "URL de requête:" << request.url().toString();
    
    // Envoi de la requête
    QNetworkReply* reply = m_networkManager->get(request);
    qDebug() << "Requête envoyée, en attente de réponse...";
    
    connect(reply, &QNetworkReply::finished, this, &UpdateChecker::onVersionCheckFinished);
    qDebug() << "Requête terminée, connectée au slot onVersionCheckFinished";
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
    
    // Obtenir le code de statut HTTP
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    
    qDebug() << "Réponse HTTP:" << statusCode;
    qDebug() << "Taille de la réponse:" << responseData.size() << "octets";
    
    if (reply->error() == QNetworkReply::NoError) {
        if (responseData.isEmpty() || responseData == "[]") {
            qInfo() << "Aucune release trouvée sur GitHub";
            emit noUpdateAvailable();
        } else {
            // Analyser la réponse (peut être une liste ou un objet unique)
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isArray()) {
                // Si c'est une liste de releases
                QJsonArray releases = doc.array();
                if (releases.isEmpty()) {
                    qInfo() << "Aucune release trouvée";
                    emit noUpdateAvailable();
                } else {
                    // Prendre la première release (la plus récente)
                    QJsonObject latestRelease = releases.at(0).toObject();
                    parseReleaseObject(latestRelease);
                }
            } else if (doc.isObject()) {
                // Si c'est une seule release
                parseReleaseObject(doc.object());
            } else {
                m_errorMessage = "Format de réponse inattendu";
                emit updateCheckFailed(m_errorMessage);
            }
        }
    } else {
        m_errorMessage = reply->errorString();
        
        // Afficher plus d'informations pour le débogage
        qWarning() << "Échec de la vérification des mises à jour. Code:" << statusCode;
        qWarning() << "Message d'erreur:" << m_errorMessage;
        
        if (!responseData.isEmpty()) {
            qWarning() << "Corps de la réponse:" << responseData;
        }
        
        // Gestion spécifique du 404
        if (statusCode == 404) {
            qInfo() << "Aucune release trouvée sur GitHub. Vérifiez que des releases ont été créées.";
            emit noUpdateAvailable();
        } else {
            emit updateCheckFailed(m_errorMessage);
        }
    }
    
    reply->deleteLater();
    m_checkCompleted = true;
    m_waitSemaphore.release();
}

// Nouvelle méthode pour analyser un objet release individuel
void UpdateChecker::parseReleaseObject(const QJsonObject& releaseObj)
{
    // Récupération de la version (tag_name)
    m_latestVersion = releaseObj["tag_name"].toString();
    if (m_latestVersion.startsWith('v') || m_latestVersion.startsWith('V')) {
        m_latestVersion = m_latestVersion.mid(1); // Suppression du préfixe 'v' si présent
    }
    
    qInfo() << "Version actuelle:" << currentVersion() << "- Dernière version:" << m_latestVersion;
    
    // Vérification si une version plus récente est disponible
    if (isNewerVersion(m_latestVersion)) {
        // Récupération de l'URL de téléchargement
        QJsonArray assets = releaseObj["assets"].toArray();
        for (const QJsonValue& asset : assets) {
            QJsonObject assetObj = asset.toObject();
            QString name = assetObj["name"].toString();
            
            // Sélection de l'asset en fonction de la plateforme
            #ifdef Q_OS_WIN // Windows
            if (name.endsWith(".zip") || name.endsWith(".exe")) {
            #elif defined(Q_OS_MAC) // macOS
            if (name.endsWith(".dmg") || name.endsWith(".zip")) {
            #else // Linux
            if (name.endsWith(".AppImage") || name.endsWith(".tar.gz") || name.endsWith(".zip")) {
            #endif
                m_downloadUrl = assetObj["browser_download_url"].toString();
                break;
            }
        }
        
        if (!m_downloadUrl.isEmpty()) {
            m_updateAvailable = true;
            qInfo() << "Mise à jour disponible:" << m_latestVersion;
            emit updateAvailable(m_latestVersion, m_downloadUrl);
        } else {
            m_errorMessage = "Aucun téléchargement adapté trouvé dans la dernière release";
            emit updateCheckFailed(m_errorMessage);
        }
    } else {
        qInfo() << "Aucune mise à jour disponible";
        emit noUpdateAvailable();
    }
}

void UpdateChecker::parseReleaseInfo(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        m_errorMessage = "Réponse JSON invalide";
        emit updateCheckFailed(m_errorMessage);
        return;
    }
    
    QJsonObject releaseObj = doc.object();
    
    // Récupération de la version (tag_name)
    m_latestVersion = releaseObj["tag_name"].toString();
    if (m_latestVersion.startsWith('v') || m_latestVersion.startsWith('V')) {
        m_latestVersion = m_latestVersion.mid(1); // Suppression du préfixe 'v' si présent
    }
    
    qInfo() << "Version actuelle:" << currentVersion() << "- Dernière version:" << m_latestVersion;
    
    // Vérification si la version est plus récente
    if (isNewerVersion(m_latestVersion)) {
        // Récupération de l'URL de téléchargement
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
            if (name.endsWith(".AppImage") || name.endsWith(".tar.gz")) {
            #endif
                m_downloadUrl = assetObj["browser_download_url"].toString();
                break;
            }
        }
        
        if (!m_downloadUrl.isEmpty()) {
            m_updateAvailable = true;
            qInfo() << "Mise à jour disponible:" << m_latestVersion;
            emit updateAvailable(m_latestVersion, m_downloadUrl);
        } else {
            m_errorMessage = "Aucun téléchargement adapté trouvé dans la dernière release";
            emit updateCheckFailed(m_errorMessage);
        }
    } else {
        qInfo() << "Aucune mise à jour disponible";
        emit noUpdateAvailable();
    }
}

bool UpdateChecker::isNewerVersion(const QString& latestVersion)
{
    QVersionNumber current = QVersionNumber::fromString(currentVersion());
    QVersionNumber latest = QVersionNumber::fromString(latestVersion);
    
    return latest > current;
}

void UpdateChecker::downloadAndInstallUpdate()
{
    if (!m_updateAvailable || m_downloadUrl.isEmpty()) {
        emit updateFailed("Aucune mise à jour disponible à télécharger");
        return;
    }

    qInfo() << "Téléchargement de la mise à jour depuis:" << m_downloadUrl;
    
    // Ouvrir l'URL dans le navigateur par défaut
    bool success = QDesktopServices::openUrl(QUrl(m_downloadUrl));
    
    if (success) {
        emit updateCompleted();
    } else {
        emit updateFailed("Impossible d'ouvrir le navigateur pour le téléchargement");
    }
}

void UpdateChecker::onDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "onDownloadFinished: reply est null";
        return;
    }
    
    qDebug() << "Téléchargement terminé";
    qDebug() << "Code de statut HTTP:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Erreur réseau:" << reply->error();
    qDebug() << "Taille des données reçues:" << reply->bytesAvailable() << "octets";
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        
        if (data.isEmpty()) {
            m_errorMessage = "Aucune donnée reçue lors du téléchargement";
            emit updateFailed(m_errorMessage);
            reply->deleteLater();
            return;
        }
        
        // Sauvegarde du fichier de mise à jour
        QString fileName = QUrl(m_downloadUrl).fileName();
        if (fileName.isEmpty()) {
            fileName = "update_file";
        }
        
        QString downloadPath = QDir::tempPath() + "/" + fileName;
        qDebug() << "Sauvegarde vers:" << downloadPath;
        
        QFile file(downloadPath);
        if (file.open(QIODevice::WriteOnly)) {
            qint64 bytesWritten = file.write(data);
            file.close();
            
            if (bytesWritten == data.size()) {
                qInfo() << "Mise à jour téléchargée avec succès vers:" << downloadPath;
                qInfo() << "Taille du fichier:" << bytesWritten << "octets";
                installUpdate(downloadPath);
            } else {
                m_errorMessage = QString("Erreur lors de l'écriture du fichier: %1/%2 octets écrits")
                                .arg(bytesWritten).arg(data.size());
                emit updateFailed(m_errorMessage);
            }
        } else {
            m_errorMessage = "Impossible de sauvegarder le fichier de mise à jour: " + file.errorString();
            emit updateFailed(m_errorMessage);
        }
    } else {
        // Plus de détails sur l'erreur
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorDetails = QString("Code HTTP: %1, Erreur: %2")
                              .arg(httpCode)
                              .arg(reply->errorString());
        
        // Vérifier s'il y a du contenu dans la réponse d'erreur
        QByteArray errorResponse = reply->readAll();
        if (!errorResponse.isEmpty()) {
            qDebug() << "Réponse d'erreur:" << errorResponse;
        }
        
        m_errorMessage = "Échec du téléchargement: " + errorDetails;
        emit updateFailed(m_errorMessage);
    }
    
    reply->deleteLater();
}
void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)

{
    if (bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        emit downloadProgress(percentage);
    }
}

void UpdateChecker::installUpdate(const QString& filePath)
{
    // Le processus d'installation dépend de la plateforme et du format
    
    // Exemple d'implémentation basique:
    #ifdef Q_OS_WIN
    if (filePath.endsWith(".exe")) {
        // Exécution de l'installateur
        if (QProcess::startDetached(filePath)) {
            qInfo() << "Installation lancée, fermeture de l'application...";
            emit updateCompleted();
            QCoreApplication::quit();
        } else {
            emit updateFailed("Impossible de lancer l'installateur");
        }
    } else if (filePath.endsWith(".zip")) {
        // Pour les ZIP, on pourrait extraire et lancer un script ou notifier l'utilisateur
        emit updateCompleted();
        QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(filePath)});
    }
    #elif defined(Q_OS_MAC)
    if (filePath.endsWith(".dmg")) {
        // Monter le DMG et lancer l'installateur
        QProcess::startDetached("open", {filePath});
        emit updateCompleted();
        QCoreApplication::quit();
    }
    #else // Linux
    if (filePath.endsWith(".AppImage")) {
        // Rendre exécutable et lancer
        QProcess::execute("chmod", {"+x", filePath});
        QProcess::startDetached(filePath);
        emit updateCompleted();
        QCoreApplication::quit();
    }
    #endif
    else {
        // Ouvrir le dossier contenant le fichier téléchargé
        QProcess::startDetached("xdg-open", {QFileInfo(filePath).dir().path()});
        emit updateCompleted();
    }
}