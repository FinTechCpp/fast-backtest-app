#include "ui/views/Stats/MetricsContainerWidget.h"
#include <QHBoxLayout>
#include <QDebug>

MetricsContainerWidget::MetricsContainerWidget(QWidget* parent)
    : StatsBaseWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);

    // Créer les groupes
    m_performanceGroup = new QGroupBox("Résultats et Performance");
    m_performanceLayout = new QVBoxLayout(m_performanceGroup);
    
    m_riskGroup = new QGroupBox("Mesures de Risque et Volatilité");
    m_riskLayout = new QVBoxLayout(m_riskGroup);
    
    m_generalGroup = new QGroupBox("Statistiques de Trading");
    m_generalLayout = new QVBoxLayout(m_generalGroup);
    
    // Layout horizontal pour contenir les trois groupes
    QHBoxLayout* groupsLayout = new QHBoxLayout();
    groupsLayout->setContentsMargins(0, 0, 0, 0);
    groupsLayout->addWidget(m_performanceGroup);
    groupsLayout->addWidget(m_riskGroup);
    groupsLayout->addWidget(m_generalGroup);
    m_mainLayout->addLayout(groupsLayout);

    // Légende pour les statuts des métriques
    QWidget* legendWidget = new QWidget();

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

    m_mainLayout->addWidget(legendWidget);
    
    // Initialiser les définitions de métriques pour cette section
    initializeMetrics();
    
    // Créer les widgets pour chaque section
    createMetricWidgets(Section::Performance, m_performanceLayout);
    createMetricWidgets(Section::Risk, m_riskLayout);
    createMetricWidgets(Section::General, m_generalLayout);
}

void MetricsContainerWidget::createMetricWidgets(Section section, QVBoxLayout* layout) {
    // Créer les widgets pour la section spécifiée
    for (const auto& metric : m_metricDefinitions) {
        if (metric.section != section) continue;
        
        // Créer un widget pour contenir chaque métrique
        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 5, 5, 5);
        
        // Créer le widget de métrique
        MetricWidget* widget = new MetricWidget(metric.label, "N/A");
        widget->setTooltip(metric.tooltip);
        rowLayout->addWidget(widget, 1);
        
        // Ajouter le widget à notre layout
        layout->addWidget(row);
        
        // Stocker une référence au widget de métrique
        m_metricWidgets[metric.key] = widget;
    }
    
    // Ajouter un espace extensible à la fin pour permettre un alignement vertical en haut
    layout->addStretch();
}

void MetricsContainerWidget::initializeMetrics() {
    m_metricDefinitions = {
        // Section temporelle
        // {"start", "Début:", "Date de début du backtest", "time",
        //     [](const be::Stats& s) { return MetricStatus::Neutral; },
        //     [](const be::Stats& s) { return QString::fromStdString(s.start.toString()); }
        // },
        // {"end", "Fin:", "Date de fin du backtest", "time",
        //     [](const be::Stats& s) { return MetricStatus::Neutral; },
        //     [](const be::Stats& s) { return QString::fromStdString(s.end.toString()); }
        // },
        // {"duration", "Durée:", "Durée totale du backtest", "time",
        //     [](const be::Stats& s) { return MetricStatus::Neutral; },
        //     [](const be::Stats& s) { return QString::fromStdString(s.duration.toString()); }
        // },
        // {"exposure_time", "Temps en position:", "Pourcentage du temps avec des positions ouvertes", "time",
        //     [](const be::Stats& s) { return MetricStatus::Neutral; },
        //     [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.exposureTimePct, 'f', 2)); }
        // },
        
        // Section performance
        {"equity_final", "Capital final:", "Montant final du capital", Section::Performance,
            [](const be::Stats& s) { 
                return s.equityFinal > s.equityInitial ? MetricStatus::Good : 
                      (s.equityFinal < s.equityInitial ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) { return QString("$%1").arg(QString::number(s.equityFinal, 'f', 2)); }
        },
        {"equity_peak", "Capital maximal:", "Montant maximal atteint par le capital", Section::Performance,
            [](const be::Stats& s) { 
                return s.equityPeak > s.equityInitial ? MetricStatus::Good : 
                (s.equityPeak < s.equityInitial ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) { return QString("$%1").arg(QString::number(s.equityPeak, 'f', 2)); }
        },
        {"total_return", "Rendement total:", "Pourcentage de gain/perte sur l'ensemble du backtest", Section::Performance,
            [](const be::Stats& s) { 
                return s.returnPct > 0 ? MetricStatus::Good : 
                      (s.returnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.returnPct, 'f', 2)); }
        },
        {"buy_hold_return", "Buy & Hold:", "Rendement d'une stratégie passive d'achat et maintien", Section::Performance,
            [](const be::Stats& s) { 
                return MetricStatus::Neutral; 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.buyHoldReturnPct, 'f', 2)); }
        },
        {
            "buy_hold_cagr", "Buy & Hold CAGR:", "CAGR d'une stratégie passive d'achat et maintien", Section::Performance,
            [](const be::Stats& s) {
                return MetricStatus::Neutral;
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.buyHoldCagrPct, 'f', 2)); }
        },
        {"return_ann", "Rendement annualisé:", "Rendement annuel équivalent", Section::Performance,
            [](const be::Stats& s) { 
                return s.returnAnnPct > 0 ? MetricStatus::Good : 
                      (s.returnAnnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.returnAnnPct, 'f', 2)); }
        },
        {"cagr", "CAGR:", "Taux de croissance annuel composé", Section::Performance,
            [](const be::Stats& s) { 
                return s.cagrPct > 0 ? MetricStatus::Good : 
                      (s.cagrPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.cagrPct, 'f', 2)); }
        },
        {"alpha", "Alpha:", "Surperformance par rapport au marché (ajustée au risque)", Section::Performance,
            [](const be::Stats& s) { 
                return s.alphaPct > 0 ? MetricStatus::Good : 
                      (s.alphaPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.alphaPct, 'f', 2)); }
        },
        {"beta", "Beta:", "Corrélation avec les mouvements du marché", Section::Performance,
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
        // {"max_drawdown", "Drawdown maximal:", "Perte maximale depuis un sommet précédent", Section::Risk,
        //     [](const be::Stats& s) { return MetricStatus::Neutral; },
        //     [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.maxDrawdownPct, 'f', 2)); }
        // },
        {"avg_drawdown", "Drawdown moyen:", "Perte moyenne depuis un sommet précédent", Section::Risk,
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.avgDrawdownPct, 'f', 2)); }
        },
        {"max_drawdown_duration", "Durée DD max:", "Durée de la plus longue période de drawdown", Section::Risk,
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.maxDrawdownDuration.toString()); }
        },
        {"avg_drawdown_duration", "Durée DD moyenne:", "Durée moyenne des périodes de drawdown", Section::Risk,
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.avgDrawdownDuration.toString()); }
        },
        {"sharpe_ratio", "Ratio de Sharpe:", "Rendement excédentaire par unité de risque total", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.sharpeRatio)) return MetricStatus::NA;
                return s.sharpeRatio > 1 ? MetricStatus::Good : 
                    (s.sharpeRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.sharpeRatio) ? QString("N/A") : QString::number(s.sharpeRatio, 'f', 2);
            }
        },
        {"sortino_ratio", "Ratio de Sortino:", "Rendement excédentaire par unité de risque négatif", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.sortinoRatio)) return MetricStatus::NA;
                return s.sortinoRatio > 1 ? MetricStatus::Good : 
                      (s.sortinoRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.sortinoRatio) ? QString("N/A") : QString::number(s.sortinoRatio, 'f', 2);
            }
        },
        {"calmar_ratio", "Ratio de Calmar:", "Rendement annualisé divisé par le drawdown maximal", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.calmarRatio)) return MetricStatus::NA;
                return s.calmarRatio > 1 ? MetricStatus::Good : 
                      (s.calmarRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.calmarRatio) ? QString("N/A") : QString::number(s.calmarRatio, 'f', 2);
            }
        },
        {"volatility", "Volatilité annualisée:", "Mesure de la variabilité des rendements", Section::Risk,
            [](const be::Stats& s) { 
                return s.volatilityAnnPct < 10 ? MetricStatus::Good : 
                      (s.volatilityAnnPct > 25 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.volatilityAnnPct, 'f', 2)); }
        },
        
        // Section général (trades)
        {"total_trades", "Nombre de trades:", "Nombre total de transactions effectuées", Section::General,
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::number(s.numTrades); }
        },
        {"profit_factor", "Facteur de profit:", "Ratio des gains sur les pertes (>1 est profitable)", Section::General,
            [](const be::Stats& s) {
                if (std::isnan(s.profitFactor)) return MetricStatus::NA;
                return s.profitFactor > 1.1 ? MetricStatus::Good : 
                      (s.profitFactor < 0.9 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.profitFactor) ? QString("N/A") : QString::number(s.profitFactor, 'f', 2);
            }
        },
        // {"TP_trades", "Trades sur take-profit:", "Nombre de trades sur take-profit", Section::General,
        //     [](const be::Stats& s) { return MetricStatus::Good; },
        //     [](const be::Stats& s) { return QString::number(s.pctTPTrades, 'f', 1) + "% (" + QString::number(s.numTPTrades) + ")"; }
        // },
        // {"SL_trades", "Trades sur stop-loss:", "Nombre de trades sur stop-loss", Section::General,
        //     [](const be::Stats& s) { 
        //         return s.numSLTrades <= s.numTPTrades ? MetricStatus::Neutral : MetricStatus::Bad; 
        //     },
        //     [](const be::Stats& s) { return QString::number(s.pctSLTrades, 'f', 1) + "% (" + QString::number(s.numSLTrades) + ")"; }
        // },
        // {"BE_trades", "Trades sur break-even:", "Nombre de trades sur break-even", Section::General,
        //     [](const be::Stats& s) { return s.numBETrades <= s.numTPTrades ? MetricStatus::Neutral : MetricStatus::Bad; },
        //     [](const be::Stats& s) { return QString::number(s.pctBETrades, 'f', 1) + "% (" + QString::number(s.numBETrades) + ")"; }
        // },
        // // ajouter les metric : numManualTrades et numUnknownTrades
        // {"manual_trades", "Trades manuels:", "Nombre de trades manuels", Section::General,
        //     [](const be::Stats& s) { return s.numManualTrades > 0 ? MetricStatus::Neutral : MetricStatus::Good; },
        //     [](const be::Stats& s) { return QString::number(s.pctManualTrades, 'f', 1) + "% (" + QString::number(s.numManualTrades) + ")"; }
        // },
        // {"unknown_trades", "Trades inconnus:", "Nombre de trades avec raison de clôture inconnue", Section::General,
        //     [](const be::Stats& s) { return s.numUnknownTrades > 0 ? MetricStatus::Bad : MetricStatus::Good; },
        //     [](const be::Stats& s) { return QString::number(s.pctUnknownTrades, 'f', 1) + "% (" + QString::number(s.numUnknownTrades) + ")"; }
        // },
        {"best_trade", "Meilleur trade:", "Pourcentage de gain du meilleur trade", Section::General,
            [](const be::Stats& s) { return MetricStatus::Good; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.bestTradePct, 'f', 2)); }
        },
        {"worst_trade", "Pire trade:", "Pourcentage de perte du pire trade", Section::General,
            [](const be::Stats& s) { return MetricStatus::Bad; },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.worstTradePct, 'f', 2)); }
        },
        {"avg_trade", "Trade moyen:", "Rendement moyen par trade", Section::General,
            [](const be::Stats& s) {
                return s.avgTradePct > 0 ? MetricStatus::Good : 
                      (s.avgTradePct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.avgTradePct, 'f', 2)); }
        },
        {"max_trade_duration", "Durée max trade:", "Durée maximale d'un trade", Section::General,
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.maxTradeDuration.toString()); }
        },
        {"avg_trade_duration", "Durée moy trade:", "Durée moyenne d'un trade", Section::General,
            [](const be::Stats& s) { return MetricStatus::Neutral; },
            [](const be::Stats& s) { return QString::fromStdString(s.avgTradeDuration.toString()); }
        },
        // {"expectancy", "Espérance:", "Gain moyen attendu par trade", Section::General,
        //     [](const be::Stats& s) {
        //         return s.expectancyPct > 0 ? MetricStatus::Good : 
        //               (s.expectancyPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral); 
        //     },
        //     [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.expectancyPct, 'f', 2)); }
        // },
        {"sqn", "SQN:", "System Quality Number - qualité du système de trading", Section::General,
            [](const be::Stats& s) {
                if (std::isnan(s.sqn)) return MetricStatus::NA;
                return s.sqn > 2 ? MetricStatus::Good : 
                      (s.sqn < 1 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.sqn) ? QString("N/A") : QString::number(s.sqn, 'f', 2);
            }
        },
        {"kelly_criterion", "Critère de Kelly:", "Taille de position optimale selon le critère de Kelly", Section::General,
            [](const be::Stats& s) {
                if (std::isnan(s.kellyCriterion)) return MetricStatus::NA;
                return s.kellyCriterion > 0 ? MetricStatus::Good : 
                      (s.kellyCriterion < -0.5 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.kellyCriterion) ? QString("N/A") : QString::number(s.kellyCriterion, 'f', 2);
            }
        },
        {"ulcer_index", "Ulcer Index:", "Mesure de la profondeur et durée des drawdowns", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.ulcerIndex)) return MetricStatus::NA;
                return s.ulcerIndex < 5.0 ? MetricStatus::Good : 
                      (s.ulcerIndex > 10.0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.ulcerIndex) ? QString("N/A") : QString::number(s.ulcerIndex, 'f', 2);
            }
        },
        {"ulcer_performance", "UPI:", "Ratio rendement/Ulcer Index", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.ulcerPerformanceIndex)) return MetricStatus::NA;
                return s.ulcerPerformanceIndex > 1.0 ? MetricStatus::Good : 
                      (s.ulcerPerformanceIndex < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.ulcerPerformanceIndex) ? QString("N/A") : QString::number(s.ulcerPerformanceIndex, 'f', 2);
            }
        },
        {"avg_mae", "MAE moyen:", "Perte maximale moyenne pendant les trades", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.avgMAE)) return MetricStatus::NA;
                return s.avgMAE < 1.0 ? MetricStatus::Good : 
                      (s.avgMAE > 3.0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.avgMAE) ? QString("N/A") : QString("%1%").arg(QString::number(s.avgMAE, 'f', 2));
            }
        },
        {"max_mae", "MAE max:", "Pire perte temporaire pendant un trade", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.maxMAE)) return MetricStatus::NA;
                return s.maxMAE < 2.0 ? MetricStatus::Good : 
                      (s.maxMAE > 5.0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.maxMAE) ? QString("N/A") : QString("%1%").arg(QString::number(s.maxMAE, 'f', 2));
            }
        },
        {"skewness", "Skewness:", "Asymétrie de la distribution des rendements", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.skewness)) return MetricStatus::NA;
                return s.skewness > 0.1 ? MetricStatus::Good : 
                      (s.skewness < -0.1 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.skewness) ? QString("N/A") : QString::number(s.skewness, 'f', 2);
            }
        },
        {"kurtosis", "Kurtosis:", "Mesure des événements extrêmes dans les rendements", Section::Risk,
            [](const be::Stats& s) {
                if (std::isnan(s.kurtosis)) return MetricStatus::NA;
                // Une distribution normale a un kurtosis de 3, donc on évalue par rapport à ça
                // Un kurtosis élevé (>3) signifie des queues épaisses (plus d'événements extrêmes)
                return s.kurtosis < 3.0 ? MetricStatus::Good : 
                      (s.kurtosis > 5.0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.kurtosis) ? QString("N/A") : QString::number(s.kurtosis, 'f', 2);
            }
        },
        
        // Section performance - Ajouter à la fin des métriques de performance
        {"omega_ratio", "Ratio d'Omega:", "Ratio rendements positifs/négatifs pondérés", Section::Performance,
            [](const be::Stats& s) {
                if (std::isnan(s.omegaRatio)) return MetricStatus::NA;
                return s.omegaRatio > 1.2 ? MetricStatus::Good : 
                      (s.omegaRatio < 1.0 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.omegaRatio) ? QString("N/A") : QString::number(s.omegaRatio, 'f', 2);
            }
        }
    };
}


void MetricsContainerWidget::updateContent(const be::Stats& stats) {
    // Mettre à jour toutes les métriques
    for (auto it = m_metricWidgets.begin(); it != m_metricWidgets.end(); ++it) {
        QString key = it.key();
        MetricWidget* widget = it.value();
        
        // Trouver la définition correspondante
        auto metricIt = std::find_if(m_metricDefinitions.begin(), m_metricDefinitions.end(),
            [key](const MetricDefinition& def) { return def.key == key; });
        
        if (metricIt != m_metricDefinitions.end()) {
            // Récupérer la valeur formatée et le statut
            QString value = metricIt->formatValue(stats);
            MetricStatus status = metricIt->getStatus(stats);
            
            // Mettre à jour le widget
            widget->updateValues(value, status);
        }
    }
}

void MetricsContainerWidget::clear() {
    // Réinitialiser toutes les métriques
    for (auto widget : m_metricWidgets) {
        widget->updateValues("N/A", MetricStatus::NA);
    }
}
