#include "ui/views/tradesView.h"
#include <QDebug>

TradesView::TradesView(QWidget* parent)
    : BaseView(parent),
      m_tradesTableWidget(nullptr),
      m_currentResults(nullptr)
{
    setupUI();
}

TradesView::~TradesView()
{
    // Widget cleanup is handled by Qt parent-child mechanism
}

void TradesView::setupUI()
{
    // Create the trades table widget
    m_tradesTableWidget = new TradesTableWidget(this);

    // Add directly to the main layout (no scroll area)
    m_mainLayout->addWidget(m_tradesTableWidget);

    // Connect trade click signal
    connect(m_tradesTableWidget, &TradesTableWidget::tradeClicked,
            this, &TradesView::tradeClicked);

    qDebug() << "TradesView configured";
}

void TradesView::updateData(BacktestResults* results)
{
    m_currentResults = results;
    
    if (!m_currentResults) {
        qWarning() << "TradesView: Null results received";
        clear();
        return;
    }

    // Update the table with the statistics
    if (m_tradesTableWidget) {
        m_tradesTableWidget->updateContent(m_currentResults->stats);
    }

    qDebug() << "TradesView updated with" << m_currentResults->stats.trades.size() << "trades";
}

void TradesView::clear()
{
    qDebug() << "TradesView::clear() called";

    if (m_tradesTableWidget) {
        m_tradesTableWidget->clear();
    }
    
    m_currentResults = nullptr;
}
