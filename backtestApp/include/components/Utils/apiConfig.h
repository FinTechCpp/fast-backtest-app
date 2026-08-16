#pragma once

#include <QSettings>
#include <QString>

// Minimal, centralized configuration for the market-data REST API.
// Storing the base URL in QSettings means it can later be swapped (e.g. for
// a future public endpoint) without touching any call site.
//
// Current deployment: Dockerized server on the private VPN network,
// reachable only while connected to the VPN (see local_data_server/README.md).
namespace ApiConfig {

inline QString defaultBaseUrl() {
    return QStringLiteral("http://10.8.0.1:9015");
}

inline QString baseUrl() {
    QSettings settings("fast-backtest-app", "BacktestApp");
    QString value = settings.value("dataApiBaseUrl").toString();
    return value.isEmpty() ? defaultBaseUrl() : value;
}

inline void setBaseUrl(const QString& url) {
    QSettings settings("fast-backtest-app", "BacktestApp");
    settings.setValue("dataApiBaseUrl", url);
}

} // namespace ApiConfig
