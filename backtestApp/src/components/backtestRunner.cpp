#include <QDebug>
#include <QApplication>
#include <sstream>
#include <filesystem>

#include "components/backtestRunner.h"
#include "ui/app.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/StrategyPanel.h"
#include "components/Utils/dataLoader.h"

#include "backtest.hpp"
#include "broker.hpp"
#include "components/SyntheticStrategy.hpp"
#include "Managers/LuaScriptEngine.hpp"

BacktestRunner::BacktestRunner(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<App*>(parent))
    , m_buttonLayout(nullptr)
    , m_runButton(nullptr)
    , m_stopButton(nullptr)
    , m_loadingIndicator(nullptr)
    , m_statsLabel(nullptr)
    , m_worker(nullptr)
    , m_isRunning(false)
    , m_lastTotalCandles(0)
    , m_lastChrono("")
{
    createUIComponents();
}

BacktestRunner::~BacktestRunner() {
    cleanupWorker();
}

void BacktestRunner::createUIComponents() {
    m_buttonLayout = new QHBoxLayout();
    
    m_runButton = new QPushButton("Run Backtest");
    m_runButton->setMinimumHeight(40);
    m_runButton->setStyleSheet("background-color: #4CAF50;"
                                "color: white;"
                                "font-weight: bold;");

    connect(m_runButton, &QPushButton::clicked, this, &BacktestRunner::runBacktest);

    m_stopButton = new QPushButton("Stop Backtest");
    m_stopButton->setMinimumHeight(40);
    m_stopButton->setStyleSheet("background-color: #f44336;"
                                "color: white;"
                                "font-weight: bold;");
    m_stopButton->setVisible(false);
    connect(m_stopButton, &QPushButton::clicked, this, &BacktestRunner::stopBacktest);
    
    m_loadingIndicator = new QProgressBar();
    m_loadingIndicator->setMaximum(100);
    m_loadingIndicator->setMinimum(0);
    m_loadingIndicator->setTextVisible(true);
    m_loadingIndicator->setVisible(false);
    m_loadingIndicator->setMinimumHeight(40); 
    m_loadingIndicator->setStyleSheet(
        "QProgressBar {"
        "   text-align: center;"
        "   font-size: 12px;"
        "   border: 1px solid grey;"
        "   border-radius: 2px;"
        "   padding: 2px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"
        "   border-radius: 2px;"
        "}"
    );
    
    // Create the label for execution statistics
    m_statsLabel = new QLabel();
    m_statsLabel->setVisible(false);
    m_statsLabel->setMinimumHeight(40);
    m_statsLabel->setStyleSheet("QLabel { background-color: #e8f4fd; color: #2c3e50; border: 1px solid #bdc3c7; border-radius: 4px; padding: 4px; margin: 2px 0px; font-size: 11px; }");
    m_statsLabel->setAlignment(Qt::AlignCenter);

    // Create a vertical layout for the button and stats
    QVBoxLayout* buttonStatsLayout = new QVBoxLayout();
    buttonStatsLayout->addWidget(m_runButton);
    buttonStatsLayout->addWidget(m_stopButton);
    buttonStatsLayout->addWidget(m_statsLabel);
    buttonStatsLayout->setSpacing(4); // Reduced spacing between the button and the label
    
    // Add the vertical layout to the main horizontal layout
    m_buttonLayout->addLayout(buttonStatsLayout);
    m_buttonLayout->addWidget(m_loadingIndicator);
}

void BacktestRunner::runBacktest() {
    if (m_isRunning) {
        return;
    }

    cleanupWorker();
    
    // Keep all caches (raw + filtered) for maximum performance between backtests
    // The cache will automatically handle different configurations via cache keys
    qInfo() << "=================================================";
    qInfo() << "STARTING NEW BACKTEST - Backtest runner initialized";
    qInfo() << "Thread ID:" << QThread::currentThreadId();
    qInfo() << "Using persistent cache for maximum performance";
    qInfo() << "=================================================";
    
    m_runButton->setEnabled(false);
    m_runButton->setVisible(false); // Hide the run button while backtest is running
    m_stopButton->setVisible(true);
    m_stopButton->setEnabled(true);
    m_stopButton->setText("Stop Backtest");
    m_loadingIndicator->setVisible(true);
    m_loadingIndicator->setValue(0);
    m_loadingIndicator->setFormat("Starting...");
    m_statsLabel->setVisible(false); // Hide previous stats
    m_isRunning = true;
    
    emit backtestStarted();
    
    QApplication::processEvents();
    
    // Retrieve the configuration from the application
    if (!m_mainWindow) {
        resetUI();
        showError("Reference to the main application not found");
        return;
    }
    
    // Create the worker and assign responsibility to it
    m_worker = new BacktestWorker(m_mainWindow, this);
    
    connect(m_worker, &BacktestWorker::finished, this, &BacktestRunner::onBacktestFinished);
    connect(m_worker, &BacktestWorker::error, this, &BacktestRunner::onBacktestError);
    connect(m_worker, &BacktestWorker::canceled, this, &BacktestRunner::onBacktestCanceled);
    connect(m_worker, &BacktestWorker::progressUpdated, this, &BacktestRunner::onProgressUpdated); 
    m_worker->start();
}

void BacktestRunner::stopBacktest() {
    if (!m_isRunning || !m_worker) {
        return;
    }

    m_stopButton->setEnabled(false);
    m_stopButton->setText("Stopping...");
    m_loadingIndicator->setFormat("Stopping...");
    m_worker->requestCancel();
}

void BacktestRunner::onProgressUpdated(int current, int total, const QString& chrono) {
    int percentage = (current * 100) / total;
    m_loadingIndicator->setValue(percentage);
    
    // Capture the latest statistics
    m_lastTotalCandles = total;
    m_lastChrono = chrono;
    
    // Calculate the speed in candles/second
    qint64 elapsedMs = QTime::fromString(chrono, "mm:ss.zz").msecsTo(QTime(0, 0, 0)) * -1;
    double candlesPerSecond = (current * 1000.0) / elapsedMs;
    // Use spaces for visual separation
    m_loadingIndicator->setFormat(QString("%1/%2 (%p%)  %3 c/s - %4")
                                .arg(current)
                                .arg(total)
                                .arg(QString::number(candlesPerSecond, 'f', 1))
                                .arg(chrono));
}

void BacktestRunner::onBacktestFinished(BacktestResults* results) {
    m_isRunning = false;
    resetUI();
    
    if (!results || results->candles.empty()) {
        qCritical() << "No backtest results received";
        showError("No backtest results received");
        cleanupWorker();
        // Keep cache even on error - it will help with debugging and next runs
        return;
    }
    
    qInfo() << "Backtest successfully completed, transmitting results";
    
    // Keep all caches (raw + filtered) for instant data loading in next backtest
    // Memory is managed automatically by the cache system
    qDebug() << "Size of received data:" << results->candles.size() << "bars";

    // Display execution statistics
    if (!m_lastChrono.isEmpty() && m_lastTotalCandles > 0) {
        qint64 elapsedMs = QTime::fromString(m_lastChrono, "mm:ss.zz").msecsTo(QTime(0, 0, 0)) * -1;
        double avgCandlesPerSecond = (m_lastTotalCandles * 1000.0) / elapsedMs;
        
        QString statsText = QString("📊 %1 candles • ⏱️ %2 • ⚡ %3 c/s")
                              .arg(m_lastTotalCandles)
                              .arg(m_lastChrono)
                              .arg(QString::number(avgCandlesPerSecond, 'f', 1));
        
        m_statsLabel->setText(statsText);
        m_statsLabel->setVisible(true);
    }
    
    // Transfer ownership of the results to the application
    if (m_mainWindow) {
        // Transfer ownership to the App
        m_mainWindow->setBacktestResults(m_worker->takeResults());
        emit backtestCompleted(m_mainWindow->getBacktestResults());
    } else {
        qCritical() << "Unable to transmit results: main window not available";
    }

    cleanupWorker();
}

void BacktestRunner::onBacktestError(const QString& errorMessage)
{
    resetUI();
    showError(errorMessage);
    emit backtestError(errorMessage);
    cleanupWorker();
}

void BacktestRunner::onBacktestCanceled()
{
    resetUI();
    m_statsLabel->setText("Backtest interrupted by user");
    m_statsLabel->setVisible(true);
    emit backtestCanceled();
    cleanupWorker();
}

void BacktestRunner::resetUI()
{
    m_runButton->setEnabled(true);
    m_runButton->setVisible(true); // Show the run button again
    m_stopButton->setVisible(false);
    m_stopButton->setEnabled(true);
    m_stopButton->setText("Stop Backtest");
    m_loadingIndicator->setVisible(false);
    m_isRunning = false;
}

void BacktestRunner::showError(const QString& error)
{
    qCritical() << "Backtest error:" << error;
}

QHBoxLayout* BacktestRunner::getLayout() const
{
    return m_buttonLayout;
}

void BacktestRunner::cleanupWorker() {
    if (!m_worker) {
        return;
    }

    if (m_worker->isRunning()) {
        m_worker->wait();
    }

    delete m_worker;
    m_worker = nullptr;
}

// Implementation of BacktestWorker with the C++ backtest
BacktestWorker::BacktestWorker(App* mainWindow, QObject* parent)
    : QThread(parent)
    , m_mainWindow(mainWindow)
    , m_results(nullptr)
{
}

BacktestWorker::~BacktestWorker()
{
    // The destructor automatically cleans up m_results
}

void BacktestWorker::run()
{
    qInfo() << "[WORKER] BacktestWorker::run() STARTED";
    qInfo() << "[WORKER] Worker thread ID:" << QThread::currentThreadId();

    auto cleanupLogging = []() {
        spdlog::drop("BE");
        spdlog::shutdown();
    };
    
    try {
        QElapsedTimer timer;
        timer.start();

        qInfo() << "[WORKER] Getting general params config...";
        GeneralParamsConfig generalConfig = m_mainWindow->getGeneralParamsConfig();
        qInfo() << "[WORKER] General params config retrieved successfully";

        // Load data with DataLoader and convert to be::Data
        qInfo() << "[WORKER] Loading data with DataLoader...";
        std::vector<OHLCBar> rawData = DataLoader::loadData(generalConfig.symbol, generalConfig.interval, generalConfig.period, generalConfig.endDate);
        qInfo() << "[WORKER] Data loaded, size:" << rawData.size();

        if (m_cancelRequested.load(std::memory_order_relaxed)) {
            cleanupLogging();
            emit canceled();
            return;
        }

        if (rawData.empty()) {
            qCritical() << "[WORKER] ERROR: No data loaded!";
            cleanupLogging();
            emit error("No data loaded");
            return;
        }

        // Convert to be::Data format
        std::shared_ptr<be::Data> data = convertToBeData(rawData);

        // Retrieve all strategy configurations
        std::vector<StrategyConfig> strategyConfigs = m_mainWindow->getStrategyConfigs();

        // Validate Lua scripts before running backtest
        for (size_t i = 0; i < strategyConfigs.size(); ++i) {
            if (strategyConfigs[i].use_lua_script && !strategyConfigs[i].lua_script.empty()) {
                std::string err_msg;
                if (!LuaScriptEngine::validate_script(strategyConfigs[i].lua_script, err_msg)) {
                    QString errorStr = QString("Error in Lua script for strategy %1:\n\n%2")
                                       .arg(i + 1)
                                       .arg(QString::fromStdString(err_msg));
                    qCritical() << "[WORKER] ERROR:" << errorStr;
                    cleanupLogging();
                    emit error(errorStr);
                    return;
                }
            }
        }

        if (m_cancelRequested.load(std::memory_order_relaxed)) {
            cleanupLogging();
            emit canceled();
            return;
        }

        // Create a BacktestResults object to store the results
        m_results = std::make_unique<BacktestResults>();
        m_results->candles = data->getCandles(); // Store candles
        m_results->generalConfig = generalConfig; // Store general backtest configuration
        m_results->strategyConfigs = strategyConfigs; // Store all strategy configurations

        qDebug() << "Available data:" << data->size() << "bars";
        qDebug() << "Number of strategies to execute:" << strategyConfigs.size();
        qDebug() << "Starting C++ backtest...";

        std::cout << generalConfig << std::endl;
        for (const auto& config : strategyConfigs) {
            std::cout << config << std::endl;
        }

        // Delete the existing log folder to start with a clean folder
        std::string logDirPath = "logs/backtestEngine";
#ifdef DISABLE_LOGGING
        if (std::filesystem::exists(logDirPath)) {
            std::filesystem::remove_all(logDirPath);
            qDebug() << "Old log folder deleted";
        }
#endif
        // Recreate the folder
        std::filesystem::create_directories(logDirPath);
        qDebug() << "New log folder created";

        // Create the logger
        std::string logFilePath = logDirPath + "/backtestExecution.log";
        spdlog::drop("BE"); // Ensure it doesn't already exist
        std::shared_ptr<spdlog::logger> async_file = spdlog::rotating_logger_mt<spdlog::async_factory>(
            "BE",       // Logger name
            logFilePath.c_str(),      // Log file path
            30 * 1024 * 1024,          // Max file size (30 MB)
            1
        );
        async_file->set_level(spdlog::level::debug);

        auto logCallback = [async_file](const std::string& msg) {
            async_file->log(spdlog::level::debug, msg);
        };

        // Create a vector of strategy factories - one factory per configuration
        std::vector<be::StrategyFactory> strategyFactories;
        strategyFactories.reserve(strategyConfigs.size());

        for (size_t i = 0; i < strategyConfigs.size(); ++i) {
            // Capture the config by value for each factory
            StrategyConfig config = strategyConfigs[i];

            auto strategyFactory = [this, config, async_file, i](
                std::shared_ptr<be::Broker> broker,
                std::shared_ptr<be::Data> data,
                std::function<void(const std::string&)> logCallback) mutable {

                GeneralParamsConfig generalParams = m_mainWindow->getGeneralParamsConfig();

                // Calculate the cash allocated to this strategy based on the allocation percentage
                double allocatedCash = generalParams.cash * (config.cash_allocation_percentage / 100.0);
                config.cash = allocatedCash;

                // If leverage is not defined, use the general configuration leverage
                if (config.leverage_limit <= 0)
                    config.leverage_limit = generalParams.leverage_limit;

                // Apply the reset indicators on new day setting from general params
                config.reset_indicators_on_new_day = generalParams.resetIndicatorsOnNewDay;

                qDebug() << "Creating strategy" << (i + 1)
                         << "- Allocated cash:" << allocatedCash
                         << "(" << config.cash_allocation_percentage << "% of" << generalParams.cash << ")"
                         << "- Leverage:" << config.leverage_limit;

                // Create the strategy with its specific configuration
                return std::make_shared<StrategyAdapter>(broker, data, config, logCallback);
            };

            strategyFactories.push_back(strategyFactory);
        }

        // Create and execute the backtest
        be::Backtest backtest(
            data,                             // Historical data
            strategyFactories,                // Vector of strategy factories
            generalConfig.cash,               // Initial capital
            generalConfig.spread,             // Spread
            generalConfig.commission,         // Commission
            generalConfig.leverage_limit,     // Leverage
            generalConfig.tradeOnClose,       // Trade on close
            generalConfig.positionMode,       // Position mode (Hedging or Netting)
            generalConfig.executeLimitOnLimitPrice, // Execute limit orders at limit price in a gap
            generalConfig.executeStopOnOpen,      // Execute stop orders at open in a gap
            generalConfig.finalizeTrades,     // Finalize trades
            generalConfig.spreadEntryRatio,   // Spread ratio for entry price
            generalConfig.minPositionStep,    // Minimum position size (quantization)
            logCallback                       // Logging function
        );

        backtest.setProgressCallback([this, &timer](size_t current, size_t total) {
            qint64 elapsed = timer.elapsed();
            QString chrono = QTime::fromMSecsSinceStartOfDay(elapsed).toString("mm:ss.zz");
            emit progressUpdated(static_cast<int>(current), static_cast<int>(total), chrono);
        });

        backtest.setCancelCallback([this]() {
            return m_cancelRequested.load(std::memory_order_relaxed);
        });

        qDebug() << "Executing backtest...";
        m_results->stats = backtest.run();

        if (m_cancelRequested.load(std::memory_order_relaxed)) {
            cleanupLogging();
            emit canceled();
            return;
        }

        qDebug() << "Backtest successfully completed";
        cleanupLogging();

        // Emit the signal with the results
        emit finished(m_results.get());
    } catch (const be::BacktestInterrupted&) {
        cleanupLogging();
        emit canceled();
    } catch (const std::exception& ex) {
        cleanupLogging();
        emit error(QString("Backtest failed: %1").arg(ex.what()));
    } catch (...) {
        cleanupLogging();
        emit error("Backtest failed: unknown error");
    }
        
}

std::shared_ptr<be::Data> BacktestWorker::convertToBeData(const std::vector<OHLCBar>& bars)
{
    // Pre-allocate memory to avoid reallocations
    size_t size = bars.size();
    std::vector<be::Date> dates;
    std::vector<double> open, high, low, close, volume;
    
    dates.reserve(size);
    open.reserve(size);
    high.reserve(size);
    low.reserve(size);
    close.reserve(size);
    volume.reserve(size);
    
    for (const auto& bar : bars) {
        // Convert the date from the OHLCBar structure to be::Date
        QDateTime dt = bar.timestamp;
        be::Date date(
            dt.date().year(),
            dt.date().month(),
            dt.date().day(),
            dt.time().hour(),
            dt.time().minute(),
            dt.time().second()
        );
        
        dates.push_back(date);
        open.push_back(bar.open);
        high.push_back(bar.high);
        low.push_back(bar.low);
        close.push_back(bar.close);
        volume.push_back(bar.volume);
    }
    
    // Detect gaps in the data (where the time between candles exceeds expected interval)
    std::vector<size_t> gapIndices;
    int expectedIntervalSecs = DataLoader::intervalToSeconds(m_mainWindow->getGeneralParamsConfig().interval);
    
    // Use 1.5x the expected interval as the threshold for gap detection
    int gapThreshold = expectedIntervalSecs * 1.5;
    
    for (size_t i = 0; i < bars.size() - 1; i++) {
        QDateTime current = bars[i].timestamp;
        QDateTime next = bars[i+1].timestamp;
        int secondsDiff = current.secsTo(next);
        
        // If the difference is significantly more than the expected interval, mark as a gap
        if (secondsDiff > gapThreshold) 
            gapIndices.push_back(i);   
    }
    
    // Create the Data object
    auto data = std::make_shared<be::Data>(dates, open, high, low, close, volume);
    
    // Set the gap indices
    data->setGapIndices(gapIndices);    
    return data;
}
