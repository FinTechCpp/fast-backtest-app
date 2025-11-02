#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <vector>
#include "common.h"

/**
 * @brief Panel for managing multiple trading strategies
 * 
 * This panel allows adding, removing, duplicating, and configuring multiple strategies.
 * Each strategy has its own independent configuration.
 */
class StrategyPanel : public QGroupBox
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Pointer to the parent widget
     */
    StrategyPanel(QWidget* parent = nullptr);

    /**
     * @brief Retrieves all strategy configurations
     * @return Vector containing all configurations
     */
    std::vector<StrategyConfig> getConfigs() const;

    /**
     * @brief Sets all strategy configurations
     * @param configs Vector of configurations to load
     */
    void setConfigs(const std::vector<StrategyConfig>& configs);

private slots:
    void onAddStrategy();
    void onRemoveStrategy();
    void onDuplicateStrategy();
    void onEditStrategy(QListWidgetItem* item);
    void updateButtonStates();

private:
    void setupUI();
    void refreshStrategyList();

    // UI Components
    QListWidget* m_strategyList;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_duplicateButton;

    // Data
    std::vector<StrategyConfig> m_configs;
};
