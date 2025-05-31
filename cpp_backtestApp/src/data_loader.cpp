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

int DataLoader::intervalToSeconds(const QString& interval)
{
    QString lowerInterval = interval.toLower();
    if (lowerInterval.endsWith("secs")) {
        bool ok;
        int seconds = lowerInterval.first(lowerInterval.length() - 4).toInt(&ok);
        return ok ? seconds : 0;
    } else if (lowerInterval.endsWith("min")) {
        bool ok;
        int minutes = lowerInterval.first(lowerInterval.length() - 3).toInt(&ok);
        return ok ? minutes * 60 : 0;
    } else if (lowerInterval.endsWith("h")) {
        bool ok;
        int hours = lowerInterval.first(lowerInterval.length() - 1).toInt(&ok);
        return ok ? hours * 3600 : 0;
    } else if (lowerInterval.endsWith("d")) {
        bool ok;
        int days = lowerInterval.first(lowerInterval.length() - 1).toInt(&ok);
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
    const QDateTime& endDate)
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

        std::vector<OHLCBar> result = loadFromCSV(dataFile, period, endDateString);
        
        return result;
    } catch (const std::exception& e) {
        qCritical() << "Exception in loadData:" << e.what();
        return std::vector<OHLCBar>();
    }
}

QDateTime DataLoader::calculateStartDate(const QDateTime& endDate, const QString& period)
{
    QDateTime startDate = endDate;
    
    qDebug() << "Calcul de la date de début pour la période:" << period << "depuis:" << endDate.toString("dd/MM/yyyy hh:mm:ss");
    
    if (period.endsWith("d")) {
        // Périodes en jours
        bool ok;
        int days = period.first(period.length() - 1).toInt(&ok);
        if (ok && days > 0) {
            startDate = endDate.addDays(-days);
        }
    } else if (period.endsWith("w")) {
        // Périodes en semaines
        bool ok;
        int weeks = period.first(period.length() - 1).toInt(&ok);
        if (ok && weeks > 0) {
            startDate = endDate.addDays(-weeks * 7);
        }
    } else if (period.endsWith("m")) {
        // Périodes en mois
        bool ok;
        int months = period.first(period.length() - 1).toInt(&ok);
        if (ok && months > 0) {
            startDate = endDate.addMonths(-months);
        }
    } else if (period.endsWith("y")) {
        // Périodes en années
        bool ok;
        int years = period.first(period.length() - 1).toInt(&ok);
        if (ok && years > 0) {
            startDate = endDate.addYears(-years);
        }
    } else {
        qWarning() << "Format de période non reconnu:" << period;
        // Par défaut, prendre 10 jours
        startDate = endDate.addDays(-10);
    }
    
    qDebug() << "Date de début calculée:" << startDate.toString("dd/MM/yyyy hh:mm:ss");
    return startDate;
}

std::vector<OHLCBar> DataLoader::loadFromCSV(
    const QString& filePath,
    const QString& period,
    const QString& endDate)
{
    std::vector<OHLCBar> data;
    data.reserve(1000000); // Pré-allouer mémoire pour éviter les réallocations

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Impossible d'ouvrir le fichier:" << filePath;
        return data;
    }
    
    QTextStream in(&file);
    QString line;
    
    qDebug() << "Chargement des données depuis:" << filePath;
    
    QDateTime endDateTime;
    if (endDate.isEmpty()) {
        endDateTime = QDateTime::currentDateTime();
    } else {
        QDate parsedDate = QDate::fromString(endDate, "dd/MM/yyyy");
        endDateTime = parsedDate.isValid() ? QDateTime(parsedDate, QTime(23, 59, 59)) : QDateTime::currentDateTime();
    }
    QDateTime startDateTime = calculateStartDate(endDateTime, period);
    
    if (in.readLineInto(&line)) {
        if (line.startsWith("date,")) {
            // C'est l'en-tête, l'ignorer
        } else {
            // C'est une vraie donnée, la parser
            auto bar = parseCSVLine(line);
            if (bar && bar->timestamp >= startDateTime && bar->timestamp <= endDateTime) {
                data.push_back(*bar);
            }
        }
    }
    
    int lineCount = 1;

    // Chrono start
    auto chronoStart = std::chrono::high_resolution_clock::now();

    while (in.readLineInto(&line)) {
        lineCount++;

        auto bar = parseCSVLine(line);
        if (bar) {
            // Filtrer directement pendant le chargement
            if (bar->timestamp >= startDateTime && bar->timestamp <= endDateTime) {
                data.push_back(*bar);
            }
            else if (bar->timestamp > endDateTime) {
                qDebug() << "Fin de période atteinte à la ligne" << lineCount << ", arrêt du chargement";
                break;
            }
        }
    }

    // Chrono end
    auto chronoEnd = std::chrono::high_resolution_clock::now();
    auto chronoDuration = std::chrono::duration_cast<std::chrono::milliseconds>(chronoEnd - chronoStart).count();

    file.close();
    qDebug() << "Données chargées:" << data.size() << "barres de prix depuis" << filePath
             << "en" << chronoDuration << "ms";
    
    return data;
}

std::unique_ptr<OHLCBar> DataLoader::parseCSVLine(const QString& line)
{
    if (line.length() < 45) return nullptr;
    
    const QChar* d = line.constData();
    
    // Parser manuellement "2022-02-14 14:30:10+00:00"
    // YYYY-MM-DD HH:MM:SS
    int year = (d[0].digitValue() * 1000) + (d[1].digitValue() * 100) + 
               (d[2].digitValue() * 10) + d[3].digitValue();
    int month = (d[5].digitValue() * 10) + d[6].digitValue();
    int day = (d[8].digitValue() * 10) + d[9].digitValue();
    int hour = (d[11].digitValue() * 10) + d[12].digitValue();
    int minute = (d[14].digitValue() * 10) + d[15].digitValue();
    int second = (d[17].digitValue() * 10) + d[18].digitValue();
    
    QDateTime timestamp(QDate(year, month, day), QTime(hour, minute, second), QTimeZone::utc());
    
    if (!timestamp.isValid()) {
        return nullptr;
    }
    
    // Trouver les virgules et parser les valeurs
    int comma1 = line.indexOf(',', 25);
    int comma2 = line.indexOf(',', comma1 + 1);
    int comma3 = line.indexOf(',', comma2 + 1);
    int comma4 = line.indexOf(',', comma3 + 1);
    
    if (comma1 == -1 || comma2 == -1 || comma3 == -1 || comma4 == -1) {
        comma4 = line.length(); // Pas de volume
    }
    
    double open = line.sliced(comma1 + 1, comma2 - comma1 - 1).toDouble();
    double high = line.sliced(comma2 + 1, comma3 - comma2 - 1).toDouble();
    double low = line.sliced(comma3 + 1, comma4 - comma3 - 1).toDouble();
    double close = line.sliced(comma4 + 1).toDouble();
    
    return std::make_unique<OHLCBar>(timestamp, open, high, low, close, 0.0);
}