#include "ui/views/Stats/TradesTableWidget.h"
#include <QDebug>
#include <QHeaderView>

TradesTableWidget::TradesTableWidget(QWidget* parent)
    : StatsBaseWidget(parent),
      m_tradesModel(new TradesTableModel(this))
{
    setupUI();
}

void TradesTableWidget::setupUI()
{
    // Group for trades
    m_tradesGroup = new QGroupBox("Realised trades", this);
    m_tradesLayout = new QVBoxLayout(m_tradesGroup);

    // Main layout for this widget
    QVBoxLayout* containerLayout = new QVBoxLayout(this);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_tradesGroup);

    // Controls for the trades table
    QHBoxLayout* tradesControlsLayout = new QHBoxLayout();
    
    QLabel* tradesInfoLabel = new QLabel("Trades:");
    m_tradesLimitCombo = new QComboBox();
    m_tradesLimitCombo->addItem("Last 50", 50);
    m_tradesLimitCombo->addItem("Last 100", 100);
    m_tradesLimitCombo->addItem("Last 200", 200);
    m_tradesLimitCombo->addItem("All", -1);
    m_tradesLimitCombo->setCurrentIndex(0); // 50 by default

    m_showAllTradesBtn = new QPushButton("Show all trades");

    m_filterBtn = new QPushButton("Filters");
    m_filterBtn->setIcon(QIcon::fromTheme("view-filter", QIcon(":/icons/filter.png")));
    
    // Create the filter menu
    m_filterMenu = new QMenu(this);

    // Create actions for each close reason
    QAction* tpAction = new QAction("Trades on TP", this);
    tpAction->setCheckable(true);
    tpAction->setChecked(false);
    tpAction->setData(static_cast<int>(be::CloseReason::TakeProfit));

    QAction* slAction = new QAction("Trades on SL", this);
    slAction->setCheckable(true);
    slAction->setChecked(false);
    slAction->setData(static_cast<int>(be::CloseReason::StopLoss));

    QAction* beAction = new QAction("Trades on BE", this);
    beAction->setCheckable(true);
    beAction->setChecked(false);
    beAction->setData(static_cast<int>(be::CloseReason::BreakEven));

    QAction* manualAction = new QAction("Trades on Manual", this);
    manualAction->setCheckable(true);
    manualAction->setChecked(false);
    manualAction->setData(static_cast<int>(be::CloseReason::ManualClose));

    QAction* unknownAction = new QAction("Trades on Unknown", this);
    unknownAction->setCheckable(true);
    unknownAction->setChecked(false);
    unknownAction->setData(static_cast<int>(be::CloseReason::Unknown));

    // Add actions to the menu
    m_filterMenu->addAction(tpAction);
    m_filterMenu->addAction(slAction);
    m_filterMenu->addAction(beAction);
    m_filterMenu->addAction(manualAction);
    m_filterMenu->addAction(unknownAction);

    // Add a separator
    m_filterMenu->addSeparator();

    // Add an action to clear the filters
    QAction* clearAction = new QAction("Clear", this);
    m_filterMenu->addAction(clearAction);

    // Associate the menu with the button
    m_filterBtn->setMenu(m_filterMenu);

    // Initialize filters (all enabled by default)
    m_closeReasonFilters[be::CloseReason::TakeProfit] = false;
    m_closeReasonFilters[be::CloseReason::StopLoss] = false;
    m_closeReasonFilters[be::CloseReason::BreakEven] = false;
    m_closeReasonFilters[be::CloseReason::ManualClose] = false;
    m_closeReasonFilters[be::CloseReason::Unknown] = false;

    // Connect actions
    connect(tpAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(slAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(beAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(manualAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(unknownAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(clearAction, &QAction::triggered, this, &TradesTableWidget::clearFilters);
    
    
    
    tradesControlsLayout->addWidget(tradesInfoLabel);
    tradesControlsLayout->addWidget(m_tradesLimitCombo);
    tradesControlsLayout->addWidget(m_filterBtn); // Add filter button
    tradesControlsLayout->addWidget(m_showAllTradesBtn);
    tradesControlsLayout->addStretch();

    m_tradesLayout->addLayout(tradesControlsLayout);

    // Trades table
    m_tradesTable = new QTableView();
    m_tradesTable->setModel(m_tradesModel);

    // Table configuration
    m_tradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tradesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tradesTable->setAlternatingRowColors(true);
    m_tradesTable->setSortingEnabled(true);
    m_tradesTable->verticalHeader()->setVisible(false);

    // Adjust columns
    QHeaderView* header = m_tradesTable->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::Interactive);

    // Set column widths
    m_tradesTable->setColumnWidth(0, 60);   // id
    m_tradesTable->setColumnWidth(1, 60);   // Type
    m_tradesTable->setColumnWidth(2, 70);   // Size
    m_tradesTable->setColumnWidth(3, 120);  // Entry Price
    m_tradesTable->setColumnWidth(4, 120);  // Exit Price
    m_tradesTable->setColumnWidth(5, 100);  // PnL
    m_tradesTable->setColumnWidth(6, 80);   // PnL %
    m_tradesTable->setColumnWidth(7, 100);  // Duration
    m_tradesTable->setColumnWidth(8, 150);  // Entry Date
    m_tradesTable->setColumnWidth(9, 150);  // Exit Date
    m_tradesTable->setColumnWidth(10, 100); // Initial Stop Loss
    m_tradesTable->setColumnWidth(11, 100); // Take Profit
    m_tradesTable->setColumnWidth(12, 70);  // Close Reason
    m_tradesTable->setColumnWidth(13, 80);  // Tag

    // Table height
    m_tradesTable->setMaximumHeight(1600);
    m_tradesTable->setMinimumHeight(800);

    // Table style
    QString tableStyle =    
        "QTableView {"
        "    border: 1px solid #d3d3d3;"
        "    border-radius: 5px;"
        "    background-color: #fcfcfc;"
        "    gridline-color: #e0e0e0;"
        "}"
        "QTableView::item {"
        "    padding: 5px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f0f0f0;"
        "    padding: 5px;"
        "    border: 1px solid #d3d3d3;"
        "    font-weight: bold;"
        "}";
    
    m_tradesTable->setStyleSheet(tableStyle);
    m_tradesLayout->addWidget(m_tradesTable);

    // Connect signals from table controls
    connect(m_tradesLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TradesTableWidget::refreshTable);
    connect(m_showAllTradesBtn, &QPushButton::clicked, 
            this, &TradesTableWidget::showAllTrades);

    // Connect row click signal from table
    connect(m_tradesTable, &QTableView::clicked, this, &TradesTableWidget::onTradeRowClicked);

    // Hide by default until there is data
    // m_tradesGroup->setVisible(false);
}

void TradesTableWidget::updateContent(const be::Stats& stats)
{
    // Store all trades
    m_allTrades = stats.trades;

    // Apply current filters
    std::vector<be::TradeData> filteredTrades = getFilteredTrades(m_allTrades);

    // Update the model with the filtered trades
    if (m_tradesModel) 
        m_tradesModel->updateData(filteredTrades);

    // Update the text of the "Show All" button
    if (m_showAllTradesBtn) 
        m_showAllTradesBtn->setText(QString("Show All Trades (%1)").arg(m_allTrades.size()));

    // Show the group if there are trades
    // m_tradesGroup->setVisible(!m_allTrades.empty());
    
    qDebug() << "Trades table updated with" << filteredTrades.size() << "/" << m_allTrades.size() << "trades";
}

void TradesTableWidget::clear()
{
    // Clear stored data
    m_allTrades.clear();

    // Clear the model
    if (m_tradesModel) {
        m_tradesModel->clear();
    }

    // Hide the group
    // m_tradesGroup->setVisible(false);

    // Reset the text of the button
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText("Show All Trades");
    }
}

void TradesTableWidget::refreshTable()
{
    // Do nothing if there are no trades or no model
    if (m_allTrades.empty() || !m_tradesModel) {
        return;
    }

    // Filter according to current settings
    auto filteredTrades = getFilteredTrades(m_allTrades);

    // Update the model
    m_tradesModel->updateData(filteredTrades);

    qDebug() << "Trades table refreshed with" << filteredTrades.size() << "/" << m_allTrades.size() << "trades";
}

void TradesTableWidget::showAllTrades()
{
    if (m_tradesLimitCombo) {
        m_tradesLimitCombo->setCurrentIndex(3); // Index for "All"
        refreshTable();
    }
}

std::vector<be::TradeData> TradesTableWidget::getFilteredTrades(const std::vector<be::TradeData>& allTrades) {
    std::vector<be::TradeData> filteredTrades;

    // Check if at least one filter is active
    bool anyFilterActive = false;
    for (bool active : m_closeReasonFilters.values()) {
        if (active) { anyFilterActive = true; break; }
    }

    if (anyFilterActive) {
        // Add only trades matching active filters
        for (const auto& trade : allTrades) {
            if (m_closeReasonFilters.value(trade.closeReason, false)) {
                filteredTrades.push_back(trade);
            }
        }
    } else {
        // No active filters: show all trades
        filteredTrades = allTrades;
    }

    // Apply display limit
    if (m_tradesLimitCombo) {
        int limit = m_tradesLimitCombo->currentData().toInt();
        if (limit > 0 && filteredTrades.size() > static_cast<size_t>(limit)) {
            filteredTrades = std::vector<be::TradeData>(
                filteredTrades.end() - limit, filteredTrades.end()
            );
        }
    }

    return filteredTrades;
}

void TradesTableWidget::onTradeRowClicked(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    // Get currently displayed filtered trades
    std::vector<be::TradeData> filteredTrades = getFilteredTrades(m_allTrades);
    
    int row = index.row();
    if (row >= 0 && row < static_cast<int>(filteredTrades.size())) {
        const be::TradeData& trade = filteredTrades[row];
        qDebug() << "Trade clicked - Entry:" << trade.entryDate.toString().c_str()
                 << "Exit:" << trade.exitDate.toString().c_str();

        // Emit signal with trade data
        emit tradeClicked(trade);
    }
}

void TradesTableWidget::toggleCloseReasonFilter(bool checked)
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    
    be::CloseReason reason = static_cast<be::CloseReason>(action->data().toInt());
    m_closeReasonFilters[reason] = checked;

    // Update filter button text
    updateFilterButtonText();

    // Refresh table
    refreshTable();
}

void TradesTableWidget::clearFilters()
{
    // Disable all filters
    for (auto& key : m_closeReasonFilters.keys()) {
        m_closeReasonFilters[key] = false;
    }
    // Uncheck all actions in the menu
    for (QAction* action : m_filterMenu->actions()) {
        if (action->isCheckable()) {
            action->setChecked(false);
        }
    }
    updateFilterButtonText();
    refreshTable();
}

void TradesTableWidget::updateFilterButtonText()
{
    int activeFilters = 0;
    for (bool isActive : m_closeReasonFilters.values()) {
        if (isActive) activeFilters++;
    }
    if (activeFilters == 0) {
        m_filterBtn->setText("Filters (disabled)");
    } else {
        m_filterBtn->setText(QString("Filters (%1)").arg(activeFilters));
    }
}