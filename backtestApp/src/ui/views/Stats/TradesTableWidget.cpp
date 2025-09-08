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

    // NOUVEAU: Bouton de filtrage par close reason
    m_filterBtn = new QPushButton("Filtres");
    m_filterBtn->setIcon(QIcon::fromTheme("view-filter", QIcon(":/icons/filter.png")));
    
    // Créer le menu de filtres
    m_filterMenu = new QMenu(this);
    
    // Créer les actions pour chaque close reason
    QAction* tpAction = new QAction("Trades sur TP", this);
    tpAction->setCheckable(true);
    tpAction->setChecked(false);
    tpAction->setData(static_cast<int>(be::CloseReason::TakeProfit));
    
    QAction* slAction = new QAction("Trades sur SL", this);
    slAction->setCheckable(true);
    slAction->setChecked(false);
    slAction->setData(static_cast<int>(be::CloseReason::StopLoss));
    
    QAction* beAction = new QAction("Trades sur BE", this);
    beAction->setCheckable(true);
    beAction->setChecked(false);
    beAction->setData(static_cast<int>(be::CloseReason::BreakEven));
    
    QAction* manualAction = new QAction("Trades manuels", this);
    manualAction->setCheckable(true);
    manualAction->setChecked(false);
    manualAction->setData(static_cast<int>(be::CloseReason::ManualClose));
    
    QAction* unknownAction = new QAction("Trades indéterminés", this);
    unknownAction->setCheckable(true);
    unknownAction->setChecked(false);
    unknownAction->setData(static_cast<int>(be::CloseReason::Unknown));

    // Ajouter les actions au menu
    m_filterMenu->addAction(tpAction);
    m_filterMenu->addAction(slAction);
    m_filterMenu->addAction(beAction);
    m_filterMenu->addAction(manualAction);
    m_filterMenu->addAction(unknownAction);

    // Ajouter un séparateur
    m_filterMenu->addSeparator();

    // Ajouter une action pour réinitialiser les filtres
    QAction* clearAction = new QAction("Clear", this);
    m_filterMenu->addAction(clearAction);

    // Associer le menu au bouton
    m_filterBtn->setMenu(m_filterMenu);
    
    // Initialiser les filtres (tous activés par défaut)
    m_closeReasonFilters[be::CloseReason::TakeProfit] = false;
    m_closeReasonFilters[be::CloseReason::StopLoss] = false;
    m_closeReasonFilters[be::CloseReason::BreakEven] = false;
    m_closeReasonFilters[be::CloseReason::ManualClose] = false;
    m_closeReasonFilters[be::CloseReason::Unknown] = false;

    // Connecter les actions
    connect(tpAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(slAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(beAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(manualAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(unknownAction, &QAction::toggled, this, &TradesTableWidget::toggleCloseReasonFilter);
    connect(clearAction, &QAction::triggered, this, &TradesTableWidget::clearFilters);
    
    
    
    tradesControlsLayout->addWidget(tradesInfoLabel);
    tradesControlsLayout->addWidget(m_tradesLimitCombo);
    tradesControlsLayout->addWidget(m_filterBtn); // Ajouter le bouton de filtre
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
    m_tradesTable->verticalHeader()->setVisible(false);
    
    // Ajuster les colonnes
    QHeaderView* header = m_tradesTable->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::Interactive);
    
    // Définir des largeurs de colonnes
    m_tradesTable->setColumnWidth(0, 60);   // id
    m_tradesTable->setColumnWidth(1, 60);   // Type
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
    m_tradesTable->setMaximumHeight(1600);  
    m_tradesTable->setMinimumHeight(800);

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
    
    // Connecter le signal de clic sur les lignes du tableau
    connect(m_tradesTable, &QTableView::clicked, this, &TradesTableWidget::onTradeRowClicked);
    
    // // Cacher par défaut jusqu'à ce qu'il y ait des données
    // m_tradesGroup->setVisible(false);
}

void TradesTableWidget::updateContent(const be::Stats& stats)
{
    // Stocker toutes les trades
    m_allTrades = stats.trades;

    // Appliquer les filtres actuels
    std::vector<be::TradeData> filteredTrades = getFilteredTrades(m_allTrades);
    
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

std::vector<be::TradeData> TradesTableWidget::getFilteredTrades(const std::vector<be::TradeData>& allTrades) {
    std::vector<be::TradeData> filteredTrades;

    // Vérifier si au moins un filtre est actif
    bool anyFilterActive = false;
    for (bool active : m_closeReasonFilters.values()) {
        if (active) { anyFilterActive = true; break; }
    }

    if (anyFilterActive) {
        // Ajouter uniquement les trades correspondant aux filtres actifs
        for (const auto& trade : allTrades) {
            if (m_closeReasonFilters.value(trade.closeReason, false)) {
                filteredTrades.push_back(trade);
            }
        }
    } else {
        // Aucun filtre actif : tout afficher
        filteredTrades = allTrades;
    }

    // Appliquer la limite d'affichage
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
    
    // Obtenir les trades filtrés actuellement affichés
    std::vector<be::TradeData> filteredTrades = getFilteredTrades(m_allTrades);
    
    int row = index.row();
    if (row >= 0 && row < static_cast<int>(filteredTrades.size())) {
        const be::TradeData& trade = filteredTrades[row];
        qDebug() << "Trade cliqué - Entrée:" << trade.entryDate.toString().c_str() 
                 << "Sortie:" << trade.exitDate.toString().c_str();
        
        // Émettre le signal avec les données du trade
        emit tradeClicked(trade);
    }
}

void TradesTableWidget::toggleCloseReasonFilter(bool checked)
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    
    be::CloseReason reason = static_cast<be::CloseReason>(action->data().toInt());
    m_closeReasonFilters[reason] = checked;
    
    // Mettre à jour le texte du bouton de filtre
    updateFilterButtonText();
    
    // Rafraîchir la table
    refreshTable();
}

void TradesTableWidget::clearFilters()
{
    // Désactiver tous les filtres
    for (auto& key : m_closeReasonFilters.keys()) {
        m_closeReasonFilters[key] = false;
    }
    // Décoche toutes les actions du menu
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
        m_filterBtn->setText("Filtres (désactivés)");
    } else {
        m_filterBtn->setText(QString("Filtres (%1)").arg(activeFilters));
    }
}