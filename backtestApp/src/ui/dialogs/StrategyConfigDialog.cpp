#include "ui/dialogs/StrategyConfigDialog.h"
#include "ui/dialogs/StopLossTakeProfitDialog.h"
#include "ui/dialogs/TradingHoursDialog.h"
#include "ui/dialogs/RiskManagementDialog.h"
#include "ui/dialogs/MLConfigDialog.h"
#include "ui/panels/FiltersWidget.h"
#include <QScrollArea>
#include <QDebug>
#include <QSettings>

StrategyConfigDialog::StrategyConfigDialog(QWidget* parent)
    : QDialog(parent), m_config{}  // Zero-initialize structure (booleans = false, numbers = 0)
{
    setWindowTitle("Strategy Configuration");
    setMinimumSize(1000, 700);
    
    QSettings settings("fast-backtest-app", "BacktestApp");
    if (settings.contains("StrategyConfigDialog/geometry")) {
        restoreGeometry(settings.value("StrategyConfigDialog/geometry").toByteArray());
    }

    setupUI();
}

StrategyConfigDialog::~StrategyConfigDialog() {
    QSettings settings("fast-backtest-app", "BacktestApp");
    settings.setValue("StrategyConfigDialog/geometry", saveGeometry());
}

void StrategyConfigDialog::setConfig(const StrategyConfig& config) {
    m_config = config;
    
    // Update widgets with the new config
    m_nameEdit->setText(QString::fromStdString(m_config.name));
    m_buyFiltersWidget->setFilters(m_config.buyFilters);
    m_sellFiltersWidget->setFilters(m_config.sellFilters);
    m_resaleFiltersWidget->setFilters(m_config.resaleFilters);
    m_rebuyFiltersWidget->setFilters(m_config.rebuyFilters);
#ifndef DISABLE_LOGGING
    m_enableLoggingCheck->setChecked(m_config.enable_logging);
#endif
}

void StrategyConfigDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Strategy name section at the top
    QGroupBox* nameGroup = new QGroupBox("Strategy Name", this);
    QVBoxLayout* nameLayout = new QVBoxLayout();
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("E.g.: MA Cross, RSI Strategy, etc.");
    m_nameEdit->setText(QString::fromStdString(m_config.name));
    nameLayout->addWidget(m_nameEdit);
    
    nameGroup->setLayout(nameLayout);
    mainLayout->addWidget(nameGroup);
    
    // Create scroll area for the content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    
    // Filters section at the top (2x2 grid)
    QGroupBox* filtersGroup = new QGroupBox("Filters Configuration", scrollContent);
    QGridLayout* filtersLayout = new QGridLayout();
    setupFiltersSection(filtersLayout);
    filtersGroup->setLayout(filtersLayout);
    contentLayout->addWidget(filtersGroup);
    
    // Configuration buttons section
    setupConfigButtonsSection(contentLayout);
    
    // Advanced options section
    setupAdvancedOptionsSection(contentLayout);
    
    contentLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
    
    // Buttons at the bottom (outside scroll area)
    setupButtons(mainLayout);
}

void StrategyConfigDialog::setupFiltersSection(QGridLayout* gridLayout) {
    // Create the 4 filter widgets in a 2x2 grid
    m_buyFiltersWidget = new FiltersWidget(this, "Buy Filters");
    m_sellFiltersWidget = new FiltersWidget(this, "Sell Filters");
    m_resaleFiltersWidget = new FiltersWidget(this, "Resale Filters");
    m_rebuyFiltersWidget = new FiltersWidget(this, "Rebuy Filters");
    
    // Handler for moving filters between widgets
    auto moveHandler = [this](FiltersWidget* source, size_t index, const QString& targetCategory) {
        FiltersWidget* target = nullptr;
        if (targetCategory == "Buy Filters") target = m_buyFiltersWidget;
        else if (targetCategory == "Sell Filters") target = m_sellFiltersWidget;
        else if (targetCategory == "Resale Filters") target = m_resaleFiltersWidget;
        else if (targetCategory == "Rebuy Filters") target = m_rebuyFiltersWidget;
        
        if (target && source && target != source) {
            auto filters = source->getFilters();
            if (index < filters.size()) {
                target->addFilter(filters[index]);
                source->removeFilter(index);
            }
        }
    };

    connect(m_buyFiltersWidget, &FiltersWidget::filterMoveRequested, this, [=](size_t idx, const QString& cat){ moveHandler(m_buyFiltersWidget, idx, cat); });
    connect(m_sellFiltersWidget, &FiltersWidget::filterMoveRequested, this, [=](size_t idx, const QString& cat){ moveHandler(m_sellFiltersWidget, idx, cat); });
    connect(m_resaleFiltersWidget, &FiltersWidget::filterMoveRequested, this, [=](size_t idx, const QString& cat){ moveHandler(m_resaleFiltersWidget, idx, cat); });
    connect(m_rebuyFiltersWidget, &FiltersWidget::filterMoveRequested, this, [=](size_t idx, const QString& cat){ moveHandler(m_rebuyFiltersWidget, idx, cat); });

    gridLayout->addWidget(m_buyFiltersWidget, 0, 0);
    gridLayout->addWidget(m_resaleFiltersWidget, 0, 1);
    gridLayout->addWidget(m_sellFiltersWidget, 1, 0);
    gridLayout->addWidget(m_rebuyFiltersWidget, 1, 1);
    
    // Make columns equal width
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
}

void StrategyConfigDialog::setupConfigButtonsSection(QVBoxLayout* mainLayout) {
    QGroupBox* configGroup = new QGroupBox("Advanced Configuration", this);
    QVBoxLayout* configLayout = new QVBoxLayout();
    
    // Stop Loss / Take Profit button
    m_stopLossTakeProfitButton = new QPushButton("⚙ Stop Loss / Take Profit", this);
    m_stopLossTakeProfitButton->setMinimumHeight(50);
    m_stopLossTakeProfitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
    );
    connect(m_stopLossTakeProfitButton, &QPushButton::clicked, this, &StrategyConfigDialog::openStopLossTakeProfitDialog);
    configLayout->addWidget(m_stopLossTakeProfitButton);
    
    // Risk Management button
    m_riskManagementButton = new QPushButton("🛡 Risk Management", this);
    m_riskManagementButton->setMinimumHeight(50);
    m_riskManagementButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e68900;"
        "}"
    );
    connect(m_riskManagementButton, &QPushButton::clicked, this, &StrategyConfigDialog::openRiskManagementDialog);
    configLayout->addWidget(m_riskManagementButton);

    // Trading Hours button
    m_tradingHoursButton = new QPushButton("🕒 Trading Hours", this);
    m_tradingHoursButton->setMinimumHeight(50);
    m_tradingHoursButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0b7dda;"
        "}"
    );
    connect(m_tradingHoursButton, &QPushButton::clicked, this, &StrategyConfigDialog::openTradingHoursDialog);
    configLayout->addWidget(m_tradingHoursButton);
    
    // ML Configuration button
    m_mlConfigButton = new QPushButton("Machine Learning (ML)", this);
    m_mlConfigButton->setMinimumHeight(50);
    m_mlConfigButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #9C27B0;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #7B1FA2;"
        "}"
    );
    connect(m_mlConfigButton, &QPushButton::clicked, this, &StrategyConfigDialog::openMLConfigDialog);
    configLayout->addWidget(m_mlConfigButton);
    
    configGroup->setLayout(configLayout);
    mainLayout->addWidget(configGroup);
}

void StrategyConfigDialog::openStopLossTakeProfitDialog() {
    StopLossTakeProfitDialog dialog(this);
    dialog.setConfig(m_config);
    
    if (dialog.exec() == QDialog::Accepted) {
        dialog.updateConfig(m_config);
    }
}

void StrategyConfigDialog::openTradingHoursDialog() {
    TradingHoursDialog dialog(this);
    dialog.setConfig(m_config);
    
    if (dialog.exec() == QDialog::Accepted) {
        dialog.updateConfig(m_config);
    }
}

void StrategyConfigDialog::openRiskManagementDialog() {
    RiskManagementDialog dialog(this);
    dialog.setConfig(m_config);
    
    if (dialog.exec() == QDialog::Accepted) {
        dialog.updateConfig(m_config);
    }
}

void StrategyConfigDialog::openMLConfigDialog() {
    MLConfigDialog dialog(this);
    dialog.setConfig(m_config);
    
    if (dialog.exec() == QDialog::Accepted) {
        dialog.updateConfig(m_config);
    }
}

void StrategyConfigDialog::setupAdvancedOptionsSection(QVBoxLayout* mainLayout) {
#ifndef DISABLE_LOGGING
    QGroupBox* advancedGroup = new QGroupBox("Advanced Options", this);
    QVBoxLayout* advancedLayout = new QVBoxLayout();
    
    m_enableLoggingCheck = new QCheckBox("Enable logging", this);
    m_enableLoggingCheck->setChecked(false);
    advancedLayout->addWidget(m_enableLoggingCheck);
    
    advancedGroup->setLayout(advancedLayout);
    mainLayout->addWidget(advancedGroup);
#else
    // In Release mode (DISABLE_LOGGING), hide the advanced options section
    (void)mainLayout; // Avoid unused parameter warning
#endif
}

void StrategyConfigDialog::setupButtons(QVBoxLayout* mainLayout) {
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Save config values from widgets
        m_config.name = m_nameEdit->text().toStdString();
        m_config.buyFilters = m_buyFiltersWidget->getFilters();
        m_config.sellFilters = m_sellFiltersWidget->getFilters();
        m_config.resaleFilters = m_resaleFiltersWidget->getFilters();
        m_config.rebuyFilters = m_rebuyFiltersWidget->getFilters();
#ifndef DISABLE_LOGGING
        m_config.enable_logging = m_enableLoggingCheck->isChecked();
#else
        m_config.enable_logging = false; // Always false in Release mode
#endif
        
        accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}
