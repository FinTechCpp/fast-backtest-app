#include "components/Utils/dataLoader.h"
#include "components/Utils/apiClient.h"
#include "components/Utils/binarySerializer.h"
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
static const int CONSTANT_OFFSET_HOURS = 6;


// Declaration of the static variables for the cache
std::map<QString, std::vector<OHLCBar>> DataLoader::s_dataCache;
std::map<QString, std::vector<OHLCBar>> DataLoader::s_rawFileCache;

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
        qWarning() << "Impossible to open file:" << filePath;
        return info;
    }
    
    QTextStream in(&file);
    QString line;
    int lineNumber = 0;
    
    // Check if first line is header
    if (in.readLineInto(&line)) {
        lineNumber++;
        // Support both old format (date,open,high,low,close) and new format (timestamp,open,high,low,close,volume)
        if (line.startsWith("date,") || line.startsWith("timestamp,") || line.contains(",open,high,low,close")) {
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
                    info.invalidRowDetails.append(QString("Line %1: Invalid OHLC relationship").arg(lineNumber));
                } else {
                    info.isValid = true;
                }
            } else {
                info.invalidRows++;
                info.invalidRowDetails.append(QString("Line %1: Invalid format").arg(lineNumber));
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
                info.invalidRowDetails.append(QString("Line %1: Invalid OHLC relationship").arg(lineNumber));
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
                        QString("Line %1: Timestamp out of order (%2 after %3)")
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
            info.invalidRowDetails.append(QString("Line %1: Invalid format").arg(lineNumber));
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
    // PRIORITY 1: Check if a custom path is set in QSettings
    QSettings settings("fast-backtest-app", "BacktestApp");
    QString customPath = settings.value("marketDataPath").toString();
    
    if (!customPath.isEmpty()) {
        QDir customDir(customPath);
        if (customDir.exists()) {
            qDebug() << "Using custom directory from settings:" << customPath;
            return QDir::cleanPath(customPath);
        } else {
            qWarning() << "Custom directory no longer exists, removing from settings:" << customPath;
            settings.remove("marketDataPath");
        }
    }

    // PRIORITY 2: Check for marketData directory next to the executable
    QString exeDir = QCoreApplication::applicationDirPath();
    QString marketDataPath = QDir(exeDir).absoluteFilePath("marketData");
    marketDataPath = QDir::cleanPath(marketDataPath);
    
    QDir marketDataDir(marketDataPath);
    if (marketDataDir.exists()) {
        qDebug() << "Using marketData directory next to executable:" << marketDataPath;
        return marketDataPath;
    }

    // PRIORITY 3: Create marketData directory next to executable
    qDebug() << "marketData directory not found, creating:" << marketDataPath;
    if (QDir().mkpath(marketDataPath)) {
        qDebug() << "Successfully created marketData directory:" << marketDataPath;
        return marketDataPath;
    }
    
    qCritical() << "Failed to create marketData directory:" << marketDataPath;
    return QString(); // Not found and could not create
}

bool DataLoader::setCustomMarketDataDirectory(const QString& path)
{
    // Clean and normalize the path for consistent handling across platforms
    QString cleanPath = QDir::cleanPath(path);
    
    // Check if the path exists or can be created
    QDir dir(cleanPath);
    if (!dir.exists()) {
        if (!QDir().mkpath(cleanPath)) {
            qWarning() << "Unable to create directory:" << cleanPath;
            return false;
        }
    }

    // Check write permissions
    QFileInfo dirInfo(cleanPath);
    if (!dirInfo.isWritable()) {
        qWarning() << "Directory is not writable:" << cleanPath;
        return false;
    }

    // Save the path in settings (use native separators for the platform)
    QSettings settings("fast-backtest-app", "BacktestApp");
    settings.setValue("marketDataPath", QDir::toNativeSeparators(cleanPath));
    settings.sync(); // Force immediate write to disk
    qInfo() << "Custom directory set:" << cleanPath;

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
        qWarning() << "Market data directory not found";
        return QString();
    }

    QDir dir(marketDataDir);
    
    // Refresh the directory listing to avoid cache issues on Windows
    dir.refresh();
    
    // First, try to find exact match
    QStringList nameFilters;
    nameFilters << QString("%1_%2_*.csv").arg(symbol, interval);
    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Time);

    if (!files.isEmpty()) {
        QString exactMatch = QDir::cleanPath(dir.absoluteFilePath(files.first()));
        qDebug() << "Exact file match found:" << exactMatch;
        return exactMatch;
    }

    // If no exact match, find the best base file for resampling
    QString baseFile = findBestBaseDataFile(symbol, interval);
    if (!baseFile.isEmpty()) {
        qDebug() << "Base file found for resampling:" << baseFile;
        return QDir::cleanPath(baseFile);
    }

    qWarning() << "No suitable data file found for symbol:" << symbol << "interval:" << interval;
    return QString();
}

QString DataLoader::findBestBaseDataFile(const QString& symbol, const QString& interval)
{
    QString marketDataDir = findMarketDataDirectory();
    if (marketDataDir.isEmpty()) {
        qWarning() << "Market data directory not found";
        return QString();
    }

    QDir dir(marketDataDir);
    
    // Refresh the directory listing to avoid cache issues on Windows
    dir.refresh();
    
    int targetSeconds = intervalToSeconds(interval);
    
    if (targetSeconds <= 0) {
        qWarning() << "Invalid interval for base file search:" << interval;
        return QString();
    }

    // Strategy: if target < 60 seconds, use 10secs base; otherwise use 1min base
    QString baseInterval;
    if (targetSeconds < 60) {
        baseInterval = "10secs";
    } else {
        baseInterval = "1min";
    }

    // Search for base file
    QStringList nameFilters;
    nameFilters << QString("%1_%2_*.csv").arg(symbol, baseInterval);
    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Time);

    if (!files.isEmpty()) {
        QString baseFile = QDir::cleanPath(dir.absoluteFilePath(files.first()));
        qDebug() << "Selected base file:" << baseFile << "for target interval:" << interval;
        return baseFile;
    }

    // Fallback: try to find any file for this symbol
    nameFilters.clear();
    nameFilters << QString("%1_*.csv").arg(symbol);
    files = dir.entryList(nameFilters, QDir::Files, QDir::Time);
    
    if (!files.isEmpty()) {
        QString fallbackFile = QDir::cleanPath(dir.absoluteFilePath(files.first()));
        qWarning() << "Using fallback file:" << fallbackFile << "for interval:" << interval;
        return fallbackFile;
    }

    qWarning() << "No suitable base data file found for symbol:" << symbol;
    return QString();
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

std::vector<OHLCBar> DataLoader::duplicateFirstCandle(const std::vector<OHLCBar>& data, int targetIntervalSeconds)
{
    if (data.empty()) {
        return data;
    }

    // Only apply this for 10-second source data
    QString sourceInterval = extractIntervalFromFilename(findDataFile(QString(), QString()));
    bool is10SecData = sourceInterval.toLower() == "10secs";
    
    if (!is10SecData) {
        qDebug() << "Source data is not 10-second interval, skipping first candle duplication";
        return data;
    }

    std::vector<OHLCBar> result;
    result.reserve(data.size() + 100); // Reserve a bit more space for duplicates
    
    QDate currentDay;
    bool firstCandleOfDay = true;
    
    for (const auto& bar : data) {
        QDate barDate = bar.timestamp.date();
        
        // Check if this is a new day
        if (barDate != currentDay) {
            currentDay = barDate;
            firstCandleOfDay = true;
        }
        
        // If this is the first candle of the day and its time is offset (like 15:30:10)
        if (firstCandleOfDay) {
            QTime barTime = bar.timestamp.time();
            int seconds = barTime.second();
            
            // Check if the seconds value indicates it's not aligned to a perfect interval
            // For example, 10 seconds offset from the minute
            if (seconds > 0 && seconds < targetIntervalSeconds) {
                // Create a duplicate with adjusted timestamp
                QDateTime adjustedTime = bar.timestamp.addSecs(-seconds);
                qDebug() << "Day" << barDate.toString("yyyy-MM-dd") 
                         << "- Duplicating first candle at" << barTime.toString("hh:mm:ss")
                         << "to" << adjustedTime.time().toString("hh:mm:ss")
                         << "for proper interval alignment";
                
                // Add the duplicate with the adjusted timestamp
                result.push_back(OHLCBar(adjustedTime, bar.open, bar.high, bar.low, bar.close, bar.volume));
            }
            
            firstCandleOfDay = false;
        }
        
        // Add the original bar
        result.push_back(bar);
    }
    
    qInfo() << "Added" << (result.size() - data.size()) << "duplicate candles for interval alignment";
    return result;
}

std::vector<OHLCBar> DataLoader::resampleData(
    const std::vector<OHLCBar>& data,
    const QString& targetInterval)
{
    if (data.empty()) {
        qWarning() << "Cannot resample empty data";
        return data;
    }
    
    int targetSeconds = intervalToSeconds(targetInterval);
    if (targetSeconds <= 0) {
        qWarning() << "Invalid target interval:" << targetInterval;
        return data;
    }

    // STEP 1: Duplicate first candle to get the first period aligned
    std::vector<OHLCBar> cleanedData = duplicateFirstCandle(data, targetSeconds);

    if (cleanedData.empty()) {
        qWarning() << "No complete candles found after cleaning";
        return cleanedData;
    }
    
    // STEP 2: Proceed with normal resampling on cleaned data
    std::vector<OHLCBar> resampled;
    resampled.reserve(cleanedData.size() / (targetSeconds / 10) + 1); // Rough estimate for better memory allocation
    
    // Start with the first bar's timestamp aligned to the target interval
    QDateTime firstTimestamp = cleanedData[0].timestamp;
    QDateTime currentPeriodStart = alignToInterval(firstTimestamp, targetSeconds);
    QDateTime currentPeriodEnd = currentPeriodStart.addSecs(targetSeconds);
    
    double open = 0.0;
    double high = -std::numeric_limits<double>::max();
    double low = std::numeric_limits<double>::max();
    double close = 0.0;
    double volume = 0.0;
    bool hasData = false;
    
    qDebug() << "Starting resampling with cleaned data:" << cleanedData.size() << "bars";
    
    for (size_t i = 0; i < cleanedData.size(); ++i) {
        const auto& bar = cleanedData[i];
        
        // Skip bars that are before our current period
        if (bar.timestamp < currentPeriodStart) 
            continue;
        
        // Check if we need to move to the next period
        while (bar.timestamp >= currentPeriodEnd) {
            // Finalize current period if we have data
            if (hasData) 
                resampled.emplace_back(currentPeriodStart, open, high, low, close, volume);
            
            // Move to next period
            currentPeriodStart = currentPeriodEnd;
            currentPeriodEnd = currentPeriodStart.addSecs(targetSeconds);
            
            // Reset aggregation variables
            open = 0.0;
            high = -std::numeric_limits<double>::max();
            low = std::numeric_limits<double>::max();
            close = 0.0;
            volume = 0.0;
            hasData = false;
        }
        
        // Aggregate data for current period
        if (bar.timestamp >= currentPeriodStart && bar.timestamp < currentPeriodEnd) {
            if (!hasData) {
                // First bar in this period
                open = bar.open;
                hasData = true;
            }
            
            high = std::max(high, bar.high);
            low = std::min(low, bar.low);
            close = bar.close; // Always use the most recent close
            volume += bar.volume;
        }
    }
    
    // Don't forget the last period if it has data
    if (hasData) {
        resampled.emplace_back(currentPeriodStart, open, high, low, close, volume);
    }
    
    qInfo() << "Resampled from" << data.size() << "original bars (" << cleanedData.size() << "after cleaning) to" << resampled.size()
            << "bars for interval" << targetInterval
            << "(compression factor:" << QString::number(double(cleanedData.size()) / resampled.size(), 'f', 1) << ")";
    
    // Debug: Print first 10 bars of original data
    qDebug() << "=== ORIGINAL DATA (first 10 bars) ===";
    for (size_t i = 0; i < std::min(size_t(10), data.size()); ++i) {
        const auto& bar = data[i];
        qDebug() << QString("Original[%1]: %2 | O:%3 H:%4 L:%5 C:%6 V:%7")
                    .arg(i)
                    .arg(bar.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(bar.open, 0, 'f', 2)
                    .arg(bar.high, 0, 'f', 2)
                    .arg(bar.low, 0, 'f', 2)
                    .arg(bar.close, 0, 'f', 2)
                    .arg(bar.volume, 0, 'f', 0);
    }
    
    // Debug: Print first 10 bars of cleaned data
    qDebug() << "=== CLEANED DATA (first 10 bars) ===";
    for (size_t i = 0; i < std::min(size_t(10), cleanedData.size()); ++i) {
        const auto& bar = cleanedData[i];
        qDebug() << QString("Cleaned[%1]: %2 | O:%3 H:%4 L:%5 C:%6 V:%7")
                    .arg(i)
                    .arg(bar.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(bar.open, 0, 'f', 2)
                    .arg(bar.high, 0, 'f', 2)
                    .arg(bar.low, 0, 'f', 2)
                    .arg(bar.close, 0, 'f', 2)
                    .arg(bar.volume, 0, 'f', 0);
    }
    
    // Debug: Print first 10 bars of resampled data
    qDebug() << "=== RESAMPLED DATA (first 10 bars) ===";
    for (size_t i = 0; i < std::min(size_t(10), resampled.size()); ++i) {
        const auto& bar = resampled[i];
        qDebug() << QString("Resampled[%1]: %2 | O:%3 H:%4 L:%5 C:%6 V:%7")
                    .arg(i)
                    .arg(bar.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(bar.open, 0, 'f', 2)
                    .arg(bar.high, 0, 'f', 2)
                    .arg(bar.low, 0, 'f', 2)
                    .arg(bar.close, 0, 'f', 2)
                    .arg(bar.volume, 0, 'f', 0);
    }
    qDebug() << "=== END DEBUG OUTPUT ===";
    
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
        qInfo() << "⚡ INSTANT CACHE HIT - Data loaded from filtered cache (" 
                << it->second.size() << "bars) for" << symbol << interval << period;
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

    // Load raw data from CSV
    std::vector<OHLCBar> result = loadFromCSV(dataFile, period, endDateString);
    
    // Check if we need to resample the data
    QString fileInterval = extractIntervalFromFilename(dataFile);
    if (!fileInterval.isEmpty() && fileInterval != interval) {
        qInfo() << "Resampling data from" << fileInterval << "to" << interval;
        
        // Validate that target interval is compatible with source interval
        if (isValidResamplingInterval(fileInterval, interval)) {
            result = resampleData(result, interval);
            qInfo() << "Resampling completed. Final data size:" << result.size();
        } else {
            qWarning() << "Invalid resampling: cannot resample from" << fileInterval << "to" << interval;
            qWarning() << "Target interval must be a multiple of source interval";
        }
    }

    // Store in cache for instant access on next identical request
    s_dataCache[cacheKey] = result;
    qInfo() << "Cached filtered data (" << result.size() << "bars) - next identical backtest will be instant";
    return result;
}

std::vector<OHLCBar> DataLoader::loadDataFromApi(
    const QString& symbol,
    const QString& interval,
    const QString& period,
    const QDateTime& endDate,
    QString& errorMessage)
{
    errorMessage.clear();

    bool ok = false;
    QString apiError;
    QStringList files = ApiClient::listFiles(ok, apiError);
    if (!ok) {
        errorMessage = QString("Local data API unavailable: %1").arg(apiError);
        qCritical() << errorMessage;
        return {};
    }

    // Pick the best matching .bin file, mirroring findDataFile()'s CSV logic:
    // 1) exact "<symbol>_<interval>_*.bin" match
    // 2) best base file ("10secs" for sub-minute targets, "1min" otherwise)
    // 3) any file for the symbol
    QString chosenFile;
    QRegularExpression exactPattern(QString("^%1_%2_.*\\.bin$").arg(QRegularExpression::escape(symbol), QRegularExpression::escape(interval)));
    for (const QString& file : files) {
        if (exactPattern.match(file).hasMatch()) {
            chosenFile = file;
            break;
        }
    }

    if (chosenFile.isEmpty()) {
        int targetSeconds = intervalToSeconds(interval);
        QString baseInterval = (targetSeconds > 0 && targetSeconds < 60) ? "10secs" : "1min";
        QRegularExpression basePattern(QString("^%1_%2_.*\\.bin$").arg(QRegularExpression::escape(symbol), QRegularExpression::escape(baseInterval)));
        for (const QString& file : files) {
            if (basePattern.match(file).hasMatch()) {
                chosenFile = file;
                break;
            }
        }
    }

    if (chosenFile.isEmpty()) {
        QRegularExpression anyPattern(QString("^%1_.*\\.bin$").arg(QRegularExpression::escape(symbol)));
        for (const QString& file : files) {
            if (anyPattern.match(file).hasMatch()) {
                chosenFile = file;
                break;
            }
        }
    }

    if (chosenFile.isEmpty()) {
        errorMessage = QString("No binary file available on the local data API for symbol '%1'").arg(symbol);
        qWarning() << errorMessage;
        return {};
    }

    QString downloadError;
    QByteArray data = ApiClient::downloadFile(chosenFile, ok, downloadError);
    if (!ok) {
        errorMessage = QString("Failed to download '%1': %2").arg(chosenFile, downloadError);
        qCritical() << errorMessage;
        return {};
    }

    QString deserializeError;
    std::vector<OHLCBar> rawData = BinarySerializer::deserialize(data, deserializeError);
    if (!deserializeError.isEmpty()) {
        errorMessage = QString("Failed to deserialize '%1': %2").arg(chosenFile, deserializeError);
        qCritical() << errorMessage;
        return {};
    }

    qInfo() << "Loaded" << rawData.size() << "bars from local data API file:" << chosenFile;

    // Filter by period/endDate exactly like the CSV path
    QDateTime actualEndDate = endDate.isValid() ? endDate : QDateTime::currentDateTime();
    QDateTime startDateTime = calculateStartDate(actualEndDate, period);
    std::vector<OHLCBar> result = filterByPeriod(rawData, startDateTime, actualEndDate);

    // Resample if the source file's interval differs from the requested one
    QString fileInterval = extractIntervalFromFilename(chosenFile);
    if (!fileInterval.isEmpty() && fileInterval != interval) {
        if (isValidResamplingInterval(fileInterval, interval)) {
            result = resampleData(result, interval);
        } else {
            qWarning() << "Invalid resampling: cannot resample from" << fileInterval << "to" << interval;
        }
    }

    return result;
}

void DataLoader::clearCache()
{
    s_dataCache.clear();
    s_rawFileCache.clear();
    qInfo() << "Data cache cleared (filtered + raw)";
}

void DataLoader::clearFilteredCache()
{
    s_dataCache.clear();
    qInfo() << "Filtered data cache cleared (raw file cache kept for performance)";
}

QDateTime DataLoader::calculateStartDate(const QDateTime& endDate, const QString& period)
{
    QDateTime startDate = endDate;

    qDebug() << "Calculating start date for period:" << period << "from:" << endDate.toString("dd/MM/yyyy hh:mm:ss");

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

    // Clean the file path for cross-platform compatibility
    QString cleanFilePath = QDir::cleanPath(filePath);
    
    // Check if raw file data is already in cache
    auto rawCacheIt = s_rawFileCache.find(cleanFilePath);
    std::vector<OHLCBar> rawData;
    
    if (rawCacheIt != s_rawFileCache.end()) {
        qInfo() << "Raw file data loaded from cache (" << rawCacheIt->second.size() 
                << "bars) - avoiding CSV read:" << QFileInfo(cleanFilePath).fileName();
        rawData = rawCacheIt->second;
    } else {
        // Load raw data from file
        rawData.reserve(1000000); // Pre-allocate memory to avoid reallocations
        
        QFile file(cleanFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCritical() << "Unable to open file:" << cleanFilePath;
            qCritical() << "File error:" << file.errorString();
            return rawData;
        }
        
        QTextStream in(&file);
        QString line;
        
        qInfo() << "Reading CSV file from disk:" << QFileInfo(cleanFilePath).fileName();
        
        // Skip header if present
        if (in.readLineInto(&line)) {
            if (line.startsWith("date,") || line.startsWith("timestamp,")) {
                // This is the header, ignore it
            } else {
                // This is real data, parse it
                auto bar = parseCSVLine(line);
                if (bar) {
                    rawData.push_back(*bar);
                }
            }
        }
        
        int lineCount = 1;
        while (in.readLineInto(&line)) {
            lineCount++;
            auto bar = parseCSVLine(line);
            if (bar) {
                rawData.push_back(*bar);
            }
        }
        
        file.close();
        qInfo() << "Loaded and parsed" << rawData.size() << "bars from CSV";
        
        // Store raw data in cache
        s_rawFileCache[cleanFilePath] = rawData;
        qInfo() << "Raw file data cached for future backtests";
    }
    
    // Now filter the raw data based on period and endDate
    QDateTime endDateTime;
    if (endDate.isEmpty()) {
        endDateTime = QDateTime::currentDateTime();
    } else {
        QDate parsedDate = QDate::fromString(endDate, "dd/MM/yyyy");
        endDateTime = parsedDate.isValid() ? QDateTime(parsedDate, QTime(23, 59, 59)) : QDateTime::currentDateTime();
    }
    QDateTime startDateTime = calculateStartDate(endDateTime, period);
    
    std::vector<OHLCBar> filteredData;
    filteredData.reserve(rawData.size());
    
    for (const auto& bar : rawData) {
        if (bar.timestamp >= startDateTime && bar.timestamp <= endDateTime) {
            filteredData.push_back(bar);
        } else if (bar.timestamp > endDateTime) {
            break; // Data is sorted, no need to continue
        }
    }
    
    qDebug() << "🔍 Filtered to" << filteredData.size() << "bars for period" 
             << startDateTime.toString("dd/MM/yyyy") << "to" 
             << endDateTime.toString("dd/MM/yyyy");
    
    return filteredData;
}

QString DataLoader::makeCacheKey(const QString& symbol, const QString& interval, const QString& period, const QDateTime& endDate) {
    return symbol + "|" + interval + "|" + period + "|" + endDate.toString(Qt::ISODate);
}

QString DataLoader::extractIntervalFromFilename(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.baseName(); // Get filename without extension
    
    // Expected pattern: SYMBOL_INTERVAL_dates
    // e.g., "NDX_10secs_20220214_to_20250716_TRADES"
    QStringList parts = fileName.split('_');
    
    if (parts.size() >= 2) {
        QString possibleInterval = parts[1];
        
        // Validate that this looks like an interval
        if (possibleInterval.endsWith("secs") || 
            possibleInterval.endsWith("min") || 
            possibleInterval.endsWith("h")) {
            qDebug() << "Extracted interval:" << possibleInterval << "from file:" << fileName;
            return possibleInterval;
        }
    }
    
    qWarning() << "Could not extract interval from filename:" << fileName;
    return QString();
}

bool DataLoader::isValidResamplingInterval(const QString& sourceInterval, const QString& targetInterval)
{
    int sourceSeconds = intervalToSeconds(sourceInterval);
    int targetSeconds = intervalToSeconds(targetInterval);
    
    if (sourceSeconds <= 0 || targetSeconds <= 0) {
        qWarning() << "Invalid intervals for resampling:" << sourceInterval << "to" << targetInterval;
        return false;
    }
    
    if (targetSeconds < sourceSeconds) {
        qWarning() << "Cannot downsample: target interval" << targetInterval 
                   << "(" << targetSeconds << "s) is smaller than source interval" 
                   << sourceInterval << "(" << sourceSeconds << "s)";
        return false;
    }
    
    if (targetSeconds % sourceSeconds != 0) {
        qWarning() << "Target interval" << targetInterval << "(" << targetSeconds << "s)"
                   << "is not a multiple of source interval" << sourceInterval 
                   << "(" << sourceSeconds << "s)";
        return false;
    }
    
    qDebug() << "Valid resampling:" << sourceInterval << "to" << targetInterval
             << "(factor:" << (targetSeconds / sourceSeconds) << ")";
    return true;
}

QDateTime DataLoader::alignToInterval(const QDateTime& timestamp, int intervalSeconds)
{
    // Align timestamp to the start of the interval period
    qint64 epochSeconds = timestamp.toSecsSinceEpoch();
    qint64 alignedSeconds = (epochSeconds / intervalSeconds) * intervalSeconds;
    
    QDateTime aligned = QDateTime::fromSecsSinceEpoch(alignedSeconds, timestamp.timeZone());
    qDebug() << "Aligned timestamp" << timestamp.toString("yyyy-MM-dd hh:mm:ss")
             << "to interval start" << aligned.toString("yyyy-MM-dd hh:mm:ss")
             << "for" << intervalSeconds << "second intervals";
    
    return aligned;
}

std::unique_ptr<OHLCBar> DataLoader::parseCSVLine(const QString& line)
{
    // Minimum length for: "1234567890,1.0,2.0,3.0,4.0,0.0" = ~30 chars
    if (line.length() < 20) return nullptr;
    
    const QChar* ptr = line.constData();
    const QChar* end = ptr + line.length();
    
    // Find commas in a single pass
    const QChar* commaPos[5];
    int commaCount = 0;
    
    while (ptr < end && commaCount < 5) {
        if (ptr->unicode() == ',') 
            commaPos[commaCount++] = ptr;
        ++ptr;
    }
    
    // Need at least 4 commas: timestamp,open,high,low,close[,volume]
    if (commaCount < 4) return nullptr;
    
    // Parse Unix timestamp (much faster than date parsing!)
    qint64 unixTimestamp = parseUnixTimestamp(line.constData(), commaPos[0]);
    if (unixTimestamp <= 0) return nullptr;
    
    // Create QDateTime directly from Unix timestamp (optimized!)
    static QTimeZone utcZone = QTimeZone::utc();
    QDateTime timestamp = QDateTime::fromSecsSinceEpoch(unixTimestamp, utcZone);
    
    // Parse OHLC values
    double open = parseDouble(commaPos[0] + 1, commaPos[1]);
    double high = parseDouble(commaPos[1] + 1, commaPos[2]);
    double low = parseDouble(commaPos[2] + 1, commaPos[3]);
    
    // Handle close and optional volume
    double close, volume;
    if (commaCount >= 5) {
        close = parseDouble(commaPos[3] + 1, commaPos[4]);
        volume = parseDouble(commaPos[4] + 1, end);
    } else {
        close = parseDouble(commaPos[3] + 1, end);
        volume = 0.0;
    }
    
    return std::make_unique<OHLCBar>(timestamp, open, high, low, close, volume);
}

// Optimized parser for Unix timestamp
inline qint64 DataLoader::parseUnixTimestamp(const QChar* begin, const QChar* end)
{
    qint64 result = 0;
    
    // Parse integer directly (Unix timestamps are always positive integers)
    while (begin < end && begin->unicode() >= '0' && begin->unicode() <= '9') {
        result = result * 10 + (begin->unicode() - '0');
        ++begin;
    }
    
    return result;
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