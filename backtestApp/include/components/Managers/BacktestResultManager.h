#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include "components/Utils/SerializationUtils.hpp"

/**
 * @brief Manager to save and load backtest results
 */
class BacktestResultManager : public QObject
{
    Q_OBJECT

public:
    BacktestResultManager(QObject* parent = nullptr, SerializationUtils::FileFormat defaultFormat = SerializationUtils::FileFormat::JSON);
    ~BacktestResultManager();
    
    // List all available backtest results
    QStringList listBacktestResults() const;

    // User actions with optional format
    bool saveBacktestResult(const BacktestResultConfig& config, QWidget* parentWidget = nullptr, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    bool loadBacktestResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    bool deleteBacktestResult(const QString& resultName, QWidget* parentWidget = nullptr);
    bool importBacktestResult(QWidget* parentWidget = nullptr);
    bool importExternalResult(QWidget* parentWidget = nullptr);  // Import external results
    bool exportBacktestResult(const QString& resultName, QWidget* parentWidget = nullptr, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    
    // Method to load an external result and compute statistics
    bool loadExternalResult(const QString& filePath, BacktestResultConfig& config);

    // Method to set the default format
    void setDefaultFormat(SerializationUtils::FileFormat format) { m_defaultFormat = format; }
    SerializationUtils::FileFormat getDefaultFormat() const { return m_defaultFormat; }

    // Method to open the results directory
    bool openBacktestResultsDirectory() const;
    
    // Getter for the results directory path
    QString getBacktestResultsDirectory() const { return m_resultsDir; }

signals:
    void resultListUpdated();

private:
    QString m_resultsDir;         // Directory where results are stored
    SerializationUtils::FileFormat m_defaultFormat;

    // File management methods
    bool resultExists(const QString& resultName, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto) const;
    bool saveResult(const QString& resultName, const BacktestResultConfig& config, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    bool loadResult(const QString& resultName, BacktestResultConfig& config, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto);
    QString getResultPath(const QString& resultName, SerializationUtils::FileFormat format = SerializationUtils::FileFormat::Auto) const;

    // Initialize application directories
    void initializeDirectories();
};