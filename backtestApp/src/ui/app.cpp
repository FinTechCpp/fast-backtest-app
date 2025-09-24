#include "ui/app.h"
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QTime>

#include "components/Managers/ProfileManager.h"
#include "components/Managers/BacktestResultManager.h"
#include "components/updateChecker.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
#include "ui/menu/profileMenuManager.h"
#include "ui/menu/dataMenuManager.h"
#include "ui/menu/updateMenuManager.h"
#include "ui/menu/BacktestResultMenuManager.h"
#include "ui/views/statsView.h"
#include "ui/views/chartView.h"
#include "ui/views/histogramView.h"
#include "components/Managers/resultManager.h"
#include "components/backtestRunner.h"


App::App() : QMainWindow() {
    // Set window properties
    setWindowTitle("Backtest Dashboard C++");
    resize(2100, 1300);
    
    // Initialize strategy map FIRST
    initStrategyMap();
    
    // Initialize configuration manager SECOND
    m_configManager = new ProfileManager(this);

    m_backtestResultManager = new BacktestResultManager(this, SerializationUtils::FileFormat::Binary);

    // Create main layout and central widget THIRD
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    
    // Create splitter for control panel and results
    m_splitter = new QSplitter(Qt::Horizontal);
    m_mainLayout->addWidget(m_splitter);

    createControlPanel();
    createResultsArea();
    setupConnections();

    updateStrategySpecificPanel();

    createActions();
    createMenus();
}

void App::createActions()
{
    // Help actions
    m_aboutAction = new QAction(tr("À &propos"), this);
    m_aboutAction->setStatusTip(tr("À propos de cette application"));
    connect(m_aboutAction, &QAction::triggered, this, &App::onAbout);
}

void App::createMenus()
{
    // Get the menu bar (QMainWindow creates one automatically)
    m_menuBar = menuBar();

    // Create the profile menu manager
    m_profileMenuManager = new ProfileMenuManager(this);
    m_profileMenuManager->setConfigManager(m_configManager);
    m_profileMenuManager->createProfileMenu(m_menuBar);

    // Create the data menu manager
    m_dataMenuManager = new DataMenuManager(this);
    m_dataMenuManager->createDataMenu(m_menuBar);

    // Create the update menu manager
    m_updateMenuManager = new UpdateMenuManager(this);
    m_updateMenuManager->createUpdateMenu(m_menuBar);

    // Create the results menu manager
    m_backtestResultMenuManager = new BacktestResultMenuManager(this);
    m_backtestResultMenuManager->setResultManager(m_backtestResultManager);
    m_backtestResultMenuManager->createResultMenu(m_menuBar);

    // Create Help menu (after the Profile menu)
    m_helpMenu = m_menuBar->addMenu(tr("&Aide"));
    m_helpMenu->addAction(m_aboutAction);
}

void App::onAbout()
{
    // Show an "About" dialog with application information
    QVersionNumber current = QVersionNumber::fromString(UpdateChecker::currentVersion());
    QMessageBox::about(this, tr("À propos"),
                       tr("Application de Backtest\n"
                          "Version %1\n"
                          "Développée par Hugo Miquel et Maxime Deville\n").arg(current.toString()));
}

App::~App()
{
    // Qt will handle deleting child widgets automatically
    qDebug() << "App destructor called";
}

void App::initStrategyMap()
{
    m_strategyMap["BuyHeikinGreenBA"] = "BuyHeikinGreenBA";
    m_strategyMap["SellHeikinRedBA"] = "SellHeikinRedBA";
}

void App::createControlPanel() {
    // Create the control panel widget with fixed width
    m_controlPanel = new QWidget();
    m_controlPanel->setMinimumWidth(100); // Minimum width
    m_controlPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_controlPanelLayout = new QVBoxLayout(m_controlPanel);
    
    // Backtest runner
    m_backtestRunner = new BacktestRunner(this);
    m_controlPanelLayout->addLayout(m_backtestRunner->getLayout());
    
    // General parameters panel
    m_generalParamsPanel = new GeneralParamsPanel(m_controlPanel);
    m_controlPanelLayout->addWidget(m_generalParamsPanel);
    
    // Strategy base panel
    m_strategyBasePanel = new StrategyBasePanel(m_controlPanel);
    m_controlPanelLayout->addWidget(m_strategyBasePanel);
    
    // Strategy-specific panel container
    m_strategyPanelStack = new QStackedWidget();
    m_controlPanelLayout->addWidget(m_strategyPanelStack);
    
    // Create all strategy-specific panels in advance
    m_buyHeikinGreenPanel = new BuyHeikinGreenPanel(m_controlPanel);
    m_sellHeikinRedPanel = new SellHeikinRedPanel(m_controlPanel);
    
    // Add all panels to the stack
    m_strategyPanelStack->addWidget(m_buyHeikinGreenPanel);
    m_strategyPanelStack->addWidget(m_sellHeikinRedPanel);
    
    // Initialize strategy-specific panels
    // updateStrategySpecificPanel();
    
    // Create scroll area and add to splitter
    m_controlPanelScrollArea = new QScrollArea();
    m_controlPanelScrollArea->setWidget(m_controlPanel);
    m_controlPanelScrollArea->setWidgetResizable(true);
    m_controlPanelScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_splitter->addWidget(m_controlPanelScrollArea);

    qInfo() << "Control panel created without ProfilePanel (now in the menu)";
}

void App::createResultsArea()
{    
    // Create the results manager by passing 'this' as the QObject parent
    m_resultManager = new ResultManager(this);
    
    // Add to splitter
    m_splitter->addWidget(m_resultManager);
    m_splitter->setSizes({200, 800}); // Control panel width : results width
}

void App::setupConnections()
{
    // Connect strategy change
    if (m_generalParamsPanel) {
        connect(m_generalParamsPanel, &GeneralParamsPanel::strategyChanged,
                this, &App::onStrategyChanged);
    }

    // Connect BacktestRunner
    if (m_backtestRunner) {
        connect(m_backtestRunner, &BacktestRunner::backtestCompleted,
                this, &App::onBacktestCompleted);
        connect(m_backtestRunner, &BacktestRunner::backtestError,
                this, &App::onBacktestError);
    }
}

void App::updateStrategySpecificPanel() {
    std::string selectedStrategy = m_generalParamsPanel->getConfig().strategyName;
    qInfo() << "Updating strategy-specific panel for:" << QString::fromStdString(selectedStrategy);

    // Select the appropriate panel in the stack
    if (selectedStrategy == "BuyHeikinGreenBA") {
        m_strategyPanelStack->setCurrentWidget(m_buyHeikinGreenPanel);
    } 
    else if (selectedStrategy == "SellHeikinRedBA") {
        m_strategyPanelStack->setCurrentWidget(m_sellHeikinRedPanel);
    }
}

void App::mousePressEvent(QMouseEvent *event)
{
    // Call the parent class implementation
    QMainWindow::mousePressEvent(event);
    
    // Emit signal to indicate resize might be starting
    emit windowResizeStarted();
}

void App::mouseReleaseEvent(QMouseEvent *event)
{
    // Call the parent class implementation
    QMainWindow::mouseReleaseEvent(event);
    
    // Emit signal to indicate resize is finished, with the current window size
    emit windowResizeFinished(size());
}

GeneralParamsConfig App::getGeneralParamsConfig() const {
    if (m_generalParamsPanel)
        return m_generalParamsPanel->getConfig();
    return GeneralParamsConfig();
}

StrategyBaseConfig App::getStrategyBaseConfig() const {
    if (m_strategyBasePanel)
        return m_strategyBasePanel->getConfig();
    return StrategyBaseConfig();
}

BuyHeikinGreenConfig App::getBuyHeikinGreenConfig() const {
    if (m_buyHeikinGreenPanel)
        return m_buyHeikinGreenPanel->getConfig();
    return BuyHeikinGreenConfig();
}

SellHeikinRedConfig App::getSellHeikinRedConfig() const {
    if (m_sellHeikinRedPanel)
        return m_sellHeikinRedPanel->getConfig();
    return SellHeikinRedConfig();
}

void App::setGeneralParamsConfig(const GeneralParamsConfig& config) {
    if (m_generalParamsPanel) {
        m_generalParamsPanel->setConfig(config);
    }
}

void App::setStrategyBaseConfig(const StrategyBaseConfig& config) {
    if (m_strategyBasePanel) {
        m_strategyBasePanel->setConfig(config);
    }
}

void App::setBuyHeikinGreenConfig(const BuyHeikinGreenConfig& config) {
    if (m_buyHeikinGreenPanel) {
        m_buyHeikinGreenPanel->setConfig(config);
    }
}

void App::setSellHeikinRedConfig(const SellHeikinRedConfig& config) {
    if (m_sellHeikinRedPanel) {
        m_sellHeikinRedPanel->setConfig(config);
    }
}

std::vector<std::unique_ptr<indicators::IndicatorBase>> App::readFromStrategyPanelToIndicatorInstances() const {
    std::vector<std::unique_ptr<indicators::IndicatorBase>> indicators;

    GeneralParamsConfig generalConfig = getGeneralParamsConfig();
    StrategyBaseConfig baseConfig = getStrategyBaseConfig();


    if (baseConfig.sl_method == StopLossMethod::ATR || baseConfig.tp_method == TakeProfitMethod::ATR || baseConfig.sl_method == StopLossMethod::MinMax) {
        auto atrInstance = std::make_unique<indicators::ATRInstance>();
        atrInstance->period = baseConfig.atr_period;
        atrInstance->useLogScale = true;
        indicators.push_back(std::move(atrInstance));
    }

    if (baseConfig.tp_method == TakeProfitMethod::SuperTrend) {
        auto superTrendInstance = std::make_unique<indicators::SuperTrendInstance>();
        superTrendInstance->period = baseConfig.tp_supertrend_atr_period;
        superTrendInstance->multiplier = baseConfig.tp_supertrend_multiplier;
        indicators.push_back(std::move(superTrendInstance));
    }

    QString strategyName = QString::fromStdString(generalConfig.strategyName);


    if (strategyName.contains("BuyHeikinGreen", Qt::CaseInsensitive)) {
        BuyHeikinGreenConfig config = getBuyHeikinGreenConfig();

        if (config.use_ema_short_filter) {
            auto emaShortInstance = std::make_unique<indicators::EMAInstance>();
            emaShortInstance->period = config.ema_short_period;
            indicators.push_back(std::move(emaShortInstance));
        }

        if (config.use_ema_long_filter) {
            auto emaLongInstance = std::make_unique<indicators::EMAInstance>();
            emaLongInstance->period = config.ema_long_period;
            indicators.push_back(std::move(emaLongInstance));
        }

        if (config.use_rsi_filter) {
            auto rsiInstance = std::make_unique<indicators::RSIInstance>();
            rsiInstance->period = config.rsi_period;
            rsiInstance->overboughtLevel = 70.0;  // Default value
            rsiInstance->oversoldLevel = config.rsi_threshold;
            indicators.push_back(std::move(rsiInstance));
        }

        if (config.use_stoch_filter) {
            auto stochInstance = std::make_unique<indicators::StochasticInstance>();
            stochInstance->fastKPeriod = config.stoch_fastk;
            stochInstance->slowKPeriod = config.stoch_slowk;
            stochInstance->slowDPeriod = config.stoch_slowd;
            stochInstance->overboughtLevel = 80.0;  // Default value
            stochInstance->oversoldLevel = config.stoch_threshold;
            indicators.push_back(std::move(stochInstance));
        }

        if (config.use_supertrend_filter) {
            auto superTrendInstance = std::make_unique<indicators::SuperTrendInstance>();
            superTrendInstance->period = config.supertrend_atr_period;
            superTrendInstance->multiplier = config.supertrend_multiplier;
            indicators.push_back(std::move(superTrendInstance));
        }

        if (config.use_atr_filter) {
            auto atrFilterInstance = std::make_unique<indicators::ATRInstance>();
            atrFilterInstance->period = config.atr_filter_period;
            atrFilterInstance->useLogScale = true;
            indicators.push_back(std::move(atrFilterInstance));
        }
    }
    else if (strategyName.contains("SellHeikinRed", Qt::CaseInsensitive)) {
        SellHeikinRedConfig config = getSellHeikinRedConfig();

        if (config.use_ema_short_filter) {
            auto emaShortInstance = std::make_unique<indicators::EMAInstance>();
            emaShortInstance->period = config.ema_short_period;
            indicators.push_back(std::move(emaShortInstance));
        }

        if (config.use_ema_long_filter) {
            auto emaLongInstance = std::make_unique<indicators::EMAInstance>();
            emaLongInstance->period = config.ema_long_period;
            indicators.push_back(std::move(emaLongInstance));
        }

        if (config.use_rsi_filter) {
            auto rsiInstance = std::make_unique<indicators::RSIInstance>();
            rsiInstance->period = config.rsi_period;
            rsiInstance->overboughtLevel = config.rsi_threshold;
            rsiInstance->oversoldLevel = 30.0;  // Default value
            indicators.push_back(std::move(rsiInstance));
        }

        if (config.use_stoch_filter) {
            auto stochInstance = std::make_unique<indicators::StochasticInstance>();
            stochInstance->fastKPeriod = config.stoch_fastk;
            stochInstance->slowKPeriod = config.stoch_slowk;
            stochInstance->slowDPeriod = config.stoch_slowd;
            stochInstance->overboughtLevel = config.stoch_threshold;
            stochInstance->oversoldLevel = 20.0;  // Default value
            indicators.push_back(std::move(stochInstance));
        }

        if (config.use_supertrend_filter) {
            auto superTrendInstance = std::make_unique<indicators::SuperTrendInstance>();
            superTrendInstance->period = config.supertrend_atr_period;
            superTrendInstance->multiplier = config.supertrend_multiplier;
            indicators.push_back(std::move(superTrendInstance));
        }

        if (config.use_atr_filter) {
            auto atrFilterInstance = std::make_unique<indicators::ATRInstance>();
            atrFilterInstance->period = config.atr_filter_period;
            atrFilterInstance->useLogScale = true;
            indicators.push_back(std::move(atrFilterInstance));
        }
    }

    // on supprime les doublons dans le vector indicators
    // auto end = std::unique(indicators.begin(), indicators.end(), [](const indicators::IndicatorBase& a, const indicators::IndicatorBase& b) {
    //     return a == b;
    // });
    // indicators.erase(end, indicators.end());

    return indicators;
}

void App::setBacktestResults(std::unique_ptr<BacktestResults> results) {
    m_backtestResults = std::move(results);

    if (m_resultManager) {
        m_resultManager->updateAllViews(m_backtestResults.get());
    }
}

// Slots implementation
void App::onStrategyChanged(const QString& strategy)
{
    Q_UNUSED(strategy)
    updateStrategySpecificPanel();
}

void App::onRunBacktest()
{
    // TODO: Implement backtest execution
    qInfo() << "Running backtest...";
}

void App::onBacktestCompleted()
{
    qInfo() << "Backtest completed successfully";
}

void App::onBacktestError(const QString& error)
{
    qCritical() << "Backtest error:" << error;
    QMessageBox::critical(this, "Erreur de backtest", error);
}

void App::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    
    qDebug() << "Window size:" << size().width() << "x" << size().height();
}