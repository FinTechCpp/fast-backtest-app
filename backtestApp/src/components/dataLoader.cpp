#include "components/dataLoader.h"
#include <QApplication>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QStringList>
#include <algorithm>
#include <cmath>

/*
Note: Market data from New York timezone is processed with a constant offset
instead of timezone conversion to avoid daylight saving time complications.
This ensures consistent data timeframes (e.g., 15:30-22:00) throughout the year.
You can adjust this offset if needed to match your local market hours.
*/
static const int CONSTANT_OFFSET_HOURS = 2;


// Declaration of the static variable for the cache
std::map<QString, std::vector<OHLCBar>> DataLoader::s_dataCache;

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
    // 1. FIRST STEP: Check if a custom path is set in QSettings
    QSettings settings("fast-backtest-app", "BacktestApp");
    QString customPath = settings.value("marketDataPath").toString();
    if (!customPath.isEmpty() && QDir(customPath).exists()) {
        qDebug() << "Using custom directory:" << customPath;
        return customPath;
    }

    // 2. Otherwise, continue with the standard search
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir currentDir(exeDir);

    // Go up the directory tree to find the fast-backtest-app folder
    do {
        QString currentPath = currentDir.absolutePath();

        // Check if this is the fast-backtest-app folder
        if (currentDir.dirName() == "fast-backtest-app") {
            QString marketDataPath = currentDir.absoluteFilePath("marketData");
            if (QFileInfo(marketDataPath).isDir()) {
                return marketDataPath;
            }
        }

        // Look for a subfolder fast-backtest-app
        QString igTradingBotPath = currentDir.absoluteFilePath("fast-backtest-app");
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
    // Check if the path exists or can be created
    QDir dir(path);
    if (!dir.exists()) {
        if (!QDir().mkpath(path)) {
            qWarning() << "Unable to create directory:" << path;
            return false;
        }
    }

    // Check write permissions
    QFileInfo dirInfo(path);
    if (!dirInfo.isWritable()) {
        qWarning() << "Directory is not writable:" << path;
        return false;
    }

    // Save the path in settings
    QSettings settings("fast-backtest-app", "BacktestApp");
    settings.setValue("marketDataPath", path);
    qInfo() << "Custom directory set:" << path;

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

    // Search for a file matching the pattern symbol_interval_*.csv
    QDir dir(marketDataDir);
    QStringList nameFilters;
    nameFilters << QString("%1_%2_*.csv").arg(symbol, interval);

    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Time);

    if (files.isEmpty()) {
        qWarning() << "No file found for pattern:"
                   << QString("%1_%2_*.csv").arg(symbol, interval)
                   << "in" << marketDataDir;
        return QString();
    }

    // Return the most recent file
    QString mostRecentFile = dir.absoluteFilePath(files.first());
    qDebug() << "File found:" << mostRecentFile;

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
        qWarning() << "Invalid target interval:" << targetInterval;
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
    qDebug() << "Resampling from" << data.size() << "to" << resampled.size()
             << "bars for interval" << targetInterval;
    return resampled;
}

std::vector<OHLCBar> DataLoader::loadData(
    const QString& symbol,
    const QString& interval,
    const QString& period,
    const QDateTime& endDate)
{
    QString cacheKey = makeCacheKey(symbol, interval, period, endDate);
    auto it = s_dataCache.find(cacheKey);
    if (it != s_dataCache.end()) {
        qDebug() << "Data loaded from cache for key:" << cacheKey;
        return it->second;
    }

    qDebug() << "DataLoader::loadData called with symbol:" << symbol
                << "interval:" << interval << "period:" << period;
    qDebug() << "End date:" << endDate.toString("dd/MM/yyyy");
    
    QString dataFile = findDataFile(symbol, interval);
    if (dataFile.isEmpty()) {
        qWarning() << "No data file found for" << symbol << interval;
        return std::vector<OHLCBar>();
    }

    qDebug() << "Data file found:" << dataFile;

    QDateTime actualEndDate = endDate.isValid() ? endDate : QDateTime::currentDateTime();
    QString endDateString = actualEndDate.toString("dd/MM/yyyy");

    std::vector<OHLCBar> result = loadFromCSV(dataFile, period, endDateString);

    // Store in cache
    s_dataCache[cacheKey] = result;
    return result;
}

QDateTime DataLoader::calculateStartDate(const QDateTime& endDate, const QString& period)
{
    QDateTime startDate = endDate;
    
    qDebug() << "Calcul de la date de début pour la période:" << period << "depuis:" << endDate.toString("dd/MM/yyyy hh:mm:ss");
    
    if (period.endsWith("d")) {
        // Daily periods
        bool ok;
        int days = period.first(period.length() - 1).toInt(&ok);
        if (ok && days > 0) {
            startDate = endDate.addDays(-days);
        }
    } else if (period.endsWith("w")) {
        // Weekly periods
        bool ok;
        int weeks = period.first(period.length() - 1).toInt(&ok);
        if (ok && weeks > 0) {
            startDate = endDate.addDays(-weeks * 7);
        }
    } else if (period.endsWith("m")) {
        // Monthly periods
        bool ok;
        int months = period.first(period.length() - 1).toInt(&ok);
        if (ok && months > 0) {
            startDate = endDate.addMonths(-months);
        }
    } else if (period.endsWith("y")) {
        // Yearly periods
        bool ok;
        int years = period.first(period.length() - 1).toInt(&ok);
        if (ok && years > 0) {
            startDate = endDate.addYears(-years);
        }
    } else {
        qWarning() << "Unrecognized period format:" << period;
        // Default to 10 days
        startDate = endDate.addDays(-10);
    }

    qDebug() << "Calculated start date:" << startDate.toString("dd/MM/yyyy hh:mm:ss");
    return startDate;
}

std::vector<OHLCBar> DataLoader::loadFromCSV(
    const QString& filePath,
    const QString& period,
    const QString& endDate)
{
    std::vector<OHLCBar> data;
    data.reserve(1000000); // Pre-allocate memory to avoid reallocations

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Unable to open file:" << filePath;
        return data;
    }
    
    QTextStream in(&file);
    QString line;

    qDebug() << "Loading data from:" << filePath;

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
            // This is the header, ignore it
        } else {
            // This is real data, parse it
            auto bar = parseCSVLine(line);
            if (bar && bar->timestamp >= startDateTime && bar->timestamp <= endDateTime) {
                data.push_back(*bar);
            }
        }
    }
    
    int lineCount = 1;

    while (in.readLineInto(&line)) {
        lineCount++;

        auto bar = parseCSVLine(line);
        if (bar) {
            // Filter data while loading
            if (bar->timestamp >= startDateTime && bar->timestamp <= endDateTime) {
                data.push_back(*bar);
            }
            else if (bar->timestamp > endDateTime) {
                qDebug() << "End of period reached at line" << lineCount << ", stopping loading";
                break;
            }
        }
    }

    file.close();
    qDebug() << "Loaded data:" << data.size() << "price bars from" << filePath;

    return data;
}

QString DataLoader::makeCacheKey(const QString& symbol, const QString& interval, const QString& period, const QDateTime& endDate) {
    return symbol + "|" + interval + "|" + period + "|" + endDate.toString(Qt::ISODate);
}

std::unique_ptr<OHLCBar> DataLoader::parseCSVLine(const QString& line)
{
    if (line.length() < 45) return nullptr;
    
    // Find the first comma to skip the index
    int firstComma = line.indexOf(',');
    if (firstComma == -1) return nullptr;
    
    // Ensure there are enough characters after the comma
    if (line.length() < firstComma + 25) return nullptr;
    
    // Pointer to the start of the date (after the index and the comma)
    const QChar* d = line.constData() + firstComma + 1;
    
    // Manually parse "2022-02-14 14:30:10+00:00" with direct calculation (avoids digitValue)
    int year = ((d[0].unicode() - '0') * 1000) + ((d[1].unicode() - '0') * 100) + 
               ((d[2].unicode() - '0') * 10) + (d[3].unicode() - '0');
    int month = ((d[5].unicode() - '0') * 10) + (d[6].unicode() - '0');
    int day = ((d[8].unicode() - '0') * 10) + (d[9].unicode() - '0');
    int hour = ((d[11].unicode() - '0') * 10) + (d[12].unicode() - '0');
    int minute = ((d[14].unicode() - '0') * 10) + (d[15].unicode() - '0');
    int second = ((d[17].unicode() - '0') * 10) + (d[18].unicode() - '0');
    
    // Quickly check if the values are within valid ranges before creating QDateTime
    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour > 23 || minute > 59 || second > 59) {
        return nullptr;
    }
    
    // Create the timestamp without timezone conversion, then apply a constant offset
    // to have the data between 15:30 and 22:00 (New York market hours)

    QDateTime timestamp(QDate(year, month, day), QTime(hour, minute, second), QTimeZone::UTC);

    // Apply a constant offset to simulate the New York -> desired local time shift
    timestamp = timestamp.addSecs(CONSTANT_OFFSET_HOURS * 3600);

    // Optimization: find all commas in a single pass
    const QChar* ptr = line.constData();
    const QChar* end = ptr + line.length();
    ptr += firstComma + 25;  // Skip the index and the date part

    // Find commas for OHLC data
    const QChar* commaPos[4] = {nullptr, nullptr, nullptr, nullptr};
    int commaCount = 0;
    
    while (ptr < end && commaCount < 4) {
        if (ptr->unicode() == ',') {
            commaPos[commaCount++] = ptr;
        }
        ++ptr;
    }

    if (commaCount < 3) return nullptr; // Not enough commas for OHLC data

    // Parse numeric values directly
    double values[4];  // open, high, low, close

    // Parse open
    values[0] = parseDouble(commaPos[0] + 1, commaPos[1]);

    // Parse high
    values[1] = parseDouble(commaPos[1] + 1, commaPos[2]);

    // Parse low and close
    if (commaCount == 4) {
        values[2] = parseDouble(commaPos[2] + 1, commaPos[3]);
        values[3] = parseDouble(commaPos[3] + 1, end);
    } else {
        values[2] = parseDouble(commaPos[2] + 1, end);
        values[3] = 0.0;
    }
    
    return std::make_unique<OHLCBar>(timestamp, values[0], values[1], values[2], values[3], 0.0);
}

// Optimized version of parseDouble that avoids digitValue()
inline double DataLoader::parseDouble(const QChar* begin, const QChar* end)
{
    double result = 0.0;
    bool negative = false;
    double fraction = 0.0;
    double divisor = 1.0;

    // Handle negative sign
    if (begin < end && begin->unicode() == '-') {
        negative = true;
        ++begin;
    }

    // Integer part - direct calculation with unicode() instead of digitValue()
    while (begin < end && begin->unicode() >= '0' && begin->unicode() <= '9') {
        result = result * 10.0 + (begin->unicode() - '0');
        ++begin;
    }

    // Decimal part
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