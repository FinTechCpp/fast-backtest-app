#include <QDebug>
#include <QApplication>
#include <sstream>
#include <filesystem>

#include "components/backtestRunner.h"
#include "ui/app.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/StrategyPanel.h"

#include "backtest.hpp"
#include "broker.hpp"
#include "components/SyntheticStrategy.hpp"

BacktestRunner::BacktestRunner(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<App*>(parent))
    , m_buttonLayout(nullptr)
    , m_runButton(nullptr)
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
    if (m_worker) {
        if (m_worker->isRunning()) {
            m_worker->quit();
            m_worker->wait();
        }
        delete m_worker;  // Explicit deletion
        m_worker = nullptr;
    }
}

void BacktestRunner::createUIComponents() {
    m_buttonLayout = new QHBoxLayout();
    
    m_runButton = new QPushButton("Run Backtest");
    m_runButton->setMinimumHeight(40);
    m_runButton->setStyleSheet("background-color: #4CAF50;"
                                "color: white;"
                                "font-weight: bold;");

    connect(m_runButton, &QPushButton::clicked, this, &BacktestRunner::runBacktest);
    
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
    
    m_runButton->setEnabled(false);
    m_runButton->setVisible(false);
    m_loadingIndicator->setVisible(true);
    m_statsLabel->setVisible(false); // Hide previous stats
    m_isRunning = true;
    
    emit backtestStarted();
    
    QApplication::processEvents();
    
    // Retrieve the configuration from the application
    if (!m_mainWindow) {
        showError("Reference to the main application not found");
        return;
    }
    
    // Create the worker and assign responsibility to it
    m_worker = new BacktestWorker(m_mainWindow, this);
    
    connect(m_worker, &BacktestWorker::finished, this, &BacktestRunner::onBacktestFinished);
    connect(m_worker, &BacktestWorker::error, this, &BacktestRunner::onBacktestError);
    connect(m_worker, &BacktestWorker::progressUpdated, this, &BacktestRunner::onProgressUpdated); 
    // connect(m_worker, &QThread::finished, m_worker, &QObject::deleteLater);
    m_worker->start();
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
        return;
    }
    
    qInfo() << "Backtest successfully completed, transmitting results";
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
}

void BacktestRunner::onBacktestError(const QString& errorMessage)
{
    resetUI();
    showError(errorMessage);
    emit backtestError(errorMessage);
}

void BacktestRunner::resetUI()
{
    m_runButton->setEnabled(true);
    m_runButton->setVisible(true);
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
    QElapsedTimer timer;
    timer.start();

    GeneralParamsConfig generalConfig = m_mainWindow->getGeneralParamsConfig();

    // Load data with DataLoader and convert to be::Data
    std::vector<OHLCBar> rawData = DataLoader::loadData(generalConfig.symbol, generalConfig.interval, generalConfig.period, generalConfig.endDate);

    if (rawData.empty()) {
        emit error("No data loaded");
        return;
    }
    // Convert to be::Data format
    std::shared_ptr<be::Data> data = convertToBeData(rawData);
        
    // Retrieve all strategy configurations
    std::vector<StrategyConfig> strategyConfigs = m_mainWindow->getStrategyConfigs();
    
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
            if (config.leverage_limit <= 0) {
                config.leverage_limit = generalParams.leverage_limit;
            }

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
    
    qDebug() << "Executing backtest...";
    m_results->stats = backtest.run();
    qDebug() << "Backtest successfully completed";
    
    // Emit the signal with the results
    emit finished(m_results.get());
        
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
