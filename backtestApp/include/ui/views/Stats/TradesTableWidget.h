#pragma once

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QMenu>
#include "ui/views/Stats/TradesTableModel.h"
#include "ui/views/Stats/StatsBaseWidget.h"

class TradesTableWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit TradesTableWidget(QWidget* parent = nullptr);
    
    void updateContent(const be::Stats& stats) override;
    void clear() override;

signals:
    void tradeClicked(const be::TradeData& trade);

private slots:
    void refreshTable();  // Rafraîchit la table avec les filtres actuels
    void showAllTrades(); // Afficher tous les trades
    void onTradeRowClicked(const QModelIndex& index);

    void toggleCloseReasonFilter(bool checked);
    void clearFilters();
    void updateFilterButtonText();

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

    QPushButton* m_filterBtn;
    QMenu* m_filterMenu;
    QMap<be::CloseReason, bool> m_closeReasonFilters; // Stocke l'état des filtres
};