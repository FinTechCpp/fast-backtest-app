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
    // Groupe pour les trades
    m_tradesGroup = new QGroupBox("Trades Réalisés");
    m_tradesLayout = new QVBoxLayout(m_tradesGroup);
    
    // Layout principal pour ce widget
    QVBoxLayout* containerLayout = new QVBoxLayout(this);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_tradesGroup);
    
    // Contrôles pour la table des trades
    QHBoxLayout* tradesControlsLayout = new QHBoxLayout();
    
    QLabel* tradesInfoLabel = new QLabel("Trades:");
    m_tradesLimitCombo = new QComboBox();
    m_tradesLimitCombo->addItem("50 derniers", 50);
    m_tradesLimitCombo->addItem("100 derniers", 100);
    m_tradesLimitCombo->addItem("200 derniers", 200);
    m_tradesLimitCombo->addItem("Tous", -1);
    m_tradesLimitCombo->setCurrentIndex(0); // 50 par défaut
    
    m_showAllTradesBtn = new QPushButton("Afficher tous les trades");
    
    tradesControlsLayout->addWidget(tradesInfoLabel);
    tradesControlsLayout->addWidget(m_tradesLimitCombo);
    tradesControlsLayout->addWidget(m_showAllTradesBtn);
    tradesControlsLayout->addStretch();

    m_tradesLayout->addLayout(tradesControlsLayout);
    
    // Table des trades
    m_tradesTable = new QTableView();
    m_tradesTable->setModel(m_tradesModel);
    
    // Configuration de la table
    m_tradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tradesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tradesTable->setAlternatingRowColors(true);
    m_tradesTable->setSortingEnabled(true);
    // m_tradesTable->verticalHeader()->setVisible(false);
    
    // Ajuster les colonnes
    QHeaderView* header = m_tradesTable->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::Interactive);
    
    // Définir des largeurs de colonnes
    m_tradesTable->setColumnWidth(0, 10);   // #
    m_tradesTable->setColumnWidth(1, 50);   // Type
    m_tradesTable->setColumnWidth(2, 70);   // Taille
    m_tradesTable->setColumnWidth(3, 120);  // Prix d'entrée
    m_tradesTable->setColumnWidth(4, 120);  // Prix de sortie
    m_tradesTable->setColumnWidth(5, 100);  // PnL
    m_tradesTable->setColumnWidth(6, 80);   // PnL %
    m_tradesTable->setColumnWidth(7, 100);  // Durée
    m_tradesTable->setColumnWidth(8, 150);  // Date d'entrée
    m_tradesTable->setColumnWidth(9, 150);  // Date de sortie
    m_tradesTable->setColumnWidth(10, 100); // Stop Loss initial
    m_tradesTable->setColumnWidth(11, 100); // Take Profit
    m_tradesTable->setColumnWidth(12, 70);  // Clôture
    m_tradesTable->setColumnWidth(13, 80);  // Tag

    // Hauteur de la table
    m_tradesTable->setMaximumHeight(1000);  
    m_tradesTable->setMinimumHeight(400);

    // Style de la table
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
    
    // Connecter les signaux des contrôles de la table
    connect(m_tradesLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TradesTableWidget::refreshTable);
    connect(m_showAllTradesBtn, &QPushButton::clicked, 
            this, &TradesTableWidget::showAllTrades);
    
    // // Cacher par défaut jusqu'à ce qu'il y ait des données
    // m_tradesGroup->setVisible(false);
}

void TradesTableWidget::updateContent(const be::Stats& stats)
{
    // Stocker toutes les trades
    m_allTrades = stats.trades;

    // Appliquer les filtres actuels
    auto filteredTrades = getFilteredTrades(m_allTrades);
    
    // Mettre à jour le modèle avec les trades filtrées
    if (m_tradesModel) {
        m_tradesModel->updateData(filteredTrades);
    }
    
    // Mettre à jour le texte du bouton "Afficher tous"
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText(QString("Afficher tous les trades (%1)").arg(m_allTrades.size()));
    }
    
    // Afficher le groupe s'il y a des trades
    // m_tradesGroup->setVisible(!m_allTrades.empty());
    
    qDebug() << "Table des trades mise à jour avec" << filteredTrades.size() << "/" << m_allTrades.size() << "trades";
}

void TradesTableWidget::clear()
{
    // Vider les données stockées
    m_allTrades.clear();
    
    // Vider le modèle
    if (m_tradesModel) {
        m_tradesModel->clear();
    }
    
    // Cacher le groupe
    // m_tradesGroup->setVisible(false);
    
    // Réinitialiser le texte du bouton
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText("Afficher tous les trades");
    }
}

void TradesTableWidget::refreshTable()
{
    // Ne rien faire s'il n'y a pas de trades ou pas de modèle
    if (m_allTrades.empty() || !m_tradesModel) {
        return;
    }
    
    // Filtrer selon les paramètres actuels
    auto filteredTrades = getFilteredTrades(m_allTrades);
    
    // Mettre à jour le modèle
    m_tradesModel->updateData(filteredTrades);
    
    qDebug() << "Table des trades rafraîchie avec" << filteredTrades.size() << "/" << m_allTrades.size() << "trades";
}

void TradesTableWidget::showAllTrades()
{
    if (m_tradesLimitCombo) {
        m_tradesLimitCombo->setCurrentIndex(3); // Index pour "Tous"
        refreshTable();
    }
}

std::vector<be::TradeData> TradesTableWidget::getFilteredTrades(const std::vector<be::TradeData>& allTrades)
{
    std::vector<be::TradeData> filteredTrades = allTrades;

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