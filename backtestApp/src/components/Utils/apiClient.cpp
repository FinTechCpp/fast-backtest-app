#include "components/Utils/apiClient.h"
#include "components/Utils/apiConfig.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {

// Simple blocking GET helper for the PoC: runs a local event loop until the
// reply finishes or a timeout elapses. Good enough for a local API; a real
// production client would use async signals/slots instead.
QByteArray blockingGet(const QUrl& url, bool& ok, QString& errorMessage, int timeoutMs = 5000)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeoutTimer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        ok = false;
        errorMessage = "Unable to connect to data server. Please check your VPN connection.";
        return {};
    }

    if (reply->error() != QNetworkReply::NoError) {
        ok = false;
        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
            case QNetworkReply::RemoteHostClosedError:
            case QNetworkReply::HostNotFoundError:
            case QNetworkReply::TimeoutError:
            case QNetworkReply::NetworkSessionFailedError:
            case QNetworkReply::UnknownNetworkError:
                // Typical symptoms of the VPN being down / 10.8.0.1 unreachable.
                errorMessage = "Unable to connect to data server. Please check your VPN connection.";
                break;
            default:
                errorMessage = QString("HTTP request failed for %1: %2").arg(url.toString(), reply->errorString());
                break;
        }
        return {};
    }

    ok = true;
    errorMessage.clear();
    return reply->readAll();
}

} // namespace

QStringList ApiClient::listFiles(bool& ok, QString& errorMessage)
{
    QUrl url(ApiConfig::baseUrl() + "/files");
    QByteArray body = blockingGet(url, ok, errorMessage);
    if (!ok) {
        return {};
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        ok = false;
        errorMessage = QString("Invalid JSON response from %1: %2").arg(url.toString(), parseError.errorString());
        return {};
    }

    QStringList files;
    for (const QJsonValue& value : doc.array()) {
        files << value.toString();
    }
    return files;
}

QByteArray ApiClient::downloadFile(const QString& filename, bool& ok, QString& errorMessage)
{
    if (filename.isEmpty()) {
        ok = false;
        errorMessage = "downloadFile called with an empty filename";
        return {};
    }

    QUrl url(ApiConfig::baseUrl() + "/files/" + filename);
    QByteArray data = blockingGet(url, ok, errorMessage);
    if (!ok) {
        return {};
    }

    if (data.isEmpty()) {
        ok = false;
        errorMessage = QString("Downloaded file is empty: %1").arg(filename);
        return {};
    }

    return data;
}
