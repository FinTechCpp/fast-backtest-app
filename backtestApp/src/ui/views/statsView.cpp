#include "ui/views/statsView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSlice>
#include <QScatterSeries>
#include <QLineSeries>


/*
gantt chart pour les trades
5. Ajouts supplémentaires pour une visualisation complète
Voici quelques idées supplémentaires qui pourraient être intégrées:

Equity curve avec bandes de drawdown: Montrer l'évolution du capital avec des zones colorées indiquant les drawdowns

Analyse des jours/heures de trading: Heatmap montrant les performances par jour de semaine/heure de la journée

Timeline des trades: Visualisation chronologique des trades avec des barres colorées pour TP/SL/BE

Carte de performance vs volatilité: Positionnement de votre stratégie par rapport à d'autres dans un graphique risque/rendement

Indicateurs d'amélioration: Des suggestions visuelles sur les aspects à améliorer dans la stratégie
*/


// Implémentation de StatsView
StatsView::StatsView(QWidget* parent)
    : BaseView(parent),
      m_tradesModel(new TradesTableModel(this)),
      m_app(nullptr)
{
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
    
    setupUI();
}

StatsView::~StatsView()
{
    // Les modèles et widgets sont automatiquement détruits par Qt
}

void StatsView::setupUI() {

    // 1. CRÉATION DE LA STRUCTURE DE BASE
    // --------------------------------------
    // Scroll area principal
    m_scrollStats = new QScrollArea();
    m_scrollStats->setWidgetResizable(true);
    
    // Widget de contenu principal
    m_statsContent = new QWidget();
    
    // Layout principal en grille
    m_statsGridLayout = new QGridLayout(m_statsContent);
    m_statsGridLayout->setSpacing(10);
    
    // Layout pour le contenu additionnel (trades, etc.)
    m_statsContentLayout = new QVBoxLayout();
    
    // Placeholder pour l'état sans données
    m_statsPlaceholder = new QLabel("Exécutez le backtest pour afficher les statistiques");
    m_statsPlaceholder->setAlignment(Qt::AlignCenter);
    m_statsContentLayout->addWidget(m_statsPlaceholder);

    // 2. CRÉATION DES GROUPES DE MÉTRIQUES
    // --------------------------------------
    m_metricsWidget = new MetricsContainerWidget();

    // 3. CRÉATION DU WIDGET TIMELINE
    // --------------------------------------
    m_timelineWidget = new TimelineWidget();
    
    // 4. CRÉATION DU WIDGET EQUITY CURVE
    // --------------------------------------
    m_equityCurveWidget = new EquityCurveWidget();

    // 6. CRÉATION DE LA LÉGENDE
    // --------------------------------------
    QWidget* legendWidget = new QWidget();
    legendWidget->setProperty("isLegend", true);
    m_legendWidget = legendWidget;

    QHBoxLayout* legendLayout = new QHBoxLayout(legendWidget);
    
    QLabel* goodLabel = new QLabel("●");
    goodLabel->setStyleSheet("QLabel { color: #2ecc71; font-size: 16px; }");
    QLabel* goodText = new QLabel("Bon");
    
    QLabel* neutralLabel = new QLabel("●");
    neutralLabel->setStyleSheet("QLabel { color: black; font-size: 16px; }");
    QLabel* neutralText = new QLabel("Neutre");
    
    QLabel* badLabel = new QLabel("●");
    badLabel->setStyleSheet("QLabel { color: #e74c3c; font-size: 16px; }");
    QLabel* badText = new QLabel("Mauvais");
    
    QLabel* naLabel = new QLabel("●");
    naLabel->setStyleSheet("QLabel { color: #7f8c8d; font-size: 16px; }");
    QLabel* naText = new QLabel("N/A");
    
    legendLayout->addWidget(goodLabel);
    legendLayout->addWidget(goodText);
    legendLayout->addSpacing(15);
    legendLayout->addWidget(neutralLabel);
    legendLayout->addWidget(neutralText);
    legendLayout->addSpacing(15);
    legendLayout->addWidget(badLabel);
    legendLayout->addWidget(badText);
    legendLayout->addSpacing(15);
    legendLayout->addWidget(naLabel);
    legendLayout->addWidget(naText);
    legendLayout->addStretch();
    
    // 7. CRÉATION DE LA TABLE DES TRADES ET COMPOSANTS ASSOCIÉS
    // --------------------------------------------------------
    // Groupe pour les trades
    m_tradesGroup = new QGroupBox("Trades Réalisés");
    m_tradesLayout = new QVBoxLayout(m_tradesGroup);
    
    // Widget de répartition des trades (camembert)
    m_tradeClosureWidget = new TradeClosureWidget();
    m_tradesLayout->addWidget(m_tradeClosureWidget);
    
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
    m_tradesTable->verticalHeader()->setVisible(false);
    
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
            this, &StatsView::refreshTradesTable);
    connect(m_showAllTradesBtn, &QPushButton::clicked, [this]() {
        m_tradesLimitCombo->setCurrentIndex(3); // Index pour "Tous"
        refreshTradesTable();
    });
    
    // Ajouter le groupe de trades au layout principal
    m_statsContentLayout->addWidget(m_tradesGroup);
    m_tradesGroup->setVisible(false); // Caché par défaut jusqu'à ce qu'il y ait des données
    
    // 8. ASSEMBLAGE FINAL DANS LA GRILLE
    // --------------------------------------
    int row = 0;
    
    // Ligne 0: Equity Curve (si activé)
    if (m_equityCurveWidget) {
        m_statsGridLayout->addWidget(m_equityCurveWidget, row, 0, 1, 3);
        row++;
    }
    
    // Ligne 1: Timeline
    m_statsGridLayout->addWidget(m_timelineWidget, row, 0, 1, 3);
    row++;
    
    // Ligne 2: Les trois groupes de métriques
    m_statsGridLayout->addWidget(m_metricsWidget, row, 0, 1, 3);
    row++;
    
    // Ligne 3: Légende
    m_statsGridLayout->addWidget(legendWidget, row, 0, 1, 2);
    row++;
    
    // Ligne 4: Contenu additionnel (placeholder, trades, etc.)
    QWidget* placeholderWidget = new QWidget();
    placeholderWidget->setLayout(m_statsContentLayout);
    m_statsGridLayout->addWidget(placeholderWidget, row, 0, 1, 3);
    
    // 9. FINALISATION
    // --------------------------------------
    // Configurer le scroll area et l'ajouter au layout principal
    m_scrollStats->setWidget(m_statsContent);
    m_mainLayout->addWidget(m_scrollStats);
}

void StatsView::updateData(BacktestResults* results)
{    
    // Stocker les résultats pour les mises à jour ultérieures
    m_currentResults = results;
    
    if (!m_currentResults) {
        qWarning() << "Résultats nuls reçus";
        clear();
        return;
    }

    try {
        // Masquer le placeholder
        if (m_statsPlaceholder) {
            m_statsPlaceholder->setVisible(false);
        }
        
        // Afficher le widget de métriques
        if (m_metricsWidget) {
            m_metricsWidget->setGroupsVisible(true);
            m_metricsWidget->updateMetrics(m_currentResults->stats);
        }

        // Mettre à jour la timeline
        if (m_timelineWidget) {
            m_timelineWidget->setData(m_currentResults->stats.start, m_currentResults->stats.end,
                                     m_currentResults->stats.duration, m_currentResults->stats.exposureTimePct);
        }

        // m_equityCurveWidget->updateData(m_currentResults->stats);
        m_tradeClosureWidget->updateData(m_currentResults->stats);

        // Mettre à jour la table des trades
        populateTrades(m_currentResults->stats.trades);


        qInfo() << "StatsView mise à jour avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la mise à jour StatsView:" << e.what();
    }
}

std::vector<be::TradeData> StatsView::getFilteredTrades(const std::vector<be::TradeData>& allTrades) {
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

void StatsView::populateTrades(const std::vector<be::TradeData> &trades)
{
    if (!m_tradesModel) { return; }

    auto filteredTrades = getFilteredTrades(trades);
    m_tradesModel->updateData(filteredTrades);
    
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText(QString("Afficher tous les trades (%1)").arg(trades.size()));
    }
    
    if (m_tradesGroup) {
        m_tradesGroup->setVisible(!trades.empty());
    }
}

void StatsView::refreshTradesTable() {
    qDebug() << "StatsView::refreshTradesTable() appelé";
    
    if (m_app) {
        m_currentResults = m_app->getBacktestResults();
    }
    
    if (!m_tradesModel || !m_currentResults) {
        return;
    }
    
    // Récupérer les trades depuis les résultats
    const auto& allTrades = m_currentResults->stats.trades;
    
    // Utiliser la fonction existante pour filtrer
    auto filteredTrades = getFilteredTrades(allTrades);
    
    // Mettre à jour le modèle
    m_tradesModel->updateData(filteredTrades);
    
    // Mettre à jour le texte du bouton
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText(QString("Afficher tous les trades (%1)").arg(allTrades.size()));
    }
    
    qDebug() << "Table des trades mise à jour avec" << filteredTrades.size() << "/" << allTrades.size() << "trades";
}

void StatsView::clear() {
    qDebug() << "StatsView::clear() appelé";
    
    // Réinitialiser le widget de métriques
    if (m_metricsWidget) {
        m_metricsWidget->clear();
        m_metricsWidget->setGroupsVisible(false);
    }
    
    // Vider le modèle de trades
    if (m_tradesModel) {
        m_tradesModel->clear();
    }

    // Réinitialiser les widgets de graphiques
    // if (m_equityCurveWidget) {
    //     m_equityCurveWidget->clear();
    // }
    
    if (m_tradeClosureWidget) {
        m_tradeClosureWidget->clear();
    }
    
    // Gérer la visibilité des composants
    if (m_statsPlaceholder) {
        m_statsPlaceholder->setText("Exécutez un backtest pour voir les statistiques");
        m_statsPlaceholder->setVisible(true);
    }
    
    // Masquer le groupe des trades
    if (m_tradesGroup) {
        m_tradesGroup->setVisible(false);
    }
    
    m_currentResults = nullptr;
}