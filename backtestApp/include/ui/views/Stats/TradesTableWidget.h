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
    void refreshTable();  // Refresh the table with the current filters
    void showAllTrades(); // Show all trades
    void onTradeRowClicked(const QModelIndex& index);

    void toggleCloseReasonFilter(bool checked);
    void clearFilters();
    void updateFilterButtonText();

private:
    // Interface configuration
    void setupUI();

    // Utility method to filter trades
    std::vector<be::TradeData> getFilteredTrades(const std::vector<be::TradeData>& allTrades);

    // Main container
    QGroupBox* m_tradesGroup;
    QVBoxLayout* m_tradesLayout;

    // Controls for the table
    QTableView* m_tradesTable;
    TradesTableModel* m_tradesModel;
    QComboBox* m_tradesLimitCombo;
    QPushButton* m_showAllTradesBtn;

    // Data
    std::vector<be::TradeData> m_allTrades;

    QPushButton* m_filterBtn;
    QMenu* m_filterMenu;
    QMap<be::CloseReason, bool> m_closeReasonFilters; // Stores the state of the filters
};