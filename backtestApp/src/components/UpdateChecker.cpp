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
#include "version.h"  

// DOWNLOAD_URL : https://fintechcpp.github.io/fast-backtest-app-releases/downloads/fast-backtest-app-windows-v1.0.0.19.zip

const QUrl UpdateChecker::GITHUB_PAGES_RELEASES_URL = QUrl(QStringLiteral("https://fintechcpp.github.io/fast-backtest-app-releases/"));

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_downloadFile(nullptr)
    , m_progressCount(0)
{
    qDebug() << "UpdateChecker constructor called";
    
    // Créer le gestionnaire réseau
    m_networkManager = new QNetworkAccessManager(this);
    qDebug() << "Network manager created";
}

UpdateChecker::~UpdateChecker()
{
    qDebug() << "UpdateChecker destructor called";
    
    // Nettoyer les ressources
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }
}

void UpdateChecker::downloadLatestRelease()
{
    qDebug() << "Starting minimal download test...";
    
    // URL de téléchargement (même que dans l'exemple qui fonctionne)
    QString url = "https://fintechcpp.github.io/fast-backtest-app-releases/downloads/fast-backtest-app-windows-v1.0.0.19.zip";
    qDebug() << "URL:" << url;
    
    // Créer la requête (exactement comme dans l'exemple qui fonctionne)
    QUrl downloadUrl(url);
    QNetworkRequest request;
    request.setUrl(downloadUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 Test App");
    request.setRawHeader("Accept", "*/*");
    
    qDebug() << "Starting download...";
    
    // Créer un fichier pour sauvegarder (même nom que dans l'exemple)
    m_downloadFile = new QFile("downloaded_update.zip");
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot open file for writing:" << m_downloadFile->fileName();
        emit downloadError("Cannot open file for writing");
        return;
    }
    
    // Réinitialiser le compteur de progrès
    m_progressCount = 0;
    
    // Démarrer le téléchargement
    m_currentReply = m_networkManager->get(request);
    
    // Connecter les signaux (exactement comme dans l'exemple qui fonctionne)
    connect(m_currentReply, &QNetworkReply::downloadProgress, 
            this, &UpdateChecker::onDownloadProgress);
    
    connect(m_currentReply, &QNetworkReply::readyRead, 
            this, &UpdateChecker::onReadyRead);
    
    connect(m_currentReply, &QNetworkReply::errorOccurred, 
            this, &UpdateChecker::onDownloadError);
    
    connect(m_currentReply, &QNetworkReply::finished, 
            this, &UpdateChecker::onDownloadFinished);
    
    // Afficher des informations sur la requête (comme dans l'exemple)
    qDebug() << "Request URL:" << request.url().toString();
    qDebug() << "User-Agent:" << request.header(QNetworkRequest::UserAgentHeader).toString();
    qDebug() << "Waiting for download to complete...";
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    // Même logique que dans l'exemple qui fonctionne
    m_progressCount++;
    if (m_progressCount % 10 == 0) { // Afficher seulement tous les 10 événements
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
    
    // Même logique que dans l'exemple qui fonctionne
    QByteArray data = m_currentReply->readAll();
    if (!data.isEmpty()) {
        qint64 written = m_downloadFile->write(data);
        qDebug() << "Wrote" << written << "bytes to file";
    }
}

void UpdateChecker::onDownloadError(QNetworkReply::NetworkError error)
{
    // Même logique que dans l'exemple qui fonctionne
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
    
    // Fermer le fichier
    m_downloadFile->close();
    
    // Vérifier le résultat (même logique que dans l'exemple qui fonctionne)
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
        
        // Supprimer le fichier vide en cas d'erreur
        m_downloadFile->remove();
        emit downloadCompleted(false, "");
    }
    
    // Nettoyer
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    delete m_downloadFile;
    m_downloadFile = nullptr;
}

QString UpdateChecker::currentVersion()
{
    return QString(APP_VERSION);
}