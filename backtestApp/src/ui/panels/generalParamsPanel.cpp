#include "ui/panels/generalParamsPanel.h"
#include <QFormLayout>
#include <QLabel>
#include <QDate>
#include <QDateEdit>
#include <QDebug>
#include "components/Utils/dataLoader.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <filesystem>
#include <set>

GeneralParamsPanel::GeneralParamsPanel(QWidget* parent)
    : ConfigPanel<GeneralParamsConfig>("General Parameters", parent)
{
    setupUI();
}

void GeneralParamsPanel::refreshSymbols()
{
    if (!m_symbolCombo) return;

    const std::filesystem::path marketDataDir = std::filesystem::path(m_marketDataDir.toStdString());
    std::set<std::string> prefixes;

    if (std::filesystem::exists(marketDataDir) && std::filesystem::is_directory(marketDataDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(marketDataDir)) {
            if (!entry.is_regular_file()) continue;
            const auto filename = entry.path().filename().string();
            auto pos = filename.find('_');
            if (pos == std::string::npos) continue;
            const std::string prefix = filename.substr(0, pos);
            if (!prefix.empty()) prefixes.insert(prefix);
        }
    }

    // Preserve current selection if possible
    const QString current = m_symbolCombo->currentText();

    m_symbolCombo->blockSignals(true);
    m_symbolCombo->clear();
    for (const auto& p : prefixes) m_symbolCombo->addItem(QString::fromStdString(p));
    m_symbolCombo->blockSignals(false);

    if (!current.isEmpty()) {
        const int idx = m_symbolCombo->findText(current);
        if (idx != -1) m_symbolCombo->setCurrentIndex(idx);
    }

    if (prefixes.empty()) {
        qWarning() << "No market data files found in" << m_marketDataDir;
    }
}

void GeneralParamsPanel::setupUI()
{
    // Use "this" as main container instead of creating a new QGroupBox
    QFormLayout* paramsLayout = new QFormLayout(this);
    
    // Symbol
    m_symbolCombo = new QComboBox(this);
    // Dynamically populate symbols from the project's marketData directory.
    // Symbol is defined as the prefix before the first underscore in the filename.

    // Resolve market data directory early so refreshSymbols and the watcher use
    // a consistent, absolute path. Prefer DataLoader (reads QSettings and
    // performs a search), fall back to applicationDirPath-based heuristics.
    QString detected = DataLoader::findMarketDataDirectory();
    if (!detected.isEmpty()) {
        m_marketDataDir = detected;
        qDebug() << "GeneralParamsPanel: Using marketData dir from DataLoader():" << m_marketDataDir;
    } else {
        // If m_marketDataDir is empty or relative, try to resolve it relative to the exe dir
        QString appDir = QCoreApplication::applicationDirPath();
        if (m_marketDataDir.isEmpty()) {
            // Try common locations under the exe dir and its parents (similar to DataLoader)
            QDir cur(appDir);
            bool found = false;
            do {
                QString candidate = cur.absoluteFilePath("marketData");
                if (QDir(candidate).exists()) {
                    m_marketDataDir = candidate;
                    qDebug() << "GeneralParamsPanel: Found marketData under parent tree:" << m_marketDataDir;
                    found = true;
                    break;
                }
            } while (cur.cdUp());

            if (!found) {
                qWarning() << "GeneralParamsPanel: could not auto-discover marketData directory. m_marketDataDir is empty or not set.";
            }
        } else {
            // Convert relative path to absolute based on application dir
            QDir given(m_marketDataDir);
            if (!given.isAbsolute()) {
                QString abs = QDir(appDir).absoluteFilePath(m_marketDataDir);
                m_marketDataDir = abs;
                qDebug() << "GeneralParamsPanel: Converted relative m_marketDataDir to absolute:" << m_marketDataDir;
            }
        }
    }

    // Populate initial symbols and setup watcher (use resolved m_marketDataDir)
    refreshSymbols();

    // Add watcher using the resolved path so directory changes trigger refresh
    if (m_marketDataDir.isEmpty()) {
        qWarning() << "GeneralParamsPanel: marketData directory is empty - QFileSystemWatcher not added.";
    } else {
        if (!QFileInfo(m_marketDataDir).isDir()) {
            qWarning() << "GeneralParamsPanel: marketData path does not exist (watcher will still watch path):" << m_marketDataDir;
        }
        m_watcher.addPath(m_marketDataDir);
        connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &GeneralParamsPanel::refreshSymbols);
    }
                   

    paramsLayout->addRow(new QLabel("Symbol:", this), m_symbolCombo);
    
    // Binding for the symbol
    addBinding(PropertyBinderFactory::createStringComboBinding(
        m_symbolCombo,
        &m_config.symbol)
    );
    
    // Data period
    QComboBox* periodCombo = new QComboBox(this);
    periodCombo->addItems({"1m", "3m", "6m", "1y", "3y", "5y", "10y", "20y"});
    periodCombo->setCurrentIndex(5);
    paramsLayout->addRow(new QLabel("Data period:", this), periodCombo);
    
    // Binding for the period
    addBinding(PropertyBinderFactory::createStringComboBinding(
        periodCombo,
        &m_config.period)
    );
    
    // Interval
    QComboBox* intervalCombo = new QComboBox(this);
    intervalCombo->addItems({
        "10secs", "20secs", "30secs", "1min", "2min", "3min", "5min", 
        "10min", "15min", "30min", "1h", "2h", "4h", "1d"
    });
    intervalCombo->setCurrentIndex(1);
    paramsLayout->addRow(new QLabel("Interval:", this), intervalCombo);
    
    // Binding for the interval
    addBinding(PropertyBinderFactory::createStringComboBinding(
        intervalCombo,
        &m_config.interval)
    );
    
    // End date
    QDateEdit* dateEdit = new QDateEdit(this);
    // Initialize with current date if endDate is invalid, otherwise use endDate
    if (m_config.endDate.isValid())
        dateEdit->setDate(m_config.endDate.date());
    else 
        dateEdit->setDate(QDate::currentDate());
    
    dateEdit->setCalendarPopup(true);
    paramsLayout->addRow(new QLabel("End date:", this), dateEdit);
    
    // Binding for the end date
    addBinding(PropertyBinderFactory::createDateTimeBinding(
        dateEdit,
        &m_config.endDate)
    );
    
    // // Spread (per mille)
    // QDoubleSpinBox* spreadSpin = new QDoubleSpinBox(this);
    // spreadSpin->setDecimals(3);
    // spreadSpin->setRange(0, 10);
    // spreadSpin->setSingleStep(0.001); 
    // spreadSpin->setValue(0.100);
    // paramsLayout->addRow(new QLabel("Spread (‰):", this), spreadSpin);
    
    // // Binding for spread
    // addBinding(PropertyBinderFactory::createDoubleBinding(
    //     spreadSpin,
    //     &m_config.spread)
    // );

    // QDoubleSpinBox* commissionSpin = new QDoubleSpinBox(this);
    // commissionSpin->setDecimals(2);
    // commissionSpin->setRange(0.0, 100.0);
    // commissionSpin->setSingleStep(0.01);
    // commissionSpin->setValue(0.0);
    // paramsLayout->addRow(new QLabel("Commission (%):", this), commissionSpin);

    // // Binding for commission
    // addBinding(PropertyBinderFactory::createDoubleBinding(
    //     commissionSpin,
    //     &m_config.commission)
    // );
    
    // // Initial cash
    // QDoubleSpinBox* cashSpin = new QDoubleSpinBox(this);
    // cashSpin->setDecimals(2);
    // cashSpin->setRange(1000, 10000000);
    // cashSpin->setSingleStep(1000);
    // cashSpin->setValue(10000);
    // paramsLayout->addRow(new QLabel("Initial cash:", this), cashSpin);
    
    // // Binding for cash
    // addBinding(PropertyBinderFactory::createDoubleBinding(
    //     cashSpin,
    //     &m_config.cash)
    // );

    // // Maximum allowed leverage
    // QDoubleSpinBox* leverageSpin = new QDoubleSpinBox(this);
    // leverageSpin->setDecimals(2);
    // leverageSpin->setRange(1, 10000);
    // leverageSpin->setSingleStep(1);
    // leverageSpin->setValue(20);
    // paramsLayout->addRow(new QLabel("Maximum allowed leverage:", this), leverageSpin);
    
    // // Binding for leverage
    // addBinding(PropertyBinderFactory::createDoubleBinding(
    //     leverageSpin,
    //     &m_config.leverage_limit)
    // );

    // // Trade on close
    // QCheckBox* tradeOnCloseCheck = new QCheckBox("Trade on close", this);
    // tradeOnCloseCheck->setChecked(false);
    // paramsLayout->addRow(tradeOnCloseCheck);

    // // Binding for trade on close
    // addBinding(PropertyBinderFactory::createBoolBinding(
    //     tradeOnCloseCheck,
    //     &m_config.tradeOnClose)
    // );

    // // Position mode
    // QComboBox* positionModeCombo = new QComboBox(this);
    // positionModeCombo->addItems({"Hedging", "Netting"});
    // positionModeCombo->setCurrentIndex(0);
    // paramsLayout->addRow(new QLabel("Position Mode:", this), positionModeCombo);

    // // Binding for position mode
    // addBinding(PropertyBinderFactory::createEnumComboBinding<be::PositionMode>(
    //     positionModeCombo,
    //     &m_config.positionMode)
    // );

    // // Finalize trades
    // QCheckBox* finalizeTradesCheck = new QCheckBox("Finalize trades", this);
    // finalizeTradesCheck->setChecked(true);
    // paramsLayout->addRow(finalizeTradesCheck);

    // // Binding for finalize trades
    // addBinding(PropertyBinderFactory::createBoolBinding(
    //     finalizeTradesCheck,
    //     &m_config.finalizeTrades)
    // );

    // Advanced configuration button
    QPushButton* advancedConfigBtn = new QPushButton("Advanced Configuration...", this);
    paramsLayout->addRow(advancedConfigBtn);
    
    // Connect button
    connect(advancedConfigBtn, &QPushButton::clicked, this, &GeneralParamsPanel::openAdvancedConfigDialog);
    
    // By default, initialize advanced options
    m_config.executeStopOnOpen = true; // default worst-case
    m_config.executeLimitOnLimitPrice = true; // default worst-case
}


void GeneralParamsPanel::openAdvancedConfigDialog()
{
    BacktestEngineDialog dialog(this);
    
    // Initialize the dialog with current values
    dialog.setExecuteStopOnOpen(m_config.executeStopOnOpen);
    dialog.setExecuteLimitOnLimitPrice(m_config.executeLimitOnLimitPrice);
    dialog.setTradeOnClose(m_config.tradeOnClose);
    dialog.setLeverageLimit(m_config.leverage_limit);
    dialog.setPositionMode(m_config.positionMode);
    dialog.setFinalizeTrades(m_config.finalizeTrades);
    dialog.setSpread(m_config.spread);
    dialog.setCommission(m_config.commission);
    dialog.setCash(m_config.cash);
    dialog.setSpreadEntryRatio(m_config.spreadEntryRatio);
    dialog.setMinPositionStep(m_config.minPositionStep);
    
    // Execute the dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Retrieve new values
        m_config.executeStopOnOpen = dialog.executeStopOnOpen();
        m_config.executeLimitOnLimitPrice = dialog.executeLimitOnLimitPrice();
        m_config.tradeOnClose = dialog.tradeOnClose();
        m_config.leverage_limit = dialog.leverageLimit();
        m_config.positionMode = dialog.positionMode();
        m_config.finalizeTrades = dialog.finalizeTrades();
        m_config.spread = dialog.spread();
        m_config.commission = dialog.commission();
        m_config.cash = dialog.cash();
        m_config.spreadEntryRatio = dialog.spreadEntryRatio();
        m_config.minPositionStep = dialog.minPositionStep();
    }
}