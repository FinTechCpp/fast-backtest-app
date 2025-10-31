#include "ui/app.h"
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QTime>
#include <QCoreApplication>

#include "components/Managers/ProfileManager.h"
#include "components/Managers/BacktestResultManager.h"
#include "components/updateChecker.h"
#include "ui/panels/generalParamsPanel.h"
#include "ui/panels/StrategyPanel.h"
#include "ui/menu/profileMenuManager.h"
#include "ui/menu/dataMenuManager.h"
#include "ui/menu/updateMenuManager.h"
#include "ui/menu/BacktestResultMenuManager.h"
#include "ui/views/statsView.h"
#include "ui/views/chartView.h"
#include "components/Managers/resultManager.h"
#include "components/backtestRunner.h"
#include "components/Utils/SerializationUtils.hpp"


App::App() : QMainWindow() {
    // Set window properties
    setWindowTitle("Backtest Dashboard C++");
    resize(2100, 1300);
        
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

    // Connect profile change signal to update the indicator label
    if (m_configManager && m_profileIndicator) {
        connect(m_configManager, &ProfileManager::profileChanged, this, [this](const QString& profile){
            if (m_profileIndicator)
                m_profileIndicator->setText(tr("active profile: %1").arg(profile));
        });
        // Ensure the indicator displays the current profile at startup
        m_profileIndicator->setText(tr("active profile: %1").arg(m_configManager->getCurrentProfile()));
    }

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
    QMessageBox::about(this, tr("About"),
                       tr("Backtest Application\n"
                          "Version %1\n"
                          "DDeveloped by Hugo Miquel and Maxime Deville\n").arg(current.toString()));
}

App::~App()
{
    // Qt will handle deleting child widgets automatically
    qDebug() << "App destructor called";
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
    
    // Profile indicator - afficher le profil actif entre le BacktestRunner et les panels
    QWidget* profileRow = new QWidget(this);
    QHBoxLayout* profileRowLayout = new QHBoxLayout(profileRow);
    // Make the profileRow look like the grey rounded box
    profileRow->setStyleSheet("QWidget#profileRow { background-color: #f0f4f8; border: 1px solid #cbd5e1; border-radius: 6px; }");
    profileRow->setObjectName("profileRow");
    profileRowLayout->setContentsMargins(6, 6, 6, 6);
    profileRowLayout->setSpacing(8);

    m_profileIndicator = new QLabel(this);
    m_profileIndicator->setMinimumHeight(28);
    m_profileIndicator->setStyleSheet("QLabel { background: transparent; color: #333; }");
    m_profileIndicator->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // Small reset button to reload the current profile (inside the grey box)
    m_profileResetButton = new QPushButton(tr("Reset"), this);
    m_profileResetButton->setToolTip(tr("Recharger le profil actif"));
    m_profileResetButton->setFixedSize(70, 28);
    m_profileResetButton->setStyleSheet("QPushButton { font-size: 11px; padding: 2px 6px; }");

    // Initial text
    if (m_configManager)
        m_profileIndicator->setText(tr("active profile: %1").arg(m_configManager->getCurrentProfile()));
    else
        m_profileIndicator->setText(tr("active profile: -"));

    // Connect reset button to reload the current profile
    connect(m_profileResetButton, &QPushButton::clicked, this, [this]() {
        if (m_configManager) {
            QString current = m_configManager->getCurrentProfile();
            if (!current.isEmpty()) {
                // Call ProfileManager::onProfileChanged to reapply the profile
                m_configManager->onProfileChanged(current);
            }
        }
    });

    // Add indicator and button to the row (indicator expands, button stays compact)
    profileRowLayout->addWidget(m_profileIndicator, 1);
    profileRowLayout->addWidget(m_profileResetButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    // Add the profile row to the control panel
    m_controlPanelLayout->addWidget(profileRow);
    
    // General parameters panel
    m_generalParamsPanel = new GeneralParamsPanel(m_controlPanel);
    m_controlPanelLayout->addWidget(m_generalParamsPanel);
    
    // Strategy base panel
    m_strategyPanel = new StrategyPanel(m_controlPanel);
    m_controlPanelLayout->addWidget(m_strategyPanel);
    
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
    // Connect BacktestRunner
    if (m_backtestRunner) {
        connect(m_backtestRunner, &BacktestRunner::backtestCompleted,
                this, &App::onBacktestCompleted);
        connect(m_backtestRunner, &BacktestRunner::backtestError,
                this, &App::onBacktestError);
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

std::vector<StrategyConfig> App::getStrategyConfigs() const {
    if (m_strategyPanel)
        return m_strategyPanel->getConfigs();
    
    // Retourner un vecteur avec une config par défaut
    std::vector<StrategyConfig> defaultConfigs;
    defaultConfigs.push_back(StrategyConfig());
    return defaultConfigs;
}


void App::setGeneralParamsConfig(const GeneralParamsConfig& config) {
    if (m_generalParamsPanel) {
        m_generalParamsPanel->setConfig(config);
    }
}

void App::setStrategyConfigs(const std::vector<StrategyConfig>& configs) {
    if (m_strategyPanel) {
        m_strategyPanel->setConfigs(configs);
    }
}

void App::setBacktestResults(std::unique_ptr<BacktestResults> results) {
    m_backtestResults = std::move(results);

    if (m_resultManager) {
        m_resultManager->updateAllViews(m_backtestResults.get());
    }
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