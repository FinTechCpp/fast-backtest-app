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
    
    // Layout principal
    m_statsLayout = new QVBoxLayout(m_statsContent);
    m_statsLayout->setSpacing(10);
    
    // Layout pour le contenu additionnel (trades, etc.)
    m_statsContentLayout = new QVBoxLayout();

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
    // Widget de répartition des trades (camembert)
    m_tradeClosureWidget = new TradeClosureWidget();
    m_statsContentLayout->addWidget(m_tradeClosureWidget);

    m_tradesTableWidget = new TradesTableWidget();
    m_tradesTableWidget->clear();
    m_statsContentLayout->addWidget(m_tradesTableWidget);
    
    // 8. ASSEMBLAGE FINAL DANS LA GRILLE
    // --------------------------------------    
    // Ligne 0: Equity Curve (si activé)
    if (m_equityCurveWidget) {
        m_statsLayout->addWidget(m_equityCurveWidget);
    }
    
    // Ligne 1: Timeline
    m_statsLayout->addWidget(m_timelineWidget);
    
    // Ligne 2: Les trois groupes de métriques
    m_statsLayout->addWidget(m_metricsWidget);
    
    // Ligne 3: Légende
    m_statsLayout->addWidget(legendWidget);
    
    // Ligne 4: Contenu additionnel (placeholder, trades, etc.)
    QWidget* placeholderWidget = new QWidget();
    placeholderWidget->setLayout(m_statsContentLayout);
    m_statsLayout->addWidget(placeholderWidget);

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

        // Mise à jour des widgets des trades
        if (m_tradeClosureWidget) {
            m_tradeClosureWidget->updateData(m_currentResults->stats);
        }

        // Remplacer populateTrades par:
        if (m_tradesTableWidget) {
            m_tradesTableWidget->updateData(m_currentResults->stats.trades);
        }


        qInfo() << "StatsView mise à jour avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la mise à jour StatsView:" << e.what();
    }
}

void StatsView::clear() {
    qDebug() << "StatsView::clear() appelé";
    
    // Réinitialiser le widget de métriques
    if (m_metricsWidget) {
        m_metricsWidget->clear();
    }
    
    if (m_tradeClosureWidget) {
        m_tradeClosureWidget->clear();
    }

    if (m_tradesTableWidget) {
        m_tradesTableWidget->clear();
    }
    
    if (m_tradeClosureWidget) {
        m_tradeClosureWidget->clear();
    }
    
    m_currentResults = nullptr;
}