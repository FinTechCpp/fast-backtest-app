#include "app.h"
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QTime>

#include "components/configManager.h"
#include "components/updateChecker.h"
#include "panels/generalParamsPanel.h"
#include "panels/strategySpecificPanels/strategyBasePanel.h"
#include "menu/profileMenuManager.h"
#include "menu/dataMenuManager.h"
#include "menu/updateMenuManager.h"
#include "views/statsView.h"
#include "views/chartView.h"
#include "views/histogramView.h"
#include "components/resultManager.h"
#include "components/backtestRunner.h"
#include "panels/strategySpecificPanels/buyHeikinGreenPanel.h"
#include "panels/strategySpecificPanels/sellHeikinRedPanel.h"


App::App() : QMainWindow()
{
    // Set window properties
    setWindowTitle("Backtest Dashboard C++");
    resize(2100, 1300);
    
    // Initialize strategy map FIRST
    initStrategyMap();
    
    // Initialize configuration manager SECOND
    m_configManager = new ConfigManager(this);
    
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
    m_strategyMap["BuyTrendFollowingBA"] = "BuyTrendFollowingBA";
    m_strategyMap["SellTrendFollowingBA"] = "SellTrendFollowingBA";
    m_strategyMap["CrossEMABA"] = "CrossEMABA";
}

void App::createControlPanel()
{
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
    m_generalParamsPanel->initialize(); // Appeler initialize() après la création
    m_controlPanelLayout->addWidget(m_generalParamsPanel); // Ajouter directement le panel
    
    // Strategy base panel
    m_strategyBasePanel = new StrategyBasePanel(m_controlPanel);
    m_strategyBasePanel->initialize(); // Appeler initialize() après la création
    m_controlPanelLayout->addWidget(m_strategyBasePanel); // Ajouter directement le panel
    
    // Strategy-specific panel container
    m_strategyPanelStack = new QStackedWidget();
    m_controlPanelLayout->addWidget(m_strategyPanelStack);
    
    // Initialize strategy-specific panels
    updateStrategySpecificPanel();
    
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
        QWidget* strategyWidget = m_generalParamsPanel->getWidgetByName("strategy");
        if (QComboBox* strategyCombo = qobject_cast<QComboBox*>(strategyWidget)) {
            connect(strategyCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, &App::onStrategyChanged);
        }
    }

    // Connexion du BacktestRunner
    if (m_backtestRunner) {
        connect(m_backtestRunner, &BacktestRunner::backtestCompleted,
                this, &App::onBacktestCompleted);
        connect(m_backtestRunner, &BacktestRunner::backtestError,
                this, &App::onBacktestError);
    }
}

void App::updateStrategySpecificPanel()
{
    if (!m_generalParamsPanel) return;
    
    QComboBox* strategyCombo = qobject_cast<QComboBox*>(
            m_generalParamsPanel->getWidgetByName("strategy"));
    if (!strategyCombo) return;
    
    QString selectedStrategy = strategyCombo->currentText();
    qInfo() << "Updating strategy-specific panel for:" << selectedStrategy;
    
    // Remove old panel if it exists
    if (m_strategySpecificPanel) {
        // CORRECTION: Utiliser une méthode correcte pour récupérer le widget
        // Chercher le widget dans le stack ou le layout
        for (int i = 0; i < m_strategyPanelStack->count(); ++i) {
            QWidget* widget = m_strategyPanelStack->widget(i);
            m_strategyPanelStack->removeWidget(widget);
            widget->deleteLater();
        }
        m_strategySpecificPanel = nullptr;
    }
    
    // Create new panel based on strategy
    QWidget* strategyWidget = createStrategySpecificPanel(selectedStrategy);
    if (strategyWidget) {
        m_strategyPanelStack->addWidget(strategyWidget);
        m_strategyPanelStack->setCurrentWidget(strategyWidget);
    }
    
    // Load current profile values
    QMap<QString, QVariant> currentProfile = m_configManager->getProfile(m_configManager->getCurrentProfile());
    if (m_strategySpecificPanel && !currentProfile.isEmpty()) {
        m_strategySpecificPanel->setValues(currentProfile);
    }
}

QWidget* App::createStrategySpecificPanel(const QString& strategy)
{
    BasePanel* panel = nullptr;
    
    if (strategy == "BuyHeikinGreenBA") {
        panel = new BuyHeikinGreenPanel(m_controlPanel);
    } else if (strategy == "SellHeikinRedBA") {
        panel = new SellHeikinRedPanel(m_controlPanel);
    }
    
    if (panel) {
        m_strategySpecificPanel = panel;
        panel->initialize();
        return panel;
    }
    
    return nullptr;
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

QMap<QString, QVariant> App::getStrategyConfig() const
{
    QMap<QString, QVariant> config;
    
    // Récupérer depuis le panel général
    if (m_generalParamsPanel) {
        QMap<QString, QVariant> generalValues = m_generalParamsPanel->getValues();
        config.insert(generalValues);  // Qt 5.15+
    }
    
    // Récupérer depuis le panel de base
    if (m_strategyBasePanel) {
        QMap<QString, QVariant> baseValues = m_strategyBasePanel->getValues();
        config.insert(baseValues);  // Qt 5.15+
    }
    
    // Récupérer depuis le panel spécifique à la stratégie
    if (m_strategySpecificPanel) {
        QMap<QString, QVariant> specificValues = m_strategySpecificPanel->getValues();
        config.insert(specificValues);  // Qt 5.15+
    }
    
    // Debug pour vérifier
    qDebug() << "Config récupérée:" << config;
    
    return config;
}

std::vector<StrategyIndicator> App::getIndicatorConfig() const
{
    // Obtenir les paramètres de stratégie
    QMap<QString, QVariant> params = this->getStrategyConfig();
    std::vector<StrategyIndicator> indicators;
    
    // Extraire ATR si utilisé pour SL ou TP
    bool use_atr_for_sl = params.value("use_atr_for_sl", false).toBool();
    bool use_atr_for_tp = params.value("use_atr_for_tp", false).toBool();
    
    if (use_atr_for_sl || use_atr_for_tp) {
        StrategyIndicator atr;
        atr.type = StrategyIndicator::ATR;
        atr.params["period"] = params.value("atr_period", 14).toDouble();
        atr.params["useLogScale"] = 1.0;  // true par défaut
        indicators.push_back(atr);
    }
    
    // Extraire les indicateurs spécifiques à la stratégie
    QString strategyName = params.value("strategy", "").toString();
    
    if (strategyName.contains("BuyHeikinGreen", Qt::CaseInsensitive)) {
        // EMA court terme
        bool use_ema_short = params.value("use_ema_short_filter", false).toBool();
        if (use_ema_short) {
            StrategyIndicator ema;
            ema.type = StrategyIndicator::EMA;
            ema.params["period"] = params.value("ema_short_period", 150).toDouble();
            indicators.push_back(ema);
        }
        
        // EMA long terme
        bool use_ema_long = params.value("use_ema_long_filter", false).toBool();
        if (use_ema_long) {
            StrategyIndicator ema;
            ema.type = StrategyIndicator::EMA;
            ema.params["period"] = params.value("ema_long_period", 198).toDouble();
            indicators.push_back(ema);
        }
        
        // RSI
        bool use_rsi = params.value("use_rsi_filter", false).toBool();
        if (use_rsi) {
            StrategyIndicator rsi;
            rsi.type = StrategyIndicator::RSI;
            rsi.params["period"] = params.value("rsi_period", 14).toDouble();
            rsi.params["overboughtLevel"] = 70.0;  // Valeur par défaut
            rsi.params["oversoldLevel"] = params.value("rsi_threshold", 30).toDouble();
            indicators.push_back(rsi);
        }
        
        // Stochastique
        bool use_stoch = params.value("use_stoch_filter", false).toBool();
        if (use_stoch) {
            StrategyIndicator stoch;
            stoch.type = StrategyIndicator::STOCHASTIC;
            stoch.params["fastKPeriod"] = params.value("stoch_fastk", 10).toDouble();
            stoch.params["slowKPeriod"] = params.value("stoch_slowk", 7).toDouble();
            stoch.params["slowDPeriod"] = params.value("stoch_slowd", 3).toDouble();
            stoch.params["overboughtLevel"] = 80.0;  // Valeur par défaut
            stoch.params["oversoldLevel"] = params.value("stoch_threshold", 20).toDouble();
            indicators.push_back(stoch);
        }
        
        // Supertrend
        bool use_supertrend = params.value("use_supertrend_filter", false).toBool();
        if (use_supertrend) {
            StrategyIndicator supertrend;
            supertrend.type = StrategyIndicator::SUPERTREND;
            supertrend.params["period"] = params.value("supertrend_atr_period", 10).toDouble();
            supertrend.params["multiplier"] = params.value("supertrend_multiplier", 3.0).toDouble();
            indicators.push_back(supertrend);
        }
    }
    
    // Pour SellHeikinRed ou d'autres stratégies, ajouter d'autres conditions ici
    
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