#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <vector>
#include "common.h"

/**
 * @brief Panel pour la gestion de plusieurs stratégies de trading
 * 
 * Ce panel permet d'ajouter, supprimer, dupliquer et configurer plusieurs stratégies.
 * Chaque stratégie possède sa propre configuration indépendante.
 */
class StrategyPanel : public QGroupBox
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    StrategyPanel(QWidget* parent = nullptr);

    /**
     * @brief Récupère toutes les configurations de stratégies
     * @return Vecteur contenant toutes les configurations
     */
    std::vector<StrategyConfig> getConfigs() const;

    /**
     * @brief Définit toutes les configurations de stratégies
     * @param configs Vecteur de configurations à charger
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
