#include "data_loader.h"
#include <QApplication>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include "binding/pybinding.h"

namespace py = pybind11;

const QString DataLoader::MARKET_DATA_PATH = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../marketData");

DataLoader::DataLoader() {}
DataLoader::~DataLoader() {}

QString DataLoader::findMarketDataDirectory()
{
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir currentDir(exeDir);

    // Remonte dans l'arborescence pour trouver le dossier ig-trading-bot
    do {
        QString currentPath = currentDir.absolutePath();

        // Vérifie si c'est le dossier ig-trading-bot
        if (currentDir.dirName() == "ig-trading-bot") {
            QString marketDataPath = currentDir.absoluteFilePath("marketData");
            if (QFileInfo(marketDataPath).isDir()) {
                return marketDataPath;
            }
        }

        // Cherche un sous-dossier ig-trading-bot
        QString igTradingBotPath = currentDir.absoluteFilePath("ig-trading-bot");
        if (QFileInfo(igTradingBotPath).isDir()) {
            QString marketDataPath = QDir(igTradingBotPath).absoluteFilePath("marketData");
            if (QFileInfo(marketDataPath).isDir()) {
                return marketDataPath;
            }
        }

    } while (currentDir.cdUp());

    return QString(); // Not found
}

QStringList DataLoader::getMarketDataPaths()
{
    QStringList paths;
    QString marketDataPath = findMarketDataDirectory();
    if (!marketDataPath.isEmpty()) {
        paths << marketDataPath;
    }
    return paths;
}

QString DataLoader::findDataFile(const QString& symbol, const QString& interval)
{
    QString marketDataDir = findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        return QString();
    }

    // Rechercher un fichier correspondant au pattern symbol_interval_*.csv
    QDir dir(marketDataDir);
    QStringList nameFilters;
    nameFilters << QString("%1_%2_*.csv").arg(symbol, interval);

    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Time);

    if (files.isEmpty()) {
        qWarning() << "Aucun fichier trouvé pour le pattern:"
                   << QString("%1_%2_*.csv").arg(symbol, interval)
                   << "dans" << marketDataDir;
        return QString();
    }

    // Retourner le fichier le plus récent
    QString mostRecentFile = dir.absoluteFilePath(files.first());
    qDebug() << "Fichier trouvé:" << mostRecentFile;

    return mostRecentFile;
}

std::vector<OHLCBar> DataLoader::filterByPeriod(
    const std::vector<OHLCBar>& data,
    const QDateTime& startDate,
    const QDateTime& endDate)
{
    std::vector<OHLCBar> filtered;
    filtered.reserve(data.size());
    for (const auto& bar : data) {
        if (bar.timestamp >= startDate && bar.timestamp <= endDate) {
            filtered.push_back(bar);
        }
    }
    return filtered;
}

std::vector<OHLCBar> DataLoader::filterByTradingDays(
    const std::vector<OHLCBar>& data,
    const std::vector<int>& tradingDays)
{
    if (tradingDays.empty()) {
        return data;
    }
    std::vector<OHLCBar> filtered;
    filtered.reserve(data.size());
    for (const auto& bar : data) {
        // Qt: 1=Lundi, 7=Dimanche, mais nous utilisons 0=Lundi, 6=Dimanche
        int dayOfWeek = bar.timestamp.date().dayOfWeek() - 1;
        if (dayOfWeek == -1) dayOfWeek = 6; // Dimanche
        if (std::find(tradingDays.begin(), tradingDays.end(), dayOfWeek) != tradingDays.end()) {
            filtered.push_back(bar);
        }
    }
    return filtered;
}

std::vector<OHLCBar> DataLoader::filterByTradingHours(
    const std::vector<OHLCBar>& data,
    const QTime& tradingFrom,
    const QTime& tradingTo)
{
    if (!tradingFrom.isValid() || !tradingTo.isValid()) {
        return data;
    }
    std::vector<OHLCBar> filtered;
    filtered.reserve(data.size());
    qDebug() << "Filtrage par heures de trading:"
             << tradingFrom.toString("hh:mm")
             << "à" << tradingTo.toString("hh:mm");
    for (const auto& bar : data) {
        QTime barTime = bar.timestamp.time();
        bool inRange;
        if (tradingTo > tradingFrom) {
            inRange = (barTime >= tradingFrom && barTime <= tradingTo);
        } else {
            inRange = (barTime >= tradingFrom || barTime <= tradingTo);
        }
        if (inRange) {
            filtered.push_back(bar);
        }
    }
    return filtered;
}

int DataLoader::intervalToSeconds(const QString& interval)
{
    QString lowerInterval = interval.toLower();
    if (lowerInterval.endsWith("secs")) {
        bool ok;
        int seconds = lowerInterval.leftRef(lowerInterval.length() - 4).toInt(&ok);
        return ok ? seconds : 0;
    } else if (lowerInterval.endsWith("min")) {
        bool ok;
        int minutes = lowerInterval.leftRef(lowerInterval.length() - 3).toInt(&ok);
        return ok ? minutes * 60 : 0;
    } else if (lowerInterval.endsWith("h")) {
        bool ok;
        int hours = lowerInterval.leftRef(lowerInterval.length() - 1).toInt(&ok);
        return ok ? hours * 3600 : 0;
    } else if (lowerInterval.endsWith("d")) {
        bool ok;
        int days = lowerInterval.leftRef(lowerInterval.length() - 1).toInt(&ok);
        return ok ? days * 86400 : 0;
    }
    return 0;
}

std::vector<OHLCBar> DataLoader::resampleData(
    const std::vector<OHLCBar>& data,
    const QString& targetInterval)
{
    if (data.empty()) {
        return data;
    }
    int targetSeconds = intervalToSeconds(targetInterval);
    if (targetSeconds <= 0) {
        qWarning() << "Intervalle cible invalide:" << targetInterval;
        return data;
    }
    std::vector<OHLCBar> resampled;
    QDateTime currentPeriodStart = data[0].timestamp;
    QDateTime currentPeriodEnd = currentPeriodStart.addSecs(targetSeconds);
    double open = data[0].open;
    double high = data[0].high;
    double low = data[0].low;
    double close = data[0].close;
    double volume = data[0].volume;
    for (size_t i = 1; i < data.size(); ++i) {
        const auto& bar = data[i];
        if (bar.timestamp < currentPeriodEnd) {
            high = std::max(high, bar.high);
            low = std::min(low, bar.low);
            close = bar.close;
            volume += bar.volume;
        } else {
            resampled.emplace_back(currentPeriodStart, open, high, low, close, volume);
            currentPeriodStart = currentPeriodEnd;
            currentPeriodEnd = currentPeriodStart.addSecs(targetSeconds);
            open = bar.open;
            high = bar.high;
            low = bar.low;
            close = bar.close;
            volume = bar.volume;
        }
    }
    resampled.emplace_back(currentPeriodStart, open, high, low, close, volume);
    qDebug() << "Resampling de" << data.size() << "à" << resampled.size()
             << "barres pour l'intervalle" << targetInterval;
    return resampled;
}

std::vector<OHLCBar> DataLoader::loadData(
    const QString& symbol,
    const QString& interval,
    const QString& period,
    const QDateTime& endDate,
    const QTime& tradingFrom,
    const QTime& tradingTo,
    const std::vector<int>& tradingDays)
{
    try {
        qDebug() << "DataLoader::loadData called with symbol:" << symbol 
                 << "interval:" << interval << "period:" << period;
        qDebug() << "End date:" << endDate.toString("dd/MM/yyyy");
        
        QString dataFile = findDataFile(symbol, interval);
        if (dataFile.isEmpty()) {
            qWarning() << "Aucun fichier de données trouvé pour" << symbol << interval;
            return std::vector<OHLCBar>();
        }

        qDebug() << "Fichier de données trouvé:" << dataFile;
        
        QDateTime actualEndDate = endDate.isValid() ? endDate : QDateTime::currentDateTime();
        QString endDateString = actualEndDate.toString("dd/MM/yyyy");

        std::vector<OHLCBar> result = loadFromCSV(dataFile, period, endDateString, tradingFrom, tradingTo);
        
        // Filtrage par jours de trading si spécifiés
        if (!tradingDays.empty()) {
            result = filterByTradingDays(result, tradingDays);
            qDebug() << "Après filtrage par jours:" << result.size() << "barres de prix";
        }
        
        return result;
    } catch (const std::exception& e) {
        qCritical() << "Exception in loadData:" << e.what();
        return std::vector<OHLCBar>();
    }
}

std::vector<OHLCBar> DataLoader::loadFromCSV(
    const QString& filePath,
    const QString& period,
    const QString& endDate,
    const QTime& tradingFrom,
    const QTime& tradingTo)
{
    std::vector<OHLCBar> data;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Impossible d'ouvrir le fichier:" << filePath;
        return data;
    }
    QTextStream in(&file);
    QString line;
    bool isFirstLine = true;
    int lineCount = 0;
    qDebug() << "Chargement des données depuis:" << filePath;
    while (in.readLineInto(&line)) {
        lineCount++;
        if (isFirstLine) {
            isFirstLine = false;
            if (line.contains("date", Qt::CaseInsensitive) ||
                line.contains("time", Qt::CaseInsensitive) ||
                line.contains("open", Qt::CaseInsensitive)) {
                continue;
            }
        }
        auto bar = parseCSVLine(line, false);
        if (bar) {
            data.push_back(*bar);
        }
        if (lineCount % 100000 == 0) {
            qDebug() << "Lignes traitées:" << lineCount;
        }
    }
    file.close();
    qDebug() << "Données chargées:" << data.size() << "barres de prix depuis" << filePath;
    if (data.empty()) {
        qWarning() << "Aucune donnée valide trouvée dans le fichier";
        return data;
    }
    std::sort(data.begin(), data.end(),
              [](const OHLCBar& a, const OHLCBar& b) {
                  return a.timestamp < b.timestamp;
              });
    // Standardiser les timestamps en heure française (NY + 6h)
    for (auto& bar : data) {
        bar.timestamp = bar.timestamp.addSecs(6 * 3600);
    }
    QDateTime endDateTime;
    if (endDate.isEmpty()) {
        endDateTime = data.back().timestamp;
        qDebug() << "Date de fin automatique:" << endDateTime.toString("dd/MM/yyyy hh:mm:ss");
    } else {
        // Parser la date fournie (supposée au format dd/MM/yyyy)
        endDateTime = QDateTime::fromString(endDate, "dd/MM/yyyy");
        if (!endDateTime.isValid()) {
            endDateTime = QDateTime::fromString(endDate, "yyyy-MM-dd");
        }
        if (!endDateTime.isValid()) {
            qWarning() << "Format de date invalide:" << endDate;
            endDateTime = data.back().timestamp;
        } else {
            // S'assurer que l'heure est à la fin de la journée
            endDateTime.setTime(QTime(23, 59, 59));
            qDebug() << "Date de fin parsée:" << endDateTime.toString("dd/MM/yyyy hh:mm:ss");
        }
    }
    
    QDateTime startDateTime = calculateStartDate(endDateTime, period);
    qDebug() << "Filtrage des données pour la période"
             << startDateTime.toString("dd/MM/yyyy hh:mm")
             << "à" << endDateTime.toString("dd/MM/yyyy hh:mm");
    data = filterByPeriod(data, startDateTime, endDateTime);
    qDebug() << "Après filtrage par période:" << data.size() << "barres de prix";
    if (tradingFrom.isValid() && tradingTo.isValid()) {
        data = filterByTradingHours(data, tradingFrom, tradingTo);
        qDebug() << "Après trading hours filter:" << data.size() << "bars";
    }
    
    qDebug() << "Vérification des données chargées:";
    if (!data.empty()) {
        const auto& first = data.front();
        const auto& last = data.back();
        qDebug() << "Première barre:" << first.timestamp << "OHLC:" 
                 << first.open << first.high << first.low << first.close;
        qDebug() << "Dernière barre:" << last.timestamp << "OHLC:" 
                 << last.open << last.high << last.low << last.close;
    }
    
    return data;
}

std::unique_ptr<OHLCBar> DataLoader::parseCSVLine(const QString& line, bool hasHeader)
{
    Q_UNUSED(hasHeader);
    if (line.trimmed().isEmpty()) {
        return nullptr;
    }
    QStringList fields = line.split(',');
    if (fields.size() < 5) {
        return nullptr;
    }
    try {
        QString dateStr = fields[0].trimmed();
        QDateTime timestamp;
        
        // CORRECTION: Add support for ISO 8601 format with timezone
        timestamp = QDateTime::fromString(dateStr, "yyyy-MM-ddThh:mm:ss+00:00");
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, "yyyy-MM-ddThh:mm:ss.zzz+00:00");
        }
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, Qt::ISODate);
        }
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, "yyyy-MM-ddThh:mm:ss");
        }
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, "yyyy-MM-ddThh:mm:ss.zzz");
        }
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, "MM/dd/yyyy hh:mm:ss");
        }
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, "dd/MM/yyyy hh:mm:ss");
        }
        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(dateStr, "yyyy-MM-dd hh:mm:ss");
        }
        
        if (!timestamp.isValid()) {
            qWarning() << "Format de date non reconnu:" << dateStr;
            return nullptr;
        }
        
        // Parse OHLC values
        double open = fields[1].trimmed().toDouble();
        double high = fields[2].trimmed().toDouble();
        double low = fields[3].trimmed().toDouble();
        double close = fields[4].trimmed().toDouble();
        
        double volume = 0.0;
        if (fields.size() > 5) {
            volume = fields[5].trimmed().toDouble();
        }
        
        return std::make_unique<OHLCBar>(timestamp, open, high, low, close, volume);
        
    } catch (const std::exception& e) {
        qWarning() << "Erreur lors de l'analyse de la ligne CSV:" << e.what();
        return nullptr;
    }
}

QDateTime DataLoader::calculateStartDate(const QDateTime& endDate, const QString& period)
{
    QDateTime startDate = endDate;
    if (period.endsWith("y")) {
        bool ok;
        int years = period.leftRef(period.length() - 1).toInt(&ok);
        if (ok) {
            startDate = endDate.addYears(-years);
        }
    } else if (period.endsWith("m")) {
        bool ok;
        int months = period.leftRef(period.length() - 1).toInt(&ok);
        if (ok) {
            startDate = endDate.addMonths(-months);
        }
    } else if (period.endsWith("d")) {
        bool ok;
        int days = period.leftRef(period.length() - 1).toInt(&ok);
        if (ok) {
            startDate = endDate.addDays(-days);
        }
    } else {
        qWarning() << "Période non reconnue:" << period << ". Utilisez '1y', '6m', '30d', etc.";
        startDate = endDate.addMonths(-1);
    }
    return startDate;
}