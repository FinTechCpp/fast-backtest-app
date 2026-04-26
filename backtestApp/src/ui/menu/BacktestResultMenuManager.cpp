#include "ui/menu/BacktestResultMenuManager.h"
#include "ui/app.h"
#include "components/Managers/BacktestResultManager.h"
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication>
#include <QInputDialog>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QPushButton>

BacktestResultMenuManager::BacktestResultMenuManager(App* parent)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_resultManager(nullptr)
    , m_resultMenu(nullptr)
    , m_saveResultAction(nullptr)
    , m_manageResultsAction(nullptr)
    , m_importAction(nullptr)
    , m_importExternalAction(nullptr)
    , m_openDirectoryAction(nullptr)
{
    qDebug() << "BacktestResultMenuManager created";
}

void BacktestResultMenuManager::createResultMenu(QMenuBar* menuBar)
{
    if (!menuBar) {
        qWarning() << "MenuBar null passed to createResultMenu";
        return;
    }

    // Create the Results menu (before the Help menu)
    m_resultMenu = menuBar->addMenu(tr("&Results"));
    
    createActions();

    // Add actions to the menu
    m_resultMenu->addAction(m_saveResultAction);
    m_resultMenu->addSeparator();

    // Main action to manage results (load, view details, export, delete)
    m_resultMenu->addAction(m_manageResultsAction);
    
    m_resultMenu->addSeparator();
    m_resultMenu->addAction(m_importAction);
    m_resultMenu->addAction(m_importExternalAction);

    // Add a separator then the action to open the directory
    m_resultMenu->addSeparator();
    m_resultMenu->addAction(m_openDirectoryAction);

    qDebug() << "Results menu created in menu bar";
}

void BacktestResultMenuManager::createActions()
{
    // Action Save
    m_saveResultAction = new QAction(tr("&Save Current Result"), this);
    m_saveResultAction->setStatusTip(tr("Save the current backtest result"));
    connect(m_saveResultAction, &QAction::triggered, this, &BacktestResultMenuManager::onSaveCurrentResult);

    // Action Manage Results
    m_manageResultsAction = new QAction(tr("&Manage Results..."), this);
    m_manageResultsAction->setStatusTip(tr("Load, view details, export or delete a result"));
    connect(m_manageResultsAction, &QAction::triggered, this, &BacktestResultMenuManager::onManageResults);

    // Action Import
    m_importAction = new QAction(tr("&Import Backtest Result..."), this);
    m_importAction->setStatusTip(tr("Import a complete backtest result (with stats)"));
    connect(m_importAction, &QAction::triggered, this, &BacktestResultMenuManager::onImportResult);

    // Action Import External Result
    m_importExternalAction = new QAction(tr("&Import External Result..."), this);
    m_importExternalAction->setStatusTip(tr("Import an external result (without stats - will be calculated automatically)"));
    connect(m_importExternalAction, &QAction::triggered, this, &BacktestResultMenuManager::onImportExternalResult);

    // Action - Open Results Directory
    m_openDirectoryAction = new QAction(tr("&Open Results Directory"), this);
    m_openDirectoryAction->setStatusTip(tr("Open the directory containing result files"));
    connect(m_openDirectoryAction, &QAction::triggered, this, &BacktestResultMenuManager::onOpenResultsDirectory);
}

void BacktestResultMenuManager::setResultManager(BacktestResultManager* resultManager)
{
    m_resultManager = resultManager;
    if (m_resultManager) {
        qDebug() << "Connecting BacktestResultManager to BacktestResultMenuManager";

        // Connect signals
        connect(m_resultManager, &BacktestResultManager::resultListUpdated,
                this, &BacktestResultMenuManager::updateResultList);

        qDebug() << "Signals connected, initial result list update";
        // Use a timer to ensure everything is initialized
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "Delayed update of result list";
            updateResultList();
        });

        qDebug() << "BacktestResultManager connected to BacktestResultMenuManager";
    } else {
        qWarning() << "BacktestResultManager null passed to setResultManager";
    }
}

void BacktestResultMenuManager::updateResultList()
{
    qDebug() << "updateResultList() called";

    if (!m_resultManager) {
        qWarning() << "BacktestResultManager missing";
        return;
    }

    // Clean up internal actions if needed
    for (auto action : m_resultActions.values()) 
        delete action;
    m_resultActions.clear();

    // Note: We no longer use dynamic sub-menus
    // Result management is now done via the onManageResults() dialog
    QStringList results = m_resultManager->listBacktestResults();
    qDebug() << "Result list updated with" << results.size() << "results";
}

void BacktestResultMenuManager::onResultLoaded(const QString& resultName)
{
    // Update the current result name
    m_currentResult = resultName;

    qDebug() << "Current result updated to:" << resultName;
}

void BacktestResultMenuManager::onSaveCurrentResult()
{
    if (!m_resultManager || !m_mainWindow) 
        return;
    
    // Create a new result from the current stats
    // This assumes that the App has a method to retrieve the current stats
    // Example: be::Stats currentStats = m_mainWindow->getCurrentStats();

    // For this example, we will assume you have a method in App to create a config
    // BacktestResultConfig config = m_mainWindow->createBacktestResultConfig();
    
    bool ok;
    QString name = QInputDialog::getText(m_mainWindow, tr("Result Name"),
                                        tr("Enter a name for this result:"),
                                        QLineEdit::Normal, "", &ok);
    
    if (ok && !name.isEmpty()) {
        // Create a result config
        BacktestResultConfig config;
        config.name = name.toStdString();
        config.version = QCoreApplication::applicationVersion().toStdString();
        config.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();

        BacktestResults currentResults = m_mainWindow->getBacktestResults();

        config.generalParams = currentResults.generalConfig;
        config.strategyConfigs = currentResults.strategyConfigs;
        config.candles = currentResults.candles;
        config.stats = currentResults.stats;
        config.userMarkers = currentResults.userMarkers;

        // Save the result
        m_resultManager->saveBacktestResult(config, m_mainWindow, SerializationUtils::FileFormat::JSON);
    }
}

void BacktestResultMenuManager::onManageResults()
{
    if (!m_resultManager || !m_mainWindow) {
        return;
    }

    // Retrieve the list of available results
    QStringList results = m_resultManager->listBacktestResults();

    if (results.isEmpty()) {
        QMessageBox::information(m_mainWindow, tr("No Results"),
                                tr("No result files found."));
        return;
    }

    // Create a custom dialog
    QDialog dlg(m_mainWindow);
    dlg.setWindowTitle(tr("Manage Backtest Results"));
    dlg.resize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QLabel* label = new QLabel(tr("Select a result:"), &dlg);
    layout->addWidget(label);

    QListWidget* list = new QListWidget(&dlg);
    for (const QString& r : results) {
        list->addItem(r);
    }
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list);

    // Create action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* loadBtn = new QPushButton(tr("Load"), &dlg);
    QPushButton* detailsBtn = new QPushButton(tr("View Details"), &dlg);
    QPushButton* exportBtn = new QPushButton(tr("Export"), &dlg);
    QPushButton* deleteBtn = new QPushButton(tr("Delete"), &dlg);
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"), &dlg);

    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(detailsBtn);
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addWidget(deleteBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    
    layout->addLayout(buttonLayout);

    // Deactivate buttons if no result is selected
    auto updateButtonStates = [&]() {
        bool hasSelection = list->currentItem() != nullptr;
        loadBtn->setEnabled(hasSelection);
        detailsBtn->setEnabled(hasSelection);
        exportBtn->setEnabled(hasSelection);
        deleteBtn->setEnabled(hasSelection);
    };
    
    updateButtonStates();
    connect(list, &QListWidget::itemSelectionChanged, updateButtonStates);

    // Button connections

    // Load button
    connect(loadBtn, &QPushButton::clicked, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) return;
        
        QString resultName = selected->text();
        BacktestResultConfig config;
        
        if (m_resultManager->loadBacktestResult(resultName, config)) {
            std::unique_ptr<BacktestResults> backtestResults = std::make_unique<BacktestResults>();
            backtestResults->generalConfig = config.generalParams;
            backtestResults->strategyConfigs = config.strategyConfigs;
            backtestResults->candles = config.candles;
            backtestResults->stats = config.stats;
            backtestResults->userMarkers = config.userMarkers;

            m_mainWindow->setGeneralParamsConfig(config.generalParams);
            m_mainWindow->setStrategyConfigs(config.strategyConfigs);
            m_mainWindow->setBacktestResults(std::move(backtestResults));

            onResultLoaded(resultName);
            
            QMessageBox::information(&dlg, tr("Result Loaded"),
                                    tr("The result '%1' has been loaded successfully.").arg(resultName));
            dlg.accept();
        } else {
            QMessageBox::warning(&dlg, tr("Error"),
                                tr("Failed to load the result '%1'.").arg(resultName));
        }
    });
    
    // Button see details
    connect(detailsBtn, &QPushButton::clicked, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) return;
        
        QString resultName = selected->text();
        BacktestResultConfig config;
        
        if (m_resultManager->loadBacktestResult(resultName, config)) {
            QString details = tr("Name: %1\n"
                               "Version: %2\n"
                               "Created At: %3\n\n"
                               "Statistics:\n"
                               "- Return: %4%\n"
                               "- Transactions: %5\n"
                               "- Sharpe: %6\n"
                               "- Drawdown max: %7%")
                             .arg(QString::fromStdString(config.name))
                             .arg(QString::fromStdString(config.version))
                             .arg(QString::fromStdString(config.createdAt))
                             .arg(config.stats.returnPct, 0, 'f', 2)
                             .arg(config.stats.numTrades)
                             .arg(config.stats.sharpeRatio, 0, 'f', 2)
                             .arg(config.stats.maxDrawdownPct, 0, 'f', 2);
                             
            QMessageBox::information(&dlg, tr("Result details"), details);
        } else {
            QMessageBox::warning(&dlg, tr("Error"),
                                tr("Failed to load the details of the result '%1'.").arg(resultName));
        }
    });

    // Button Export
    connect(exportBtn, &QPushButton::clicked, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) return;
        
        QString resultName = selected->text();
        m_resultManager->exportBacktestResult(resultName, m_mainWindow);
    });

    // Button Delete
    connect(deleteBtn, &QPushButton::clicked, [&]() {
        QListWidgetItem* selected = list->currentItem();
        if (!selected) return;
        
        QString resultName = selected->text();

        // Confirmation is handled by deleteBacktestResult
        if (m_resultManager->deleteBacktestResult(resultName, m_mainWindow)) {
            // Remove the item from the list
            delete selected;

            // If it was the current result, reset it
            if (m_currentResult == resultName) {
                m_currentResult.clear();
                onResultLoaded(QString());
            }

            // If no more results, close the dialog
            if (list->count() == 0) {
                QMessageBox::information(&dlg, tr("No Result"),
                                        tr("There are no more results available."));
                dlg.reject();
            }
        }
    });
    
    // Button Cancel
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    // Double-click to load directly
    connect(list, &QListWidget::itemDoubleClicked, [&]() {
        loadBtn->click();
    });

    dlg.exec();
}

void BacktestResultMenuManager::onImportResult()
{
    if (m_resultManager) 
        if (m_resultManager->importBacktestResult(m_mainWindow)) 
            updateResultList();
}

void BacktestResultMenuManager::onImportExternalResult()
{
    if (m_resultManager) 
        if (m_resultManager->importExternalResult(m_mainWindow)) 
            updateResultList();
}

void BacktestResultMenuManager::onOpenResultsDirectory()
{
    if (m_resultManager) {
        if (!m_resultManager->openBacktestResultsDirectory()) {
            // Show an error message if failed
            QMessageBox::warning(m_mainWindow, tr("Error"),
                                 tr("Unable to open the results folder.\n"
                                    "Path: %1").arg(m_resultManager->getBacktestResultsDirectory()));
        }
    }
}