#include "components/data_loader.h"
#include <QApplication>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QStringList>
#include <QTimeZone>
#include <algorithm>
#include <cmath>


const QString DataLoader::MARKET_DATA_PATH = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../marketData");

DataLoader::DataLoader() {}
DataLoader::~DataLoader() {}

DataFileInfo DataLoader::checkDataFile(const QString& filePath)
{
    DataFileInfo info;
    
    // Set basic file info
    QFileInfo fileInfo(filePath);
    info.filePath = filePath;
    info.fileName = fileInfo.fileName();
    info.fileSize = fileInfo.size();
    
    // Default to invalid until we confirm it's valid
    info.isValid = false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Impossible d'ouvrir le fichier:" << filePath;
        return info;
    }
    
    QTextStream in(&file);
    QString line;
    int lineNumber = 0;
    
    // Check if first line is header
    if (in.readLineInto(&line)) {
        lineNumber++;
        if (line.startsWith("date,") || line.contains(",open,high,low,close")) {
            info.hasHeader = true;
        } else {
            // Try to parse the first line
            auto bar = parseCSVLine(line);
            if (bar) {
                info.hasHeader = false;
                info.totalRows = 1;
                info.startDate = bar->timestamp;
                info.endDate = bar->timestamp;
                info.minPrice = std::min({bar->open, bar->high, bar->low, bar->close});
                info.maxPrice = std::max({bar->open, bar->high, bar->low, bar->close});
                
                // Check OHLC relationship integrity
                if (bar->high < bar->low || bar->high < bar->open || bar->high < bar->close ||
                    bar->low > bar->open || bar->low > bar->close) {
                    info.invalidRows++;
                    info.invalidRowDetails.append(QString("Ligne %1: Relation OHLC invalide").arg(lineNumber));
                } else {
                    info.isValid = true;
                }
            } else {
                info.invalidRows++;
                info.invalidRowDetails.append(QString("Ligne %1: Format invalide").arg(lineNumber));
            }
        }
    }
    
    QDateTime prevTimestamp;
    QList<int> intervals; // Store time differences to detect the interval
    
    // Process remaining lines
    while (in.readLineInto(&line)) {
        lineNumber++;
        
        // Skip processing for header row
        if (lineNumber == 1 && info.hasHeader) {
            info.totalRows++;
            continue;
        }
        
        auto bar = parseCSVLine(line);
        if (bar) {
            info.totalRows++;
            
            // Check OHLC relationship integrity
            if (bar->high < bar->low || bar->high < bar->open || bar->high < bar->close ||
                bar->low > bar->open || bar->low > bar->close) {
                info.invalidRows++;
                info.invalidRowDetails.append(QString("Ligne %1: Relation OHLC invalide").arg(lineNumber));
                continue;
            }
            
            // Update date range
            if (info.startDate.isNull() || bar->timestamp < info.startDate) {
                info.startDate = bar->timestamp;
            }
            if (info.endDate.isNull() || bar->timestamp > info.endDate) {
                info.endDate = bar->timestamp;
            }
            
            // Update price range
            info.minPrice = std::min({info.minPrice, bar->open, bar->high, bar->low, bar->close});
            info.maxPrice = std::max({info.maxPrice, bar->open, bar->high, bar->low, bar->close});
            
            // Check for time gaps
            if (!prevTimestamp.isNull()) {
                int seconds = prevTimestamp.secsTo(bar->timestamp);
                
                // Store interval for detection (ignore large gaps for interval detection)
                if (seconds > 0 && seconds < 3600) {
                    intervals.append(seconds);
                }
                
                // Check if timestamps are out of order
                if (seconds < 0) {
                    info.invalidRows++;
                    info.invalidRowDetails.append(
                        QString("Ligne %1: Horodatage hors séquence (%2 après %3)")
                            .arg(lineNumber)
                            .arg(bar->timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                            .arg(prevTimestamp.toString("yyyy-MM-dd hh:mm:ss"))
                    );
                }
                
                // Flag large gaps (more than 5 minutes initially)
                if (seconds > 300) {
                    // Store this gap
                    if (info.largestGaps.size() < 10) { // Keep up to 10 largest gaps
                        info.largestGaps.append(qMakePair(prevTimestamp, bar->timestamp));
                        // Sort by gap size (largest first)
                        std::sort(info.largestGaps.begin(), info.largestGaps.end(), 
                            [](const QPair<QDateTime, QDateTime>& a, const QPair<QDateTime, QDateTime>& b) {
                                return a.first.secsTo(a.second) > b.first.secsTo(b.second);
                            });
                    } else if (seconds > prevTimestamp.secsTo(info.largestGaps.last().second)) {
                        // Replace the smallest gap if this one is larger
                        info.largestGaps.removeLast();
                        info.largestGaps.append(qMakePair(prevTimestamp, bar->timestamp));
                        std::sort(info.largestGaps.begin(), info.largestGaps.end(), 
                            [](const QPair<QDateTime, QDateTime>& a, const QPair<QDateTime, QDateTime>& b) {
                                return a.first.secsTo(a.second) > b.first.secsTo(b.second);
                            });
                    }
                }
            }
            
            prevTimestamp = bar->timestamp;
            info.isValid = true;
        } else {
            info.invalidRows++;
            info.invalidRowDetails.append(QString("Ligne %1: Format invalide").arg(lineNumber));
        }
    }
    
    file.close();
    
    // Calculate duration in days
    if (info.startDate.isValid() && info.endDate.isValid()) {
        info.durationDays = info.startDate.daysTo(info.endDate) + 1; // Include both start and end day
    }
    
    // Detect interval from collected time differences
    if (!intervals.isEmpty()) {
        // Sort and take the most common interval (mode)
        std::sort(intervals.begin(), intervals.end());
        
        // Find the most frequent interval
        QMap<int, int> frequencyMap;
        for (int interval : intervals) {
            frequencyMap[interval]++;
        }
        
        int mostFrequentInterval = 0;
        int highestFrequency = 0;
        
        for (auto it = frequencyMap.begin(); it != frequencyMap.end(); ++it) {
            if (it.value() > highestFrequency) {
                highestFrequency = it.value();
                mostFrequentInterval = it.key();
            }
        }
        
        // Convert to human-readable interval
        if (mostFrequentInterval < 60) {
            info.interval = QString("%1secs").arg(mostFrequentInterval);
        } else if (mostFrequentInterval < 3600) {
            info.interval = QString("%1min").arg(mostFrequentInterval / 60);
        } else {
            info.interval = QString("%1h").arg(mostFrequentInterval / 3600);
        }
        
        // Now refine gap detection with detected interval
        int expectedInterval = mostFrequentInterval;
        info.gapsCount = 0;
        
        for (const auto& gap : info.largestGaps) {
            int gapSeconds = gap.first.secsTo(gap.second);
            
            // Check if this is a significant gap (more than 2x expected interval)
            // but ignore gaps overnight/weekends
            if (gapSeconds > expectedInterval * 2) {
                // Check if the gap is during market hours
                // This is a simple check - real implementation should account for market calendar
                QTime startTime = gap.first.time();
                QTime endTime = gap.second.time();
                int daysDiff = gap.first.daysTo(gap.second);
                
                // If gap spans more than 1 day or is overnight, it might be normal
                // Simplified check for after market close to before market open
                bool isNormalGap = (daysDiff > 1) || 
                                  (daysDiff == 1 && startTime.hour() >= 16 && endTime.hour() <= 9) ||
                                  (startTime.hour() >= 16 && endTime.hour() >= 9 && daysDiff == 0);
                
                if (!isNormalGap) {
                    info.gapsCount++;
                }
            }
        }
    }
    
    // Final validity check
    info.isValid = info.isValid && (info.invalidRows == 0 || 
                                   (info.invalidRows * 100.0 / info.totalRows) < 5.0); // Less than 5% invalid
    
    return info;
}

QString DataLoader::findMarketDataDirectory()
{
    // 1. PREMIÈRE ÉTAPE: Vérifier si un chemin personnalisé est défini dans QSettings
    QSettings settings("IG-Trading-Bot", "BacktestApp");
    QString customPath = settings.value("marketDataPath").toString();
    if (!customPath.isEmpty() && QDir(customPath).exists()) {
        qDebug() << "Utilisation du répertoire personnalisé:" << customPath;
        return customPath;
    }

    // 2. Sinon, continuer avec la recherche standard
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

bool DataLoader::setCustomMarketDataDirectory(const QString& path)
{
    // Vérifier que le chemin existe ou peut être créé
    QDir dir(path);
    if (!dir.exists()) {
        if (!QDir().mkpath(path)) {
            qWarning() << "Impossible de créer le répertoire:" << path;
            return false;
        }
    }
    
    // Vérifier les permissions d'écriture
    QFileInfo dirInfo(path);
    if (!dirInfo.isWritable()) {
        qWarning() << "Le répertoire n'est pas accessible en écriture:" << path;
        return false;
    }
    
    // Sauvegarder le chemin dans les paramètres
    QSettings settings("IG-Trading-Bot", "BacktestApp");
    settings.setValue("marketDataPath", path);
    qInfo() << "Répertoire personnalisé défini:" << path;
    
    return true;
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
    
    // Trouver la première virgule pour sauter l'index
    int firstComma = line.indexOf(',');
    if (firstComma == -1) return nullptr;
    
    // S'assurer qu'il y a assez de caractères après la virgule
    if (line.length() < firstComma + 25) return nullptr;
    
    // Pointer vers le début de la date (après l'index et la virgule)
    const QChar* d = line.constData() + firstComma + 1;
    
    // Parser manuellement "2022-02-14 14:30:10+00:00" avec calcul direct (évite digitValue)
    int year = ((d[0].unicode() - '0') * 1000) + ((d[1].unicode() - '0') * 100) + 
               ((d[2].unicode() - '0') * 10) + (d[3].unicode() - '0');
    int month = ((d[5].unicode() - '0') * 10) + (d[6].unicode() - '0');
    int day = ((d[8].unicode() - '0') * 10) + (d[9].unicode() - '0');
    int hour = ((d[11].unicode() - '0') * 10) + (d[12].unicode() - '0');
    int minute = ((d[14].unicode() - '0') * 10) + (d[15].unicode() - '0');
    int second = ((d[17].unicode() - '0') * 10) + (d[18].unicode() - '0');
    
    // Utiliser Qt::UTC directement plutôt que QTimeZone::utc() qui est coûteux
    static const QDate nullDate(1970, 1, 1);
    static const QTime nullTime(0, 0, 0);
    
    // Vérifier rapidement si les valeurs sont dans des plages valides avant de créer QDateTime
    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour > 23 || minute > 59 || second > 59) {
        return nullptr;
    }
    
    QDateTime timestamp(QDate(year, month, day), QTime(hour, minute, second), Qt::UTC);
    
    // Optimisation: trouver toutes les virgules en un seul passage
    const QChar* ptr = line.constData();
    const QChar* end = ptr + line.length();
    ptr += firstComma + 25;  // Sauter l'index et la partie date
    
    // Trouver les virgules pour les données OHLC
    const QChar* commaPos[4] = {nullptr, nullptr, nullptr, nullptr};
    int commaCount = 0;
    
    while (ptr < end && commaCount < 4) {
        if (ptr->unicode() == ',') {
            commaPos[commaCount++] = ptr;
        }
        ++ptr;
    }
    
    if (commaCount < 3) return nullptr; // Pas assez de virgules pour les données OHLC
    
    // Parser les valeurs numériques directement
    double values[4];  // open, high, low, close
    
    // Parser open
    values[0] = parseDouble(commaPos[0] + 1, commaPos[1]);
    
    // Parser high
    values[1] = parseDouble(commaPos[1] + 1, commaPos[2]);
    
    // Parser low et close
    if (commaCount == 4) {
        values[2] = parseDouble(commaPos[2] + 1, commaPos[3]);
        values[3] = parseDouble(commaPos[3] + 1, end);
    } else {
        values[2] = parseDouble(commaPos[2] + 1, end);
        values[3] = 0.0;
    }
    
    return std::make_unique<OHLCBar>(timestamp, values[0], values[1], values[2], values[3], 0.0);
}

// Version optimisée de parseDouble qui évite digitValue()
inline double DataLoader::parseDouble(const QChar* begin, const QChar* end)
{
    double result = 0.0;
    bool negative = false;
    double fraction = 0.0;
    double divisor = 1.0;
    
    // Gestion du signe négatif
    if (begin < end && begin->unicode() == '-') {
        negative = true;
        ++begin;
    }
    
    // Partie entière - calcul direct avec unicode() plutôt que digitValue()
    while (begin < end && begin->unicode() >= '0' && begin->unicode() <= '9') {
        result = result * 10.0 + (begin->unicode() - '0');
        ++begin;
    }
    
    // Partie décimale
    if (begin < end && begin->unicode() == '.') {
        ++begin;
        while (begin < end && begin->unicode() >= '0' && begin->unicode() <= '9') {
            fraction = fraction * 10.0 + (begin->unicode() - '0');
            divisor *= 10.0;
            ++begin;
        }
        result += fraction / divisor;
    }
    
    return negative ? -result : result;
}