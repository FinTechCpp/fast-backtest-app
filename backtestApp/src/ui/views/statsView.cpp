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
      m_tablesCreated(false),
      m_app(nullptr)
{
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
    
    initializeMetricDefinitions();
    setupUI();
}

StatsView::~StatsView()
{
    // Les modèles et widgets sont automatiquement détruits par Qt
}

void StatsView::setupUI() {
    createScrollAreaAndContent();
    createGroupBoxes();
    createStatsWidgets();
    createLegend();
    // createEquityCurveWidget();
    createTimelineWidget();

    arrangePanels();
    
    // Configurer le scroll area et l'ajouter au layout principal
    m_scrollStats->setWidget(m_statsContent);
    m_mainLayout->addWidget(m_scrollStats);
}

void StatsView::createScrollAreaAndContent() {
    // Créer le scroll area principal
    m_scrollStats = new QScrollArea();
    m_scrollStats->setWidgetResizable(true);
    
    // Créer le widget de contenu
    m_statsContent = new QWidget();
    
    // Remplacer le QVBoxLayout par un QGridLayout pour disposer les groupes en grille 2x2
    m_statsGridLayout = new QGridLayout(m_statsContent);
    m_statsGridLayout->setSpacing(10);

    m_statsContentLayout = new QVBoxLayout();

    // Placeholder initial
    m_statsPlaceholder = new QLabel("Exécutez le backtest pour afficher les statistiques");
    m_statsPlaceholder->setAlignment(Qt::AlignCenter);
    m_statsContentLayout->addWidget(m_statsPlaceholder);
}

void StatsView::createGroupBoxes() {
   // Créer les sections de métriques avec des titres plus descriptifs
    m_timeGroup = new QGroupBox("Période et Exposition");
    m_timeLayout = new QVBoxLayout(); // Changer en QVBoxLayout pour mettre les métriques en colonne
    m_timeGroup->setLayout(m_timeLayout);
    
    m_performanceGroup = new QGroupBox("Résultats et Performance");
    m_performanceLayout = new QVBoxLayout(); // Changer en QVBoxLayout pour mettre les métriques en colonne
    m_performanceGroup->setLayout(m_performanceLayout);
    
    m_riskGroup = new QGroupBox("Mesures de Risque et Volatilité");
    m_riskLayout = new QVBoxLayout(); // Changer en QVBoxLayout pour mettre les métriques en colonne
    m_riskGroup->setLayout(m_riskLayout);
    
    m_generalGroup = new QGroupBox("Statistiques de Trading");
    m_generalLayout = new QVBoxLayout(); // Changer en QVBoxLayout pour mettre les métriques en colonne
    m_generalGroup->setLayout(m_generalLayout);
}

void StatsView::createStatsWidgets() {
    // Map des sections aux layouts correspondants
    static const QMap<QString, QVBoxLayout*> sectionLayouts = {
        // {"time", m_timeLayout},
        {"performance", m_performanceLayout},
        {"risk", m_riskLayout},
        {"general", m_generalLayout}
    };
    
    // Créer tous les widgets de métriques à partir des définitions
    for (const auto& metric : m_metricDefinitions) {

        if (metric.section == "time") continue;

        // Récupérer le layout correspondant à la section
        QVBoxLayout* targetLayout = sectionLayouts.value(metric.section);
        if (!targetLayout) continue;
        
        // Créer le widget et l'ajouter au layout
        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 5, 5, 5);
        
        MetricWidget* widget = new MetricWidget(metric.label, "N/A");
        widget->setTooltip(metric.tooltip);
        rowLayout->addWidget(widget, 1);
        
        targetLayout->addWidget(row);
        m_metricWidgets[metric.key] = widget;
    }
}

void StatsView::createLegend() {
    // Ajouter une légende pour les couleurs
    QWidget* legendWidget = new QWidget();
    legendWidget->setProperty("isLegend", true);

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

    // m_statsGridLayout->addWidget(legendWidget, 3, 0, 1, 2); // Span sur 2 colonnes
    m_legendWidget = legendWidget;
}

void StatsView::arrangePanels() {
    // Nettoyer d'abord toutes les positions dans le layout
    // pour éviter les conflits
    while (m_statsGridLayout->count()) {
        QLayoutItem* item = m_statsGridLayout->takeAt(0);
        if (item) {
            // Ne pas supprimer le widget, juste l'item
            delete item;
        }
    }
    
    // Ajouter les widgets dans l'ordre souhaité
    int row = 0;
    
    // Ligne 0: Equity Curve (tout en haut)
    // if (m_equityCurveWidget) {
    //     m_statsGridLayout->addWidget(m_equityCurveWidget, row, 0, 1, 3);
    //     row++;
    // }
    
    // Ligne 1: Timeline
    if (m_timeGroup) {
        m_statsGridLayout->addWidget(m_timeGroup, row, 0, 1, 3);
        row++;
    }
    
    // Ligne 2: Les trois groupes de métriques
    if (m_performanceGroup && m_riskGroup && m_generalGroup) {
        m_statsGridLayout->addWidget(m_performanceGroup, row, 0);
        m_statsGridLayout->addWidget(m_riskGroup, row, 1);
        m_statsGridLayout->addWidget(m_generalGroup, row, 2);
        row++;
    }
    
    // Ligne 3: Légende
    QLayoutItem* legendItem = nullptr;
    for (int i = 0; i < m_statsGridLayout->count(); ++i) {
        QLayoutItem* item = m_statsGridLayout->itemAt(i);
        if (item && item->widget() && 
            item->widget()->property("isLegend").toBool()) {
            legendItem = item;
            break;
        }
    }
    
    // Si la légende existe, la placer sur la ligne suivante
    if (legendItem) {
        m_statsGridLayout->addWidget(legendItem->widget(), row, 0, 1, 2);
        row++;
    }
    
    // Ligne 4: Contenu additionnel (placeholder, trades, etc.)
    QWidget* placeholderWidget = new QWidget();
    placeholderWidget->setLayout(m_statsContentLayout);
    m_statsGridLayout->addWidget(placeholderWidget, row, 0, 1, 3);
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
        // Créer les tables si ce n'est pas déjà fait
        if (!m_tablesCreated) {
            createTradesTable();
            m_tablesCreated = true;
        }
        
        // Masquer le placeholder
        if (m_statsPlaceholder) {
            m_statsPlaceholder->setVisible(false);
        }
        
        // Afficher les sections
        if (m_performanceGroup) m_performanceGroup->setVisible(true);
        if (m_riskGroup) m_riskGroup->setVisible(true);
        if (m_generalGroup) m_generalGroup->setVisible(true);
        
        // Mettre à jour les métriques avec l'objet Stats
        populateMetrics(m_currentResults->stats);

        // m_equityCurveWidget->updateData(m_currentResults->stats);
        m_tradeClosureWidget->updateData(m_currentResults->stats);

        // Mettre à jour la table des trades
        populateTrades(m_currentResults->stats.trades);


        qInfo() << "StatsView mise à jour avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la mise à jour StatsView:" << e.what();
    }
}

void StatsView::initializeMetricDefinitions() {
    m_metricDefinitions = {
        // Section temporelle
        {"start", "Début:", "Date de début du backtest", "time",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.start.toString()); }
        },
        {"end", "Fin:", "Date de fin du backtest", "time",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.end.toString()); }
        },
        {"duration", "Durée:", "Durée totale du backtest", "time",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.duration.toString()); }
        },
        {"exposure_time", "Temps en position:", "Pourcentage du temps avec des positions ouvertes", "time",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.exposureTimePct, 'f', 2)); }
        },
        
        // Section performance
        {"equity_final", "Capital final:", "Montant final du capital", "performance",
            [](const be::Stats& s) { 
                return s.equityFinal > s.equityInitial ? MetricStatus::Good : 
                      (s.equityFinal < s.equityInitial ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) { return QString("$%1").arg(QString::number(s.equityFinal, 'f', 2)); }
        },
        {"equity_peak", "Capital maximal:", "Montant maximal atteint par le capital", "performance",
            [](const be::Stats& s) { 
                return s.equityPeak > s.equityInitial ? MetricStatus::Good : 
                (s.equityPeak < s.equityInitial ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) { return QString("$%1").arg(QString::number(s.equityPeak, 'f', 2)); }
        },
        {"total_return", "Rendement total:", "Pourcentage de gain/perte sur l'ensemble du backtest", "performance",
            [](const be::Stats& s) { 
                return s.returnPct > 0 ? MetricStatus::Good : 
                      (s.returnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.returnPct, 'f', 2)); }
        },
        {"buy_hold_return", "Buy & Hold:", "Rendement d'une stratégie passive d'achat et maintien", "performance",
            [](const be::Stats& s) { 
                return s.buyHoldReturnPct > 0 ? MetricStatus::Good : 
                      (s.buyHoldReturnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.buyHoldReturnPct, 'f', 2)); }
        },
        {"return_ann", "Rendement annualisé:", "Rendement annuel équivalent", "performance",
            [](const be::Stats& s) { 
                return s.returnAnnPct > 0 ? MetricStatus::Good : 
                      (s.returnAnnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.returnAnnPct, 'f', 2)); }
        },
        {"cagr", "CAGR:", "Taux de croissance annuel composé", "performance",
            [](const be::Stats& s) { 
                return s.cagrPct > 0 ? MetricStatus::Good : 
                      (s.cagrPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.cagrPct, 'f', 2)); }
        },
        {"alpha", "Alpha:", "Surperformance par rapport au marché (ajustée au risque)", "performance",
            [](const be::Stats& s) { 
                return s.alphaPct > 0 ? MetricStatus::Good : 
                      (s.alphaPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.alphaPct, 'f', 2)); }
        },
        {"beta", "Beta:", "Corrélation avec les mouvements du marché", "performance",
            [](const be::Stats& s) {
                if (std::isnan(s.beta)) return MetricStatus::NA;
                return s.beta < 0.8 ? MetricStatus::Good : 
                      (s.beta > 1.2 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.beta) ? QString("N/A") : QString::number(s.beta, 'f', 2);
            }
        },
        
        // Section risque
        {"max_drawdown", "Drawdown maximal:", "Perte maximale depuis un sommet précédent", "risk",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.maxDrawdownPct, 'f', 2)); }
        },
        {"avg_drawdown", "Drawdown moyen:", "Perte moyenne depuis un sommet précédent", "risk",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.avgDrawdownPct, 'f', 2)); }
        },
        {"max_drawdown_duration", "Durée DD max:", "Durée de la plus longue période de drawdown", "risk",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.maxDrawdownDuration.toString()); }
        },
        {"avg_drawdown_duration", "Durée DD moyenne:", "Durée moyenne des périodes de drawdown", "risk",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.avgDrawdownDuration.toString()); }
        },
        {"sharpe_ratio", "Ratio de Sharpe:", "Rendement excédentaire par unité de risque total", "risk",
            [](const be::Stats& s) {
                if (std::isnan(s.sharpeRatio)) return MetricStatus::NA;
                return s.sharpeRatio > 1 ? MetricStatus::Good : 
                      (s.sharpeRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.sharpeRatio) ? QString("N/A") : QString::number(s.sharpeRatio, 'f', 2);
            }
        },
        {"sortino_ratio", "Ratio de Sortino:", "Rendement excédentaire par unité de risque négatif", "risk",
            [](const be::Stats& s) {
                if (std::isnan(s.sortinoRatio)) return MetricStatus::NA;
                return s.sortinoRatio > 1 ? MetricStatus::Good : 
                      (s.sortinoRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.sortinoRatio) ? QString("N/A") : QString::number(s.sortinoRatio, 'f', 2);
            }
        },
        {"calmar_ratio", "Ratio de Calmar:", "Rendement annualisé divisé par le drawdown maximal", "risk",
            [](const be::Stats& s) {
                if (std::isnan(s.calmarRatio)) return MetricStatus::NA;
                return s.calmarRatio > 1 ? MetricStatus::Good : 
                      (s.calmarRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.calmarRatio) ? QString("N/A") : QString::number(s.calmarRatio, 'f', 2);
            }
        },
        {"volatility", "Volatilité annualisée:", "Mesure de la variabilité des rendements", "risk",
            [](const be::Stats& s) { 
                return s.volatilityAnnPct < 10 ? MetricStatus::Good : 
                      (s.volatilityAnnPct > 25 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.volatilityAnnPct, 'f', 2)); }
        },
        
        // Section général (trades)
        {"total_trades", "Nombre de trades:", "Nombre total de transactions effectuées", "general",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::number(s.numTrades); }
        },
        {"profit_factor", "Facteur de profit:", "Ratio des gains sur les pertes (>1 est profitable)", "general",
            [](const be::Stats& s) {
                if (std::isnan(s.profitFactor)) return MetricStatus::NA;
                return s.profitFactor > 1.1 ? MetricStatus::Good : 
                      (s.profitFactor < 0.9 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.profitFactor) ? QString("N/A") : QString::number(s.profitFactor, 'f', 2);
            }
        },
        {"TP_trades", "Trades sur take-profit:", "Nombre de trades sur take-profit", "general",
            [](const be::Stats& s) { return MetricStatus::Good; },
            [](const be::Stats& s) { return QString::number(s.pctTPTrades, 'f', 1) + "% (" + QString::number(s.numTPTrades) + ")"; }
        },
        {"SL_trades", "Trades sur stop-loss:", "Nombre de trades sur stop-loss", "general",
            [](const be::Stats& s) { 
                return s.numSLTrades <= s.numTPTrades ? MetricStatus::Neutral : MetricStatus::Bad; 
            },
            [](const be::Stats& s) { return QString::number(s.pctSLTrades, 'f', 1) + "% (" + QString::number(s.numSLTrades) + ")"; }
        },
        {"BE_trades", "Trades sur break-even:", "Nombre de trades sur break-even", "general",
            [](const be::Stats& s) { return s.numBETrades <= s.numTPTrades ? MetricStatus::Neutral : MetricStatus::Bad; },
            [](const be::Stats& s) { return QString::number(s.pctBETrades, 'f', 1) + "% (" + QString::number(s.numBETrades) + ")"; }
        },
        // ajouter les metric : numManualTrades et numUnknownTrades
        {"manual_trades", "Trades manuels:", "Nombre de trades manuels", "general",
            [](const be::Stats& s) { return s.numManualTrades > 0 ? MetricStatus::Neutral : MetricStatus::Good; },
            [](const be::Stats& s) { return QString::number(s.pctManualTrades, 'f', 1) + "% (" + QString::number(s.numManualTrades) + ")"; }
        },
        {"unknown_trades", "Trades inconnus:", "Nombre de trades avec raison de clôture inconnue", "general",
            [](const be::Stats& s) { return s.numUnknownTrades > 0 ? MetricStatus::Bad : MetricStatus::Good; },
            [](const be::Stats& s) { return QString::number(s.pctUnknownTrades, 'f', 1) + "% (" + QString::number(s.numUnknownTrades) + ")"; }
        },
        {"best_trade", "Meilleur trade:", "Pourcentage de gain du meilleur trade", "general",
            [](const be::Stats& s) { return MetricStatus::Good; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.bestTradePct, 'f', 2)); }
        },
        {"worst_trade", "Pire trade:", "Pourcentage de perte du pire trade", "general",
            [](const be::Stats& s) { return MetricStatus::Bad; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.worstTradePct, 'f', 2)); }
        },
        {"avg_trade", "Trade moyen:", "Rendement moyen par trade", "general",
            [](const be::Stats& s) { 
                return s.avgTradePct > 0 ? MetricStatus::Good : 
                      (s.avgTradePct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.avgTradePct, 'f', 2)); }
        },
        {"max_trade_duration", "Durée max trade:", "Durée maximale d'un trade", "general",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.maxTradeDuration.toString()); }
        },
        {"avg_trade_duration", "Durée moy trade:", "Durée moyenne d'un trade", "general",
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.avgTradeDuration.toString()); }
        },
        {"expectancy", "Espérance:", "Gain moyen attendu par trade", "general",
            [](const be::Stats& s) { 
                return s.expectancyPct > 0 ? MetricStatus::Good : 
                      (s.expectancyPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.expectancyPct, 'f', 2)); }
        },
        {"sqn", "SQN:", "System Quality Number - qualité du système de trading", "general",
            [](const be::Stats& s) {
                if (std::isnan(s.sqn)) return MetricStatus::NA;
                return s.sqn > 2 ? MetricStatus::Good : 
                      (s.sqn < 1 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.sqn) ? QString("N/A") : QString::number(s.sqn, 'f', 2);
            }
        },
        {"kelly_criterion", "Critère de Kelly:", "Taille de position optimale selon le critère de Kelly", "general",
            [](const be::Stats& s) {
                if (std::isnan(s.kellyCriterion)) return MetricStatus::NA;
                return s.kellyCriterion > 0 ? MetricStatus::Good : 
                      (s.kellyCriterion < -0.5 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.kellyCriterion) ? QString("N/A") : QString::number(s.kellyCriterion, 'f', 2);
            }
        }
    };
}

void StatsView::populateMetrics(const be::Stats& stats) {
    qDebug() << "Population des métriques avec coloration conditionnelle";
    
    // Mettre à jour la timeline avec les données du backtest
    if (m_timelineWidget) {
        m_timelineWidget->setData(stats.start, stats.end, stats.duration, stats.exposureTimePct);
    }

    for (const auto& metric : m_metricDefinitions) {
        if (m_metricWidgets.contains(metric.key)) {
            QString value = metric.formatValue(stats);
            MetricStatus status = metric.getStatus(stats);
            m_metricWidgets[metric.key]->updateValues(value, status);
        }
    }
    
    qDebug() << "Population des métriques terminée";
}

MetricWidget* StatsView::createMetricWidget(const QString& key, const QString& label, 
                                          const QString& value, QHBoxLayout* layout)
{
    if (!layout) {
        qWarning() << "Layout null pour la création du widget métrique:" << key;
        return nullptr;
    }
    
    qDebug() << "Création du widget métrique:" << key;
    
    MetricWidget* widget = new MetricWidget(label, value);
    layout->addWidget(widget, 1);
    
    // Stocker le widget dans la map
    m_metricWidgets[key] = widget;
    
    qDebug() << "Widget métrique créé et stocké:" << key;
    
    return widget;
}

void StatsView::createTradesTable() {
    if (m_tablesCreated) return;
    
    m_tradesGroup = new QGroupBox("Trades Réalisés");
    m_tradesLayout = new QVBoxLayout(m_tradesGroup);
    
    // 1. D'abord créer et ajouter le graphique de distribution des rendements
    
    // 2. Ensuite créer et ajouter le graphique camembert de clôture des trades
    createTradeClosureWidget();

    createTradesTableControls();
    createTradesTableView();
    setupTradesConnections();
    
    // Ajouter le groupe à la layout principale
    m_statsContentLayout->addWidget(m_tradesGroup);
    m_tradesGroup->setVisible(false);
    
    m_tablesCreated = true;
}

void StatsView::createTradesTableControls() {
    // Créer les contrôles de la table
    QHBoxLayout* tradesControlsLayout = new QHBoxLayout();
    
    // ComboBox pour limiter le nombre de trades affichés
    m_tradesLimitCombo = new QComboBox();
    m_tradesLimitCombo->addItem("50 derniers", 50);
    m_tradesLimitCombo->addItem("100 derniers", 100);
    m_tradesLimitCombo->addItem("200 derniers", 200);
    m_tradesLimitCombo->addItem("Tous", -1);
    m_tradesLimitCombo->setCurrentIndex(0); // 50 par défaut
    
    // Bouton pour afficher tous les trades
    m_showAllTradesBtn = new QPushButton("Afficher tous les trades");
    
    // Label informatif
    QLabel* tradesInfoLabel = new QLabel("Trades:");
    
    tradesControlsLayout->addWidget(tradesInfoLabel);
    tradesControlsLayout->addWidget(m_tradesLimitCombo);
    tradesControlsLayout->addWidget(m_showAllTradesBtn);
    tradesControlsLayout->addStretch();

    m_tradesLayout->addLayout(tradesControlsLayout);
}

void StatsView::createTradesTableView() {
    // Créer la table des trades
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
    
    // Définir des largeurs minimales pour certaines colonnes
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

    // Améliorer le style des tableaux
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
    
    // Ajouter les widgets au layout
    m_tradesLayout->addWidget(m_tradesTable);

    m_tradesTable->setStyleSheet(tableStyle);
}

void StatsView::setupTradesConnections() {
    // Connecter les signaux
    connect(m_tradesLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsView::refreshTradesTable);
    connect(m_showAllTradesBtn, &QPushButton::clicked, [this]() {
        m_tradesLimitCombo->setCurrentIndex(3); // Index pour "Tous"
        refreshTradesTable();
    });
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

void StatsView::createTradeClosureWidget() {
    m_tradeClosureWidget = new TradeClosureWidget();
    
    // Ajouter au groupe de trades, près du haut
    m_tradesLayout->insertWidget(1, m_tradeClosureWidget);
}

void StatsView::createEquityCurveWidget() {
    m_equityCurveWidget = new EquityCurveWidget();
}

void StatsView::createTimelineWidget()
{
    m_timelineWidget = new TimelineWidget();
    
    // Ajouter un titre
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(10, 10, 10, 10);
    
    QLabel* titleLabel = new QLabel("Période et exposition du backtest");
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    
    layout->addWidget(titleLabel);
    layout->addWidget(m_timelineWidget);
    
    // Remplacer le layout existant du groupe
    delete m_timeGroup->layout();
    m_timeGroup->setLayout(layout);
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
    
    // Réinitialiser widgets de métriques
    for (auto widget : m_metricWidgets) {
        widget->updateValues("N/A");
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
    
    // Masquer les groupes
    for (QGroupBox* group : {m_performanceGroup, m_riskGroup, m_generalGroup, m_tradesGroup}) {
        if (group) group->setVisible(false);
    }
    
    m_currentResults = nullptr;
}