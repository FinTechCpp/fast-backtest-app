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
    // Les widgets sont automatiquement détruits par Qt
}

void TradesView::setupUI()
{
    // Créer le tableau des trades
    m_tradesTableWidget = new TradesTableWidget(this);
    
    // Ajouter directement au layout principal (pas de scroll area)
    m_mainLayout->addWidget(m_tradesTableWidget);
    
    // Connecter le signal de clic sur trade
    connect(m_tradesTableWidget, &TradesTableWidget::tradeClicked,
            this, &TradesView::tradeClicked);
    
    qDebug() << "TradesView configurée";
}

void TradesView::updateData(BacktestResults* results)
{
    m_currentResults = results;
    
    if (!m_currentResults) {
        qWarning() << "TradesView: Résultats nuls reçus";
        clear();
        return;
    }
    
    // Mettre à jour le tableau avec les statistiques
    if (m_tradesTableWidget) {
        m_tradesTableWidget->updateContent(m_currentResults->stats);
    }
    
    qDebug() << "TradesView mise à jour avec" << m_currentResults->stats.trades.size() << "trades";
}

void TradesView::clear()
{
    qDebug() << "TradesView::clear() appelé";
    
    if (m_tradesTableWidget) {
        m_tradesTableWidget->clear();
    }
    
    m_currentResults = nullptr;
}
