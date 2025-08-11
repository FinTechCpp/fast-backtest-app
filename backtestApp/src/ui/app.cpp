#include "ui/app.h"
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QTime>

#include "components/Managers/ProfileManager.h"
#include "components/updateChecker.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/strategySpecificPanels/strategyBasePanel.h"
#include "ui/menu/profileMenuManager.h"
#include "ui/menu/dataMenuManager.h"
#include "ui/menu/updateMenuManager.h"
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

    // Créer le gestionnaire de menu des profils
    m_profileMenuManager = new ProfileMenuManager(this);
    m_profileMenuManager->setConfigManager(m_configManager);
    m_profileMenuManager->createProfileMenu(m_menuBar);

    // Créer le gestionnaire de menu des données
    m_dataMenuManager = new DataMenuManager(this);
    m_dataMenuManager->createDataMenu(m_menuBar);

    // Créer le gestionnaire de menu des mises à jour
    m_updateMenuManager = new UpdateMenuManager(this);
    m_updateMenuManager->createUpdateMenu(m_menuBar);

    // Create Help menu (après le menu Profils)
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
    m_controlPanel->setMinimumWidth(100); // Largeur minimale
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
    
    // Créer tous les panels de stratégies spécifiques à l'avance
    m_buyHeikinGreenPanel = new BuyHeikinGreenPanel(m_controlPanel);
    m_sellHeikinRedPanel = new SellHeikinRedPanel(m_controlPanel);
    
    // Ajouter tous les panels au stack
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

    qInfo() << "Panneau de contrôle créé sans ProfilePanel (maintenant dans le menu)";
}

void App::createResultsArea()
{    
    // Créer le gestionnaire de résultats en passant 'this' comme QObject parent
    m_resultManager = new ResultManager(this);
    
    // Ajouter au splitter
    m_splitter->addWidget(m_resultManager);
    m_splitter->setSizes({200, 800}); // Largeur du panneau de contrôle : largeur des résultats
}

void App::setupConnections()
{
    // Connexion du changement de stratégie
    if (m_generalParamsPanel) {
        connect(m_generalParamsPanel, &GeneralParamsPanel::strategyChanged,
                this, &App::onStrategyChanged);
    }

    // Connexion du BacktestRunner
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

    // Sélectionner le panel approprié dans le stack
    if (selectedStrategy == "BuyHeikinGreenBA") {
        m_strategyPanelStack->setCurrentWidget(m_buyHeikinGreenPanel);
    } 
    else if (selectedStrategy == "SellHeikinRedBA") {
        m_strategyPanelStack->setCurrentWidget(m_sellHeikinRedPanel);
    }
    
    // // Remove old panel if it exists
    // if (m_strategySpecificPanel) {
    //     // CORRECTION: Utiliser une méthode correcte pour récupérer le widget
    //     // Chercher le widget dans le stack ou le layout
    //     for (int i = 0; i < m_strategyPanelStack->count(); ++i) {
    //         QWidget* widget = m_strategyPanelStack->widget(i);
    //         m_strategyPanelStack->removeWidget(widget);
    //         widget->deleteLater();
    //     }
    //     m_strategySpecificPanel = nullptr;
    // }
    
    // // Create new panel based on strategy
    // QWidget* strategyWidget = createStrategySpecificPanel(selectedStrategy);
    // if (strategyWidget) {
    //     m_strategyPanelStack->addWidget(strategyWidget);
    //     m_strategyPanelStack->setCurrentWidget(strategyWidget);
    // }
    
    // // Load current profile values
    // QMap<QString, QVariant> currentProfile = m_configManager->getProfile(m_configManager->getCurrentProfile());
    // if (m_strategySpecificPanel && !currentProfile.isEmpty()) {
    //     m_strategySpecificPanel->setValues(currentProfile);
    // }
}

// QWidget* App::createStrategySpecificPanel(const QString& strategy)
// {
//     BasePanel* panel = nullptr;
    
//     if (strategy == "BuyHeikinGreenBA") panel = new BuyHeikinGreenPanel(m_controlPanel);
//     else if (strategy == "SellHeikinRedBA") panel = new SellHeikinRedPanel(m_controlPanel);

//     if (panel) {
//         m_strategySpecificPanel = panel;
//         panel->initialize();
//         return panel;
//     }
    
//     return nullptr;
// }

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

// QMap<QString, QVariant> App::getStrategyConfig() const
// {
//     QMap<QString, QVariant> config;
    
//     // Récupérer depuis le panel général
//     if (m_generalParamsPanel) {
//         QMap<QString, QVariant> generalValues = m_generalParamsPanel->getValues();
//         config.insert(generalValues);  // Qt 5.15+
//     }
    
//     // Récupérer depuis le panel de base
//     if (m_strategyBasePanel) {
//         QMap<QString, QVariant> baseValues = m_strategyBasePanel->getValues();
//         config.insert(baseValues);  // Qt 5.15+
//     }
    
//     // Récupérer depuis le panel spécifique à la stratégie
//     if (m_strategySpecificPanel) {
//         QMap<QString, QVariant> specificValues = m_strategySpecificPanel->getValues();
//         config.insert(specificValues);  // Qt 5.15+
//     }
    
//     // Debug pour vérifier
//     qDebug() << "Config récupérée:" << config;
    
//     return config;
// }

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

std::vector<StrategyIndicator> App::getIndicatorConfig() const
{
    // Obtenir les paramètres de stratégie
    // QMap<QString, QVariant> params = this->getStrategyConfig();
    // QMap<QString, QVariant> generalValues = m_generalParamsPanel->getValues();
    GeneralParamsConfig generalConfig = this->getGeneralParamsConfig();
    StrategyBaseConfig baseConfig = this->getStrategyBaseConfig();
    std::vector<StrategyIndicator> indicators;
    
    // Extraire ATR si utilisé pour SL ou TP
    // bool use_atr_for_sl = params.value("use_atr_for_sl", false).toBool();
    // bool use_atr_for_tp = params.value("use_atr_for_tp", false).toBool();

    if (baseConfig.sl_method == StopLossMethod::ATR || baseConfig.tp_method == TakeProfitMethod::ATR) {
        StrategyIndicator atr;
        atr.type = StrategyIndicator::ATR;
        atr.params["period"] = baseConfig.atr_period;
        atr.params["useLogScale"] = 1.0;  // true par défaut
        indicators.push_back(atr);
    }
    
    // Extraire les indicateurs spécifiques à la stratégie
    // QString strategyName = generalValues.value("strategy", "").toString();
    QString strategyName = QString::fromStdString(generalConfig.strategyName);
    
    if (strategyName.contains("BuyHeikinGreen", Qt::CaseInsensitive)) {
        // QMap<QString, QVariant> specificValues = m_strategySpecificPanel->getValues();
        BuyHeikinGreenConfig config = getBuyHeikinGreenConfig();

        // EMA court terme
        bool use_ema_short = config.use_ema_short_filter;
        if (use_ema_short) {
            StrategyIndicator ema;
            ema.type = StrategyIndicator::EMA;
            ema.params["period"] = config.ema_short_period;
            indicators.push_back(ema);
        }
        
        // EMA long terme
        bool use_ema_long = config.use_ema_long_filter;
        if (use_ema_long) {
            StrategyIndicator ema;
            ema.type = StrategyIndicator::EMA;
            ema.params["period"] = config.ema_long_period;
            indicators.push_back(ema);
        }
        
        // RSI
        bool use_rsi = config.use_rsi_filter;
        if (use_rsi) {
            StrategyIndicator rsi;
            rsi.type = StrategyIndicator::RSI;
            rsi.params["period"] = config.rsi_period;
            rsi.params["overboughtLevel"] = 70.0;  // Valeur par défaut
            rsi.params["oversoldLevel"] = config.rsi_threshold;
            indicators.push_back(rsi);
        }
        
        // Stochastique
        bool use_stoch = config.use_stoch_filter;
        if (use_stoch) {
            StrategyIndicator stoch;
            stoch.type = StrategyIndicator::STOCHASTIC;
            stoch.params["fastKPeriod"] = config.stoch_fastk;
            stoch.params["slowKPeriod"] = config.stoch_slowk;
            stoch.params["slowDPeriod"] = config.stoch_slowd;
            stoch.params["overboughtLevel"] = 80.0;  // Valeur par défaut
            stoch.params["oversoldLevel"] = config.stoch_threshold;
            indicators.push_back(stoch);
        }
        
        // Supertrend
        bool use_supertrend = config.use_supertrend_filter;
        if (use_supertrend) {
            StrategyIndicator supertrend;
            supertrend.type = StrategyIndicator::SUPERTREND;
            supertrend.params["period"] = config.supertrend_atr_period;
            supertrend.params["multiplier"] = config.supertrend_multiplier;
            indicators.push_back(supertrend);
        }
    }
    else if (strategyName.contains("SellHeikinRed", Qt::CaseInsensitive)) {
        // QMap<QString, QVariant> specificValues = m_strategySpecificPanel->getValues();
        SellHeikinRedConfig config = getSellHeikinRedConfig();
        // EMA court terme
        bool use_ema_short = config.use_ema_short_filter;
        if (use_ema_short) {
            StrategyIndicator ema;
            ema.type = StrategyIndicator::EMA;
            ema.params["period"] = config.ema_short_period;
            indicators.push_back(ema);
        }
        
        // EMA long terme
        bool use_ema_long = config.use_ema_long_filter;
        if (use_ema_long) {
            StrategyIndicator ema;
            ema.type = StrategyIndicator::EMA;
            ema.params["period"] = config.ema_long_period;
            indicators.push_back(ema);
        }
        
        // RSI
        bool use_rsi = config.use_rsi_filter;
        if (use_rsi) {
            StrategyIndicator rsi;
            rsi.type = StrategyIndicator::RSI;
            rsi.params["period"] = config.rsi_period;
            rsi.params["overboughtLevel"] = config.rsi_threshold;
            rsi.params["oversoldLevel"] = 30.0;  // Valeur par défaut
            indicators.push_back(rsi);
        }
        
        // Stochastique
        bool use_stoch = config.use_stoch_filter;
        if (use_stoch) {
            StrategyIndicator stoch;
            stoch.type = StrategyIndicator::STOCHASTIC;
            stoch.params["fastKPeriod"] = config.stoch_fastk;
            stoch.params["slowKPeriod"] = config.stoch_slowk;
            stoch.params["slowDPeriod"] = config.stoch_slowd;
            stoch.params["overboughtLevel"] = config.stoch_threshold;
            stoch.params["oversoldLevel"] = 20.0;  // Valeur par défaut
            indicators.push_back(stoch);
        }
        
        // Supertrend
        bool use_supertrend = config.use_supertrend_filter;
        if (use_supertrend) {
            StrategyIndicator supertrend;
            supertrend.type = StrategyIndicator::SUPERTREND;
            supertrend.params["period"] = config.supertrend_atr_period;
            supertrend.params["multiplier"] = config.supertrend_multiplier;
            indicators.push_back(supertrend);
        }
    }
    
    // Pour d'autres stratégies, ajouter d'autres conditions ici
    
    return indicators;
}

void App::setBacktestResults(std::unique_ptr<BacktestResults> results) {
    m_backtestResults = std::move(results);
    // Mettre à jour les vues avec le nouveau pointeur
    updateResultViews(m_backtestResults.get());
}

void App::updateResultViews(BacktestResults* results) {
    if (m_resultManager) {
        m_resultManager->updateAllViews(results);
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
    
    qDebug() << "Taille de la fenêtre:" << size().width() << "x" << size().height();
}