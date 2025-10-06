#include "ui/panels/generalParamsPanel.h"
#include <QFormLayout>
#include <QLabel>
#include <QDate>
#include <QDateEdit>
#include <QDebug>
#include <filesystem>
#include <set>

GeneralParamsPanel::GeneralParamsPanel(QWidget* parent)
    : ConfigPanel<GeneralParamsConfig>("Paramètres généraux", parent)
{
    setupUI();
}

void GeneralParamsPanel::setupUI()
{
    // Utiliser "this" comme conteneur principal au lieu de créer un nouveau QGroupBox
    QFormLayout* paramsLayout = new QFormLayout(this);
    
    // Symbole
    QComboBox* symbolCombo = new QComboBox(this);

    // Dynamically populate symbols from the project's marketData directory.
    // Symbol is defined as the prefix before the first underscore in the filename.

    const std::filesystem::path marketDataDir = std::filesystem::path("./marketData");
    std::set<QString> symbols;

    if (std::filesystem::exists(marketDataDir) && std::filesystem::is_directory(marketDataDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(marketDataDir)) {
            if (!entry.is_regular_file()) continue;
            const auto filename = entry.path().filename().string();
            auto pos = filename.find('_');
            if (pos == std::string::npos) continue;
            const std::string prefix = filename.substr(0, pos);
            if (!prefix.empty()) symbols.insert(QString::fromStdString(prefix));
        }
    }
    if (!symbols.empty()) 
        for (const auto& s : symbols) symbolCombo->addItem(s);
    else 
        qWarning() << "No market data files found in ./marketData directory.";
                   

    paramsLayout->addRow(new QLabel("Symbole:", this), symbolCombo);
    
    // Binding pour le symbole
    addBinding(PropertyBinderFactory::createStringComboBinding(
        symbolCombo,
        &m_config.symbol)
    );
    
    // Période
    QComboBox* periodCombo = new QComboBox(this);
    periodCombo->addItems({"1m", "3m", "6m", "1y", "3y", "5y", "10y", "20y"});
    periodCombo->setCurrentIndex(5);
    paramsLayout->addRow(new QLabel("Période de données:", this), periodCombo);
    
    // Binding pour la période
    addBinding(PropertyBinderFactory::createStringComboBinding(
        periodCombo,
        &m_config.period)
    );
    
    // Intervalle
    QComboBox* intervalCombo = new QComboBox(this);
    intervalCombo->addItems({
        "10secs", "20secs", "30secs", "1min", "2min", "3min", "5min", 
        "10min", "15min", "30min", "1h", "2h", "4h", "1d"
    });
    intervalCombo->setCurrentIndex(1);
    paramsLayout->addRow(new QLabel("Intervalle:", this), intervalCombo);
    
    // Binding pour l'intervalle
    addBinding(PropertyBinderFactory::createStringComboBinding(
        intervalCombo,
        &m_config.interval)
    );
    
    // Date de fin
    QDateEdit* dateEdit = new QDateEdit(this);
    dateEdit->setDate(QDate(2025, 8, 30));
    dateEdit->setCalendarPopup(true);
    paramsLayout->addRow(new QLabel("Date de fin:", this), dateEdit);
    
    // Binding pour la date de fin
    addBinding(PropertyBinderFactory::createDateTimeBinding(
        dateEdit,
        &m_config.endDate)
    );
    
    // Spread (en pour mille)
    QDoubleSpinBox* spreadSpin = new QDoubleSpinBox(this);
    spreadSpin->setDecimals(3);
    spreadSpin->setRange(0, 10);
    spreadSpin->setSingleStep(0.001); 
    spreadSpin->setValue(0.100);
    paramsLayout->addRow(new QLabel("Spread (‰):", this), spreadSpin);
    
    // Binding pour le spread
    addBinding(PropertyBinderFactory::createDoubleBinding(
        spreadSpin,
        &m_config.spread)
    );

    QDoubleSpinBox* commissionSpin = new QDoubleSpinBox(this);
    commissionSpin->setDecimals(2);
    commissionSpin->setRange(0.0, 100.0);
    commissionSpin->setSingleStep(0.01);
    commissionSpin->setValue(0.0);
    paramsLayout->addRow(new QLabel("Commission (%):", this), commissionSpin);

    // Binding pour la commission
    addBinding(PropertyBinderFactory::createDoubleBinding(
        commissionSpin,
        &m_config.commission)
    );
    
    // Cash initial
    QDoubleSpinBox* cashSpin = new QDoubleSpinBox(this);
    cashSpin->setDecimals(2);
    cashSpin->setRange(1000, 10000000);
    cashSpin->setSingleStep(1000);
    cashSpin->setValue(10000);
    paramsLayout->addRow(new QLabel("Cash initial:", this), cashSpin);
    
    // Binding pour le cash
    addBinding(PropertyBinderFactory::createDoubleBinding(
        cashSpin,
        &m_config.cash)
    );

    // Levier maximal autorisé
    QDoubleSpinBox* leverageSpin = new QDoubleSpinBox(this);
    leverageSpin->setDecimals(2);
    leverageSpin->setRange(1, 10000);
    leverageSpin->setSingleStep(1);
    leverageSpin->setValue(20);
    paramsLayout->addRow(new QLabel("Levier maximal autorisé :", this), leverageSpin);
    
    // Binding pour le levier
    addBinding(PropertyBinderFactory::createDoubleBinding(
        leverageSpin,
        &m_config.leverage_limit)
    );

    // Trade on close
    QCheckBox* tradeOnCloseCheck = new QCheckBox("Trade on close", this);
    tradeOnCloseCheck->setChecked(false);
    paramsLayout->addRow(tradeOnCloseCheck);

    // Binding pour trade on close
    addBinding(PropertyBinderFactory::createBoolBinding(
        tradeOnCloseCheck,
        &m_config.tradeOnClose)
    );

    // Hedging
    QCheckBox* hedgingCheck = new QCheckBox("Hedging", this);
    hedgingCheck->setChecked(false);
    paramsLayout->addRow(hedgingCheck);

    // Binding pour hedging
    addBinding(PropertyBinderFactory::createBoolBinding(
        hedgingCheck,
        &m_config.hedging)
    );

    // Exclusive orders
    QCheckBox* exclusiveOrdersCheck = new QCheckBox("Exclusive orders", this);
    exclusiveOrdersCheck->setChecked(true);
    paramsLayout->addRow(exclusiveOrdersCheck);

    // Binding pour exclusive orders
    addBinding(PropertyBinderFactory::createBoolBinding(
        exclusiveOrdersCheck,
        &m_config.exclusiveOrders)
    );

    // Finalize trades
    QCheckBox* finalizeTradesCheck = new QCheckBox("Finalize trades", this);
    finalizeTradesCheck->setChecked(true);
    paramsLayout->addRow(finalizeTradesCheck);

    // Binding pour finalize trades
    addBinding(PropertyBinderFactory::createBoolBinding(
        finalizeTradesCheck,
        &m_config.finalizeTrades)
    );
}