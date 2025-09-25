#include "ui/panels/strategySpecificPanels/GenericStrategyPanel.h"
#include "ui/dialogs/StrategyCreationDialog.h"
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QLabel>


GenericStrategyPanel::GenericStrategyPanel(QWidget* parent)
    : ConfigPanel<GenericStrategyConfig>("Stratégies Personnalisées", parent)
{
    setupUI();
}

void GenericStrategyPanel::setupUI()
{
    QWidget* parent = qobject_cast<QWidget*>(this);

    QVBoxLayout* strategyLayout = new QVBoxLayout(this);
    strategyLayout->setSpacing(15);
    strategyLayout->setContentsMargins(10, 15, 10, 15);

    // Strategy creation section
    QGroupBox* creationGroup = new QGroupBox("Gestion des stratégies", this);
    QVBoxLayout* creationLayout = new QVBoxLayout(creationGroup);
    
    // Current strategy info
    QHBoxLayout* currentStrategyLayout = new QHBoxLayout();
    QLabel* currentLabel = new QLabel("Stratégie actuelle:", this);
    m_currentStrategyLabel = new QLabel("Aucune stratégie chargée", this);
    m_currentStrategyLabel->setStyleSheet("font-weight: bold; color: #2c5aa0;");
    currentStrategyLayout->addWidget(currentLabel);
    currentStrategyLayout->addWidget(m_currentStrategyLabel);
    currentStrategyLayout->addStretch();
    creationLayout->addLayout(currentStrategyLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_createStrategyBtn = new QPushButton("Créer nouvelle stratégie", this);
    m_createStrategyBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );
    
    m_editStrategyBtn = new QPushButton("Modifier stratégie", this);
    m_editStrategyBtn->setEnabled(false);
    m_editStrategyBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    );

    m_clearStrategyBtn = new QPushButton("Vider stratégie", this);
    m_clearStrategyBtn->setEnabled(false);
    m_clearStrategyBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #f44336;"
        "  color: white;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #d32f2f;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    );
    
    buttonLayout->addWidget(m_createStrategyBtn);
    buttonLayout->addWidget(m_editStrategyBtn);
    buttonLayout->addWidget(m_clearStrategyBtn);
    buttonLayout->addStretch();
    
    creationLayout->addLayout(buttonLayout);
    strategyLayout->addWidget(creationGroup);

    // Filters display section
    QGroupBox* filtersGroup = new QGroupBox("Filtres de la stratégie", this);
    QVBoxLayout* filtersLayout = new QVBoxLayout(filtersGroup);
    
    m_filtersListWidget = new QListWidget(this);
    m_filtersListWidget->setMinimumHeight(200);
    m_filtersListWidget->setAlternatingRowColors(true);
    filtersLayout->addWidget(m_filtersListWidget);
    
    QLabel* filtersInfoLabel = new QLabel("Les filtres de la stratégie chargée s'afficheront ici.", this);
    filtersInfoLabel->setStyleSheet("color: #888; font-style: italic;");
    filtersLayout->addWidget(filtersInfoLabel);
    
    strategyLayout->addWidget(filtersGroup);

    // Add stretch to push everything to the top
    strategyLayout->addStretch();

    // Connect signals
    connect(m_createStrategyBtn, &QPushButton::clicked, this, &GenericStrategyPanel::onCreateStrategy);
    connect(m_editStrategyBtn, &QPushButton::clicked, this, &GenericStrategyPanel::onEditStrategy);
    connect(m_clearStrategyBtn, &QPushButton::clicked, this, &GenericStrategyPanel::onClearStrategy);
    
    // Update UI based on current config
    updateUI();
}

void GenericStrategyPanel::onCreateStrategy()
{
    StrategyCreationDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        GenericStrategyConfig newConfig = dialog.getStrategyConfig();
        
        // Update the current configuration
        updateConfig(newConfig);
        
        // Update the UI to reflect the new strategy
        updateUI();
        
        // Emit signal to notify that config has changed
        emit configChanged();
    }
}

void GenericStrategyPanel::onEditStrategy()
{
    StrategyCreationDialog dialog(m_config, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        GenericStrategyConfig updatedConfig = dialog.getStrategyConfig();
        
        // Update the current configuration
        updateConfig(updatedConfig);
        
        // Update the UI to reflect the modified strategy
        updateUI();
        
        // Emit signal to notify that config has changed
        emit configChanged();
    }
}

void GenericStrategyPanel::onClearStrategy()
{
    int result = QMessageBox::question(this, "Vider la stratégie", 
                                     "Êtes-vous sûr de vouloir vider la stratégie actuelle ?\n"
                                     "Tous les filtres seront supprimés.",
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        // Create empty strategy config
        GenericStrategyConfig emptyConfig;
        emptyConfig.name = "";
        emptyConfig.go_direction = std::nullopt;
        emptyConfig.filters.clear();
        
        // Initialize with default values for backward compatibility
        emptyConfig.ema_short_period = 20;
        emptyConfig.ema_long_period = 200;
        emptyConfig.stoch_fastk = 14;
        emptyConfig.stoch_slowk = 3;
        emptyConfig.stoch_slowd = 3;
        emptyConfig.stoch_threshold = 20;
        emptyConfig.rsi_period = 14;
        emptyConfig.rsi_threshold = 30;
        emptyConfig.supertrend_atr_period = 10;
        emptyConfig.supertrend_multiplier = 3.0;
        emptyConfig.previous_ha_candle_red_filter_n = 3;
        emptyConfig.rsi_history_periods = 3;
        emptyConfig.stoch_history_periods = 3;
        emptyConfig.atr_filter_period = 14;
        emptyConfig.atr_threshold = 0.01;
        emptyConfig.atr_history_periods = 3;
        
        // All hardcoded filters disabled
        emptyConfig.use_ema_short_filter = false;
        emptyConfig.use_ema_long_filter = false;
        emptyConfig.use_stoch_filter = false;
        emptyConfig.use_rsi_filter = false;
        emptyConfig.use_previous_ha_candle_red_filter = false;
        emptyConfig.use_supertrend_filter = false;
        emptyConfig.use_atr_filter = false;
        
        // Update the configuration
        updateConfig(emptyConfig);
        
        // Update UI
        updateUI();
        
        // Emit signal to notify that config has changed
        emit configChanged();
    }
}

void GenericStrategyPanel::updateUI()
{
    // Update current strategy label
    if (m_config.name.empty() || m_config.filters.empty()) {
        m_currentStrategyLabel->setText("Aucune stratégie chargée");
        m_currentStrategyLabel->setStyleSheet("font-weight: bold; color: #888;");
        m_editStrategyBtn->setEnabled(false);
        m_clearStrategyBtn->setEnabled(false);
    } else {
        QString directionText;
        if (m_config.go_direction.has_value()) {
            directionText = m_config.go_direction.value() ? " (Long)" : " (Short)";
        } else {
            directionText = " (Long/Short)";
        }
        
        m_currentStrategyLabel->setText(QString::fromStdString(m_config.name) + directionText);
        m_currentStrategyLabel->setStyleSheet("font-weight: bold; color: #2c5aa0;");
        m_editStrategyBtn->setEnabled(true);
        m_clearStrategyBtn->setEnabled(true);
    }
    
    // Update filters list
    m_filtersListWidget->clear();
    
    if (m_config.filters.empty()) {
        QListWidgetItem* item = new QListWidgetItem("Aucun filtre configuré");
        item->setForeground(QColor::fromRgb(136, 136, 136));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_filtersListWidget->addItem(item);
    } else {
        for (size_t i = 0; i < m_config.filters.size(); ++i) {
            const GenericFilter& filter = m_config.filters[i];
            
            QString itemText = QString("Filtre %1: %2")
                                 .arg(i + 1)
                                 .arg(QString::fromStdString(filter.description));
            
            QListWidgetItem* item = new QListWidgetItem(itemText);
            
            if (!filter.enabled) {
                itemText += " (désactivé)";
                item->setText(itemText);
                item->setForeground(QColor::fromRgb(136, 136, 136));
            } else {
                item->setForeground(QColor::fromRgb(0, 0, 0));
            }
            
            m_filtersListWidget->addItem(item);
        }
    }
}

void GenericStrategyPanel::updateConfig(const GenericStrategyConfig& newConfig)
{
    m_config = newConfig;
}