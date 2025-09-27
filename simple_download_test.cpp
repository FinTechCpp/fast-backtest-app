#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QFileInfo>
#include <QUrl>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting simple download test...";
    
    // URL de téléchargement
    QString url = "https://fintechcpp.github.io/fast-backtest-app-releases/downloads/fast-backtest-app-windows-v1.0.0.19.zip";
    qDebug() << "URL:" << url;
    
    // Créer le gestionnaire réseau
    QNetworkAccessManager manager;
    
    // Créer la requête
    QUrl downloadUrl(url);
    QNetworkRequest request;
    request.setUrl(downloadUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 Test App");
    request.setRawHeader("Accept", "*/*");
    
    qDebug() << "Starting download...";
    
    // Démarrer le téléchargement
    QNetworkReply* reply = manager.get(request);
    
    // Créer un fichier pour sauvegarder
    QFile file("downloaded_update.zip");
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot open file for writing:" << file.fileName();
        reply->deleteLater();
        return 1;
    }
    
    // Variables pour suivre le progrès
    int progressCount = 0;
    
    // Connecter les signaux
    QObject::connect(reply, &QNetworkReply::downloadProgress, 
        [&progressCount](qint64 bytesReceived, qint64 bytesTotal) {
            progressCount++;
            if (progressCount % 10 == 0) { // Afficher seulement tous les 10 événements
                if (bytesTotal > 0) {
                    int percent = (bytesReceived * 100) / bytesTotal;
                    qDebug() << "Progress:" << bytesReceived << "/" << bytesTotal << "(" << percent << "%)";
                } else {
                    qDebug() << "Progress:" << bytesReceived << "bytes received";
                }
            }
        });
    
    QObject::connect(reply, &QNetworkReply::readyRead, [&file, reply]() {
        QByteArray data = reply->readAll();
        if (!data.isEmpty()) {
            qint64 written = file.write(data);
            qDebug() << "Wrote" << written << "bytes to file";
        }
    });
    
    QObject::connect(reply, &QNetworkReply::errorOccurred, [reply](QNetworkReply::NetworkError error) {
        qWarning() << "Network error occurred:" << error;
        qWarning() << "Error string:" << reply->errorString();
    });
    
    // Utiliser une boucle d'événements pour attendre
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    // Timer de timeout
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&loop, reply]() {
        qWarning() << "Download timed out!";
        reply->abort();
        loop.quit();
    });
    timeoutTimer.start(30000); // 30 secondes
    
    // Afficher des informations sur la requête
    qDebug() << "Request URL:" << request.url().toString();
    qDebug() << "User-Agent:" << request.header(QNetworkRequest::UserAgentHeader).toString();
    
    // Démarrer la boucle d'événements
    qDebug() << "Waiting for download to complete...";
    loop.exec();
    
    // Arrêter le timer
    timeoutTimer.stop();
    
    // Fermer le fichier
    file.close();
    
    // Vérifier le résultat
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError) {
        QFileInfo fileInfo(file.fileName());
        qDebug() << "Download completed successfully!";
        qDebug() << "File saved as:" << fileInfo.absoluteFilePath();
        qDebug() << "File size:" << fileInfo.size() << "bytes";
        
        if (fileInfo.size() == 0) {
            qWarning() << "Warning: File size is 0 bytes!";
        }
    } else {
        qCritical() << "Download failed with error code:" << error;
        qCritical() << "Error description:" << reply->errorString();
        
        // Supprimer le fichier vide en cas d'erreur
        file.remove();
    }
    
    // Nettoyer
    reply->deleteLater();
    
    return (error == QNetworkReply::NoError) ? 0 : 1;
}