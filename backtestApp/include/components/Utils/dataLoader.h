#pragma once

#include <QString>
#include <QDateTime>
#include <QTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QList>
#include <QPair>
#include <vector>
#include <map>
#include <memory>
#include <limits>

/**
 * @brief Structure to store an OHLC (Open-High-Low-Close) bar
 */
struct OHLCBar {
    QDateTime timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume = 0.0;  
    
    OHLCBar() = default;
    OHLCBar(const QDateTime& dt, double o, double h, double l, double c, double v = 0.0)
        : timestamp(dt), open(o), high(h), low(l), close(c), volume(v) {}
};

Q_DECLARE_METATYPE(OHLCBar)
Q_DECLARE_METATYPE(std::vector<OHLCBar>)

/**
 * @brief Structure to store validation information for a data file
 */
struct DataFileInfo {
    // Basic file info
    QString filePath;
    QString fileName;
    qint64 fileSize;
    
    // Data metrics
    int totalRows;
    QDateTime startDate;
    QDateTime endDate;
    int durationDays;
    QString interval; // Detected interval between data points (e.g., "10secs")
    
    // Integrity checks
    bool hasHeader;
    bool isValid;
    int invalidRows;
    QStringList invalidRowDetails; // Contains reasons for invalid rows
    int gapsCount;
    QList<QPair<QDateTime, QDateTime>> largestGaps; // Stores significant gaps
    
    // Price stats
    double minPrice;
    double maxPrice;
    
    // Constructor with defaults
    DataFileInfo() : 
        fileSize(0), 
        totalRows(0), 
        durationDays(0), 
        hasHeader(false), 
        isValid(false), 
        invalidRows(0), 
        gapsCount(0),
        minPrice(std::numeric_limits<double>::max()),
        maxPrice(std::numeric_limits<double>::lowest()) {}
};

Q_DECLARE_METATYPE(DataFileInfo)

/**
 * @brief Class for loading and processing market data
 */
class DataLoader
{
public:
    DataLoader();
    ~DataLoader();
    
    /**
     * @brief Find the marketData directory in the user's home directory
     * @return Path to the marketData directory, empty QString if not found
     */
    static QString findMarketDataDirectory();
    
    /**
     * @brief Load OHLC data from a CSV file
     * @param symbol Market symbol (e.g., "NDX")
     * @param interval Data interval (e.g., "20secs")
     * @param period Data period to load (e.g., "10d", "1m", "6m", "1y")
     * @param endDate End date in QDateTime format
     * @return Vector of filtered OHLC bars
     */
    static std::vector<OHLCBar> loadData(
        const QString& symbol,
        const QString& interval, 
        const QString& period,
        const QDateTime& endDate = QDateTime()
    );

    static std::vector<OHLCBar> loadData(
        const std::string& symbol,
        const std::string& interval,
        const std::string& period,
        const QDateTime& endDate = QDateTime()
    ) {
        return loadData(QString::fromStdString(symbol), QString::fromStdString(interval), QString::fromStdString(period), endDate);
    }
    
    /**
     * @brief Find the CSV file corresponding to the symbol and interval
     * @param symbol Symbol to search for
     * @param interval Interval to search for
     * @return Path to the found file, empty QString if not found
     */
    static QString findDataFile(const QString& symbol, const QString& interval);
    static QString findDataFile(const std::string& symbol, const std::string& interval) {
        return findDataFile(QString::fromStdString(symbol), QString::fromStdString(interval));
    }
    
    /**
     * @brief Find the best base data file for resampling to target interval
     * @param symbol Symbol to search for
     * @param interval Target interval to determine the best base file
     * @return Path to the best base file, empty QString if not found
     */
    static QString findBestBaseDataFile(const QString& symbol, const QString& interval);
    static QString findBestBaseDataFile(const std::string& symbol, const std::string& interval) {
        return findBestBaseDataFile(QString::fromStdString(symbol), QString::fromStdString(interval));
    }
    
    /**
     * @brief Parse a CSV line into an OHLC bar
     * @param line CSV line to parse
     * @param hasHeader Indicates if this is the header line
     * @return Parsed OHLC bar, nullptr if error
     */
    static std::unique_ptr<OHLCBar> parseCSVLine(const QString& line);
    
    /**
     * @brief Load data from a CSV file
     * @param filePath Path to the CSV file
     * @param period PPeriod to load
     * @param endDate End date
     * @return Vector of OHLC bars
     */
    static std::vector<OHLCBar> loadFromCSV(
        const QString& filePath,
        const QString& period,
        const QString& endDate
    );
    
    /**
     * @brief Filter data by period
     * @param data Data to filter
     * @param startDate Start date
     * @param endDate End date
     * @return Filtered data
     */
    static std::vector<OHLCBar> filterByPeriod(
        const std::vector<OHLCBar>& data,
        const QDateTime& startDate,
        const QDateTime& endDate
    );
    
    /**
     * @brief Resample data to a target interval
     * @param data Source data
     * @param targetInterval Target interval (e.g., "1min", "5min")
     * @return Resampled data
     */
    static std::vector<OHLCBar> resampleData(
        const std::vector<OHLCBar>& data,
        const QString& targetInterval
    );
    
    /**
     * @brief Calculate the start date from the period
     * @param endDate End date
     * @param period Period (e.g., "10d", "1m", "6m", "1y")
     * @return Calculated start date
     */
    static QDateTime calculateStartDate(const QDateTime& endDate, const QString& period);
    
    /**
     * @brief Convert an interval to seconds
     * @param interval Interval (e.g., "10secs", "1min", "1h")
     * @return Number of seconds
     */
    static int intervalToSeconds(const QString& interval);
    static int intervalToSeconds(const std::string& interval) {
        return intervalToSeconds(QString::fromStdString(interval));
    }
    
    /**
     * @brief Set a custom directory for market data
     * @param path Path to the directory to use
     * @return true if the directory is valid and has been set, false otherwise
     */
    static bool setCustomMarketDataDirectory(const QString& path);
    
    /**
     * @brief Check the integrity of OHLC CSV data
     * @param filePath Path to the file to check
     * @return Structure containing information about the file and its integrity
     */
    static DataFileInfo checkDataFile(const QString& filePath);
    
    /**
     * @brief Clear the data cache (useful to free memory and avoid stale data)
     */
    static void clearCache();

private:
    static QStringList getMarketDataPaths();
    static std::map<QString, std::vector<OHLCBar>> s_dataCache; // Cache for loaded data
    static QString makeCacheKey(const QString& symbol, const QString& interval, const QString& period, const QDateTime& endDate);

    // Helper method for double conversion
    static inline double parseDouble(const QChar* begin, const QChar* end);
    
    // Helper method for Unix timestamp conversion
    static inline qint64 parseUnixTimestamp(const QChar* begin, const QChar* end);
    
    // Helper methods for resampling
    static QString extractIntervalFromFilename(const QString& filePath);
    static bool isValidResamplingInterval(const QString& sourceInterval, const QString& targetInterval);
    static QDateTime alignToInterval(const QDateTime& timestamp, int intervalSeconds);
    
    // New helper method for duplicating first candles of each day
    static std::vector<OHLCBar> duplicateFirstCandle(const std::vector<OHLCBar>& data, int targetIntervalSeconds);
};

