#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include "ui/views/Stats/TradesTableModel.h"
#include "stats.hpp"

class TradesTableWidget : public QWidget {
    Q_OBJECT

public:
    explicit TradesTableWidget(QWidget* parent = nullptr);
    
    // API simple pour mettre à jour et effacer les données
    void updateData(const std::vector<be::TradeData>& trades);
    void clear();

    // Getter pour le GroupBox pour contrôler sa visibilité si nécessaire
    QGroupBox* groupBox() const { return m_tradesGroup; }

private slots:
    void refreshTable();  // Rafraîchit la table avec les filtres actuels
    void showAllTrades(); // Afficher tous les trades

private:
    // Configuration de l'interface
    void setupUI();

    // Méthode utilitaire pour filtrer les trades
    std::vector<be::TradeData> getFilteredTrades(const std::vector<be::TradeData>& allTrades);

    // Conteneur principal
    QGroupBox* m_tradesGroup;
    QVBoxLayout* m_tradesLayout;

    // Contrôles pour la table
    QTableView* m_tradesTable;
    TradesTableModel* m_tradesModel;
    QComboBox* m_tradesLimitCombo;
    QPushButton* m_showAllTradesBtn;

    // Données
    std::vector<be::TradeData> m_allTrades;
};