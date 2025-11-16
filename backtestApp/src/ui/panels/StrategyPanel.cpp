#include "ui/panels/StrategyPanel.h"
#include "ui/dialogs/StrategyConfigDialog.h"
#include <QDebug>
#include <QMessageBox>
#include <QLabel>

StrategyPanel::StrategyPanel(QWidget* parent)
    : QGroupBox("Trading Strategies", parent)
{
    // Add a default strategy
    StrategyConfig defaultConfig{};  // Zero-initialize all fields
    m_configs.push_back(defaultConfig);
    
    setupUI();
    refreshStrategyList();
}

void StrategyPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Strategy list with improved styling
    m_strategyList = new QListWidget(this);
    m_strategyList->setMinimumHeight(150);
    m_strategyList->setAlternatingRowColors(true);
    m_strategyList->setStyleSheet(
        "QListWidget {"
        "   border: 2px solid #d0d0d0;"
        "   border-radius: 6px;"
        "   background-color: #ffffff;"
        "   padding: 4px;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   padding: 10px 12px;"
        "   margin: 2px;"
        "   border: 1px solid transparent;"
        "   border-radius: 4px;"
        "   background-color: #fafafa;"
        "   color: #333333;"
        "   font-size: 13px;"
        "   font-weight: 500;"
        "}"
        "QListWidget::item:alternate {"
        "   background-color: #f5f5f5;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #0078d4;"
        "   color: #ffffff;"
        "   border: 1px solid #005a9e;"
        "   font-weight: 600;"
        "}"
        "QListWidget::item:hover:!selected {"
        "   background-color: #e8f4fd;"
        "   border: 1px solid #b8d6ed;"
        "   color: #333333;"
        "}"
    );
    
    connect(m_strategyList, &QListWidget::itemDoubleClicked, 
            this, &StrategyPanel::onEditStrategy);
    connect(m_strategyList, &QListWidget::itemSelectionChanged,
            this, &StrategyPanel::updateButtonStates);
    
    mainLayout->addWidget(m_strategyList);
    
    // Management buttons with improved styling
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    
    m_addButton = new QPushButton("➕ Add", this);
    m_addButton->setToolTip("Add a new strategy");
    m_addButton->setMinimumHeight(36);
    m_addButton->setCursor(Qt::PointingHandCursor);
    m_addButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-weight: bold;"
        "   font-size: 13px;"
        "   padding: 8px 20px;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
    );
    connect(m_addButton, &QPushButton::clicked, this, &StrategyPanel::onAddStrategy);
    
    m_duplicateButton = new QPushButton("📋 Duplicate", this);
    m_duplicateButton->setToolTip("Duplicate the selected strategy");
    m_duplicateButton->setMinimumHeight(36);
    m_duplicateButton->setCursor(Qt::PointingHandCursor);
    m_duplicateButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-weight: bold;"
        "   font-size: 13px;"
        "   padding: 8px 20px;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F57C00;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #E65100;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #888888;"
        "}"
    );
    connect(m_duplicateButton, &QPushButton::clicked, this, &StrategyPanel::onDuplicateStrategy);
    
    m_removeButton = new QPushButton("🗑️ Remove", this);
    m_removeButton->setToolTip("Remove the selected strategy");
    m_removeButton->setMinimumHeight(36);
    m_removeButton->setCursor(Qt::PointingHandCursor);
    m_removeButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #f44336;"
        "   color: white;"
        "   font-weight: bold;"
        "   font-size: 13px;"
        "   padding: 8px 20px;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #da190b;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #c41c0c;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #888888;"
        "}"
    );
    connect(m_removeButton, &QPushButton::clicked, this, &StrategyPanel::onRemoveStrategy);
    
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_duplicateButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Informational label (improved)
    QLabel* infoLabel = new QLabel("💡 Double-click a strategy to edit it", this);
    infoLabel->setStyleSheet(
        "QLabel {"
        "   color: #555555;"
        "   font-size: 12px;"
        "   font-style: italic;"
        "   padding: 8px;"
        "   background-color: #f0f8ff;"
        "   border-left: 3px solid #0078d4;"
        "   border-radius: 3px;"
        "}"
    );
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
    
    setLayout(mainLayout);
    updateButtonStates();
}

void StrategyPanel::onAddStrategy() {
    // Create a new default config
    StrategyConfig newConfig{};  // Zero-initialize all fields
    for (int i = 0; i < 7; ++i) 
        newConfig.trading_days_array[i] = (i < 5);
    // Open dialog to configure
    StrategyConfigDialog dialog(this);
    dialog.setConfig(newConfig);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_configs.push_back(dialog.getConfig());
        refreshStrategyList();
        
        // Select the new strategy
        m_strategyList->setCurrentRow(static_cast<int>(m_configs.size()) - 1);
    }
}

void StrategyPanel::onRemoveStrategy() {
    int currentRow = m_strategyList->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_configs.size())) {
        return;
    }
    
    // Prevent deletion of the last strategy
    if (m_configs.size() == 1) {
        QMessageBox::warning(this, "Cannot remove",
                           "You must keep at least one strategy.");
        return;
    }
    
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm deletion",
        "Are you sure you want to delete this strategy?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_configs.erase(m_configs.begin() + currentRow);
        refreshStrategyList();
        
        // Select the previous or next strategy
        if (currentRow > 0) {
            m_strategyList->setCurrentRow(currentRow - 1);
        } else if (!m_configs.empty()) {
            m_strategyList->setCurrentRow(0);
        }
    }
}

void StrategyPanel::onDuplicateStrategy() {
    int currentRow = m_strategyList->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_configs.size())) {
        return;
    }
    
    // Duplicate the configuration
    StrategyConfig duplicatedConfig = m_configs[currentRow];
    m_configs.push_back(duplicatedConfig);
    refreshStrategyList();
    
    // Select the new duplicated strategy
    m_strategyList->setCurrentRow(static_cast<int>(m_configs.size()) - 1);
}

void StrategyPanel::onEditStrategy(QListWidgetItem* item) {
    if (!item) return;
    
    int row = m_strategyList->row(item);
    if (row < 0 || row >= static_cast<int>(m_configs.size())) {
        return;
    }
    
    // Open configuration dialog
    StrategyConfigDialog dialog(this);
    dialog.setConfig(m_configs[row]);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_configs[row] = dialog.getConfig();
        refreshStrategyList();
        m_strategyList->setCurrentRow(row);
    }
}

void StrategyPanel::updateButtonStates() {
    bool hasSelection = m_strategyList->currentRow() >= 0;
    bool canRemove = hasSelection && m_configs.size() > 1;
    
    m_duplicateButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(canRemove);
}

void StrategyPanel::refreshStrategyList() {
    m_strategyList->clear();
    
    for (size_t i = 0; i < m_configs.size(); ++i) {
        QString displayName = QString::fromStdString(m_configs[i].name);
        
        // If the name is empty, show a default name
        if (displayName.trimmed().isEmpty()) {
            displayName = QString("Strategy %1").arg(i + 1);
        }
        
        // Add an icon to make the display nicer
        QString itemText = QString("📊 %1").arg(displayName);
        m_strategyList->addItem(itemText);
    }
    
    updateButtonStates();
}

std::vector<StrategyConfig> StrategyPanel::getConfigs() const {
    return m_configs;
}

void StrategyPanel::setConfigs(const std::vector<StrategyConfig>& configs) {
    if (configs.empty()) {
        qWarning() << "Attempt to set an empty strategy list - ignored";
        return;
    }
    
    m_configs = configs;
    refreshStrategyList();
    
    if (!m_configs.empty()) {
        m_strategyList->setCurrentRow(0);
    }
}
