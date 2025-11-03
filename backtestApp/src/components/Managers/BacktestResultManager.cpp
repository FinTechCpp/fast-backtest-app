#include "components/Managers/BacktestResultManager.h"
#include "components/serializerAdapters.h"
#include "stats.hpp"
#include "data.hpp"
#include "trade.hpp"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QInputDialog>
#include <memory>
#include <algorithm>

BacktestResultManager::BacktestResultManager(QObject *parent, SerializationUtils::FileFormat defaultFormat)
    : QObject(parent), m_defaultFormat(defaultFormat)
{
    // Configure the results directory
    initializeDirectories();
    
    qInfo() << "BacktestResultManager successfully initialized. Directory:" << m_resultsDir;
}

BacktestResultManager::~BacktestResultManager()
{
    // Cleanup if necessary
}

void BacktestResultManager::initializeDirectories()
{
    // Use standard locations for application data
    m_resultsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backtest_results";

    // Create the directory if it does not exist
    QDir dir;
    if (!dir.exists(m_resultsDir)) {
        dir.mkpath(m_resultsDir);
        qInfo() << "Results directory created:" << m_resultsDir;
    }
}

QStringList BacktestResultManager::listBacktestResults() const
{
    QStringList results;
    
    // Scan the directory for JSON files
    QDir dir(m_resultsDir);
    // dir.setNameFilters(QStringList() << "*.json");
    dir.setFilter(QDir::Files);
    
    foreach (QString file, dir.entryList()) {
        QFileInfo fileInfo(m_resultsDir + "/" + file);
        QString baseName = fileInfo.completeBaseName(); // Name without extension
        
        results << baseName;
    }
    
    qDebug() << "Found results:" << results;
    return results;
}

bool BacktestResultManager::saveResult(const QString& resultName, const BacktestResultConfig& config, SerializationUtils::FileFormat format)
{
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;
    QString resultPath = getResultPath(resultName, actualFormat);
    
    return SerializationUtils::saveToFile(resultPath, config, actualFormat);
}

bool BacktestResultManager::loadResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format)
{
    if (format == SerializationUtils::FileFormat::Auto) {
        // Try all possible formats
        for (int i = 0; i < static_cast<int>(SerializationUtils::FileFormat::Auto); i++) {
            SerializationUtils::FileFormat currentFormat = static_cast<SerializationUtils::FileFormat>(i);
            QString resultPath = getResultPath(resultName, currentFormat);
            
            if (QFile::exists(resultPath)) {
                return SerializationUtils::loadFromFile(resultPath, config, currentFormat);
            }
        }
        return false;
    } else {
        QString resultPath = getResultPath(resultName, format);
        
        if (!QFile::exists(resultPath)) {
            return false;
        }
        
        return SerializationUtils::loadFromFile(resultPath, config, format);
    }
}

QString BacktestResultManager::getResultPath(const QString& resultName, SerializationUtils::FileFormat format) const
{
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;
    QString extension = SerializationUtils::getExtensionForFormat(actualFormat);
    
    return m_resultsDir + "/" + resultName + extension;
}

bool BacktestResultManager::resultExists(const QString& resultName, SerializationUtils::FileFormat format) const
{
    if (format == SerializationUtils::FileFormat::Auto) {
        // Check all possible formats
        for (int i = 0; i < static_cast<int>(SerializationUtils::FileFormat::Auto); i++) {
            SerializationUtils::FileFormat currentFormat = static_cast<SerializationUtils::FileFormat>(i);
            QString resultPath = getResultPath(resultName, currentFormat);
            if (QFile::exists(resultPath)) {
                return true;
            }
        }
        return false;
    } else {
        QString resultPath = getResultPath(resultName, format);
        return QFile::exists(resultPath);
    }
}

bool BacktestResultManager::openBacktestResultsDirectory() const
{
    QDir dir(m_resultsDir);
    if (!dir.exists()) {
        qWarning() << "The results directory does not exist:" << m_resultsDir;
        return false;
    }
    
    bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(m_resultsDir));
    if (success) {
        qInfo() << "Results directory opened:" << m_resultsDir;
    } else {
        qWarning() << "Failed to open the results directory:" << m_resultsDir;
    }
    
    return success;
}

bool BacktestResultManager::saveBacktestResult(const BacktestResultConfig& config, QWidget* parentWidget, SerializationUtils::FileFormat format)
{
    QString resultName = QString::fromStdString(config.name);
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;

    // Check if a result with this name already exists
    if (resultExists(resultName, actualFormat)) {
        if (parentWidget) {
            QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
                "Existing Result", 
                QString("A result named '%1' already exists. Do you want to replace it?").arg(resultName),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (choice != QMessageBox::Yes) {
                return false;
            }
        }
    }
    
    bool success = saveResult(resultName, config, actualFormat);
    
    if (success) {
        emit resultListUpdated();
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Success", 
                QString("Result '%1' saved.").arg(resultName));
        }
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Error", 
            QString("Failed to save the result '%1'.").arg(resultName));
    }
    
    return success;
}

bool BacktestResultManager::loadBacktestResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format)
{
    bool success = loadResult(resultName, config, format);
    
    if (!success) {
        qWarning() << "Failed to load the result:" << resultName;
    }
    
    return success;
}

bool BacktestResultManager::deleteBacktestResult(const QString& resultName, QWidget* parentWidget)
{
    // Check if the result exists in any format
    if (!resultExists(resultName, SerializationUtils::FileFormat::Auto)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Error", 
                QString("The result '%1' does not exist.").arg(resultName));
        }
        return false;
    }
    
    QMessageBox::StandardButton confirm = QMessageBox::Yes;
    
    if (parentWidget) {
        confirm = QMessageBox::question(parentWidget,
            "Confirmation",
            QString("Are you sure you want to delete the result '%1'?").arg(resultName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
    }
    
    if (confirm == QMessageBox::Yes) {
        bool success = true;
        
        // Delete the result in all formats where it exists
        for (int i = 0; i < static_cast<int>(SerializationUtils::FileFormat::Auto); i++) {
            SerializationUtils::FileFormat currentFormat = static_cast<SerializationUtils::FileFormat>(i);
            QString resultPath = getResultPath(resultName, currentFormat);
            
            if (QFile::exists(resultPath)) {
                bool deleteResult = QFile::remove(resultPath);
                success = success && deleteResult;
            }
        }
        
        if (success) {
            emit resultListUpdated();
            if (parentWidget) {
                QMessageBox::information(parentWidget, "Success", 
                    "Result deleted.");
            }
        } else if (parentWidget) {
            QMessageBox::warning(parentWidget, "Error", 
                "Failed to delete the result.");
        }
        
        return success;
    }
    
    return false;
}

bool BacktestResultManager::importBacktestResult(QWidget* parentWidget)
{
    // Build filter for all supported formats
    QString filterString = "All supported formats (*.json *.bin);;";
    filterString += "JSON files (*.json);;";
    filterString += "Binary files (*.bin)";
    
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
        "Import backtest result",
        QDir::homePath(),
        filterString);
    
    if (fileName.isEmpty()) {
        return false; // User canceled
    }
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Error", "The selected file does not exist.");
        }
        return false;
    }
    
    // Detect file format from its extension
    SerializationUtils::FileFormat detectedFormat = SerializationUtils::detectFormatFromExtension(fileName);
    
    // Try loading the result from the file
    BacktestResultConfig importedConfig;
    bool loadSuccess = SerializationUtils::loadFromFile(fileName, importedConfig, detectedFormat);
    
    if (!loadSuccess) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Import Error", 
                "The file is not a valid backtest result.");
        }
        return false;
    }
    
    // Ask for the result name
    bool ok;
    QString resultName = QString::fromStdString(importedConfig.name);
    
    if (parentWidget) {
        resultName = QInputDialog::getText(parentWidget, 
            "Result name", 
            "Enter a name for this backtest result:",
            QLineEdit::Normal,
            resultName, &ok);
        
        if (!ok || resultName.isEmpty()) {
            return false;
        }
    }
    
    // Update the name in the configuration
    importedConfig.name = resultName.toStdString();
    
    // Check if this name already exists
    if (resultExists(resultName, m_defaultFormat)) {
        QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
            "Existing Result", 
            QString("A result named '%1' already exists. Do you want to replace it?").arg(resultName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return false;
        }
    }
    
    // Save the imported result in the default format
    bool success = saveResult(resultName, importedConfig, m_defaultFormat);
    
    if (success) {
        emit resultListUpdated();
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Import successful", 
                QString("Result successfully imported as '%1'.").arg(resultName));
        }
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Import Error", 
            "Failed to import the result.");
    }
    
    return success;
}

bool BacktestResultManager::importExternalResult(QWidget* parentWidget)
{
    // Build filter for all supported formats
    QString filterString = "All supported formats (*.json *.bin);;";
    filterString += "JSON files (*.json);;";
    filterString += "Binary files (*.bin)";
    
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
        "Import external result (without precomputed stats)",
        QDir::homePath(),
        filterString);
    
    if (fileName.isEmpty()) 
        return false; // User canceled
    
    if (!QFile::exists(fileName)) {
        if (parentWidget) 
            QMessageBox::warning(parentWidget, "Error", "The selected file does not exist.");
        return false;
    }
    
    // Load the external result and compute stats
    BacktestResultConfig importedConfig;
    bool loadSuccess = loadExternalResult(fileName, importedConfig);
    
    if (!loadSuccess) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Import Error", 
                "The file is not a valid external result (expected ExternalResultConfig format).");
        }
        return false;
    }
    
    // Ask for the result name
    bool ok;
    QString resultName = QString::fromStdString(importedConfig.name);
    
    if (parentWidget) {
        resultName = QInputDialog::getText(parentWidget, 
            "Result name", 
            "Enter a name for this imported external result:",
            QLineEdit::Normal,
            resultName, &ok);
        
        if (!ok || resultName.isEmpty()) 
            return false;
    }
    
    // Update the name in the configuration
    importedConfig.name = resultName.toStdString();
    
    // Check if this name already exists
    if (resultExists(resultName, m_defaultFormat)) {
        QMessageBox::StandardButton choice = QMessageBox::question(parentWidget, 
            "Existing Result", 
            QString("A result named '%1' already exists. Do you want to replace it?").arg(resultName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) 
            return false;
    }
    
    // Save the imported result in the default format
    bool success = saveResult(resultName, importedConfig, m_defaultFormat);
    
    if (success) {
        emit resultListUpdated();
        if (parentWidget) {
            QString statsInfo = QString(
                "External result successfully imported as '%1'.\n\n"
                "Calculated statistics:\n"
                "- Number of trades: %2\n"
                "- Net Profit: %3 (%4%)\n"
                "- Profit Factor: %5")
                .arg(resultName)
                .arg(importedConfig.stats.numTrades)
                .arg(importedConfig.stats.equityFinal - importedConfig.stats.equityInitial, 0, 'f', 2)
                .arg(importedConfig.stats.returnPct, 0, 'f', 2)
                .arg(importedConfig.stats.profitFactor, 0, 'f', 2);
            
            QMessageBox::information(parentWidget, "Import successful", statsInfo);
        }
    } else if (parentWidget) {
        QMessageBox::warning(parentWidget, "Import Error", 
            "Failed to import external result.");
    }
    
    return success;
}

bool BacktestResultManager::exportBacktestResult(const QString& resultName, QWidget* parentWidget, SerializationUtils::FileFormat format)
{
    SerializationUtils::FileFormat actualFormat = (format == SerializationUtils::FileFormat::Auto) ? m_defaultFormat : format;
    
    // Verify that the result exists in any format
    if (!resultExists(resultName, SerializationUtils::FileFormat::Auto)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Error", 
                QString("The result '%1' does not exist.").arg(resultName));
        }
        return false;
    }
    
    // Load the result
    BacktestResultConfig config;
    if (!loadBacktestResult(resultName, config, SerializationUtils::FileFormat::Auto)) {
        if (parentWidget) {
            QMessageBox::warning(parentWidget, "Error", 
                QString("Unable to load the result '%1'.").arg(resultName));
        }
        return false;
    }
    
    // Default filename with result name and timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString extension = SerializationUtils::getExtensionForFormat(actualFormat);
    QString defaultFileName = QString("backtest_result_%1_%2%3").arg(resultName).arg(timestamp).arg(extension);
    
    // Build the filter for the specified format
    QString filterString;
    if (actualFormat == SerializationUtils::FileFormat::JSON) 
        filterString = "JSON files (*.json)";
    else if (actualFormat == SerializationUtils::FileFormat::Binary) 
        filterString = "Binary files (*.bin)";
    else 
        filterString = "All supported formats (*.json *.bin);;JSON files (*.json);;Binary files (*.bin)";
    
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
        QString("Export result - %1").arg(resultName),
        QDir::homePath() + "/" + defaultFileName,
        filterString);
    
    if (fileName.isEmpty()) 
        return false; // User canceled
    
    // If the user changed the extension, detect the new format
    SerializationUtils::FileFormat selectedFormat = actualFormat;
    if (format == SerializationUtils::FileFormat::Auto) 
        selectedFormat = SerializationUtils::detectFormatFromExtension(fileName);
    
    // Save the result to the selected file
    bool success = SerializationUtils::saveToFile(fileName, config, selectedFormat);
    
    if (success) {
        if (parentWidget) {
            QMessageBox::information(parentWidget, "Export successful", 
                QString("Result '%1' exported to:\n%2").arg(resultName).arg(fileName));
        }
    } else {
        if (parentWidget) {
            QMessageBox::critical(parentWidget, "Export Error", 
                "Error writing the result file.");
        }
    }
    
    return success;
}

bool BacktestResultManager::loadExternalResult(const QString& filePath, BacktestResultConfig& config)
{
    qDebug() << "Attempting to load external result from:" << filePath;
    
    ExternalResultConfig externalConfig;
    
    // Detect file format
    SerializationUtils::FileFormat format = SerializationUtils::FileFormat::JSON;
    if (filePath.endsWith(".bin")) 
        format = SerializationUtils::FileFormat::Binary;
    
    // Load the external file
    if (!SerializationUtils::loadFromFile(filePath, externalConfig, format)) {
        qCritical() << "Error loading external result";
        return false;
    }
    
    qInfo() << "External result loaded successfully. Computing stats...";
    
    // Convert ExternalResultConfig to BacktestResultConfig
    config.name = externalConfig.name;
    config.version = externalConfig.version;
    config.createdAt = externalConfig.createdAt;
    
    // External results do not have generalParams or strategyConfigs
    // Initialize with default values
    config.generalParams = GeneralParamsConfig(); // default values
    config.strategyConfigs.clear(); // no strategies
    config.candles = externalConfig.candles;
    
    // Validation
    if (externalConfig.candles.empty()) {
        qCritical() << "No candle data found in the external result";
        return false;
    }
    
    // Create a Data structure from the candles
    be::Data data(externalConfig.candles);
    
    // Build equity curve from trades
    std::vector<be::EquityPoint> equityCurve;
    // For an external result, use the default cash value (10000.0)
    double initialEquity = config.generalParams.cash;
    double currentEquity = initialEquity;
    
    // Initial point
    equityCurve.push_back({0, currentEquity});
    
    // Sort trades by exit order (exitBar)
    std::vector<be::TradeData> sortedTrades = externalConfig.trades;
    std::sort(sortedTrades.begin(), sortedTrades.end(),
              [](const be::TradeData& a, const be::TradeData& b) {
                  return a.exitBar < b.exitBar;
              });
    
    // Build equity curve by adding each trade's P&L
    for (const auto& trade : sortedTrades) {
        currentEquity += trade.pl;
        
        // Add an equity point at trade exit
        if (trade.exitBar < data.size()) 
            equityCurve.push_back({trade.exitBar, currentEquity});
    }
    
    // Add a final point if necessary
    if (equityCurve.back().index < data.size() - 1) 
        equityCurve.push_back({data.size() - 1, currentEquity});
    
    qInfo() << "Equity curve constructed with" << equityCurve.size() << "points";
    qInfo() << "Calculating statistics for" << sortedTrades.size() << "trades...";
    
    // Compute full statistics
    config.stats = be::computeStats(sortedTrades, equityCurve, data);
    
    qInfo() << "Statistics computed successfully";
    qInfo() << "- Number of trades:" << config.stats.numTrades;
    qInfo() << "- Final equity:" << config.stats.equityFinal;
    
    return true;
}