#include "ui/views/statsView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>

// Utilitaire pour convertir be::Date en QDateTime
QDateTime TradesTableModel::dateToQDateTime(const be::Date& date) {
    // Utiliser les getters publics au lieu d'accéder directement aux membres privés
    return QDateTime(
        QDate(static_cast<int>(date.getYear()), 
              static_cast<int>(date.getMonth()), 
              static_cast<int>(date.getDay())),
        QTime(static_cast<int>(date.getHour()), 
              static_cast<int>(date.getMinute()), 
              static_cast<int>(date.getSecond()))
    );
}

// Implémentation de TradesTableModel
TradesTableModel::TradesTableModel(QObject* parent)
    : QStandardItemModel(parent)
{
    // Définir les en-têtes par défaut
    QStringList headers;
    headers << "#" << "Date" << "Type" << "Entrée" << "Sortie" << "Dur." << "PnL" << "PnL%" << "SL" << "TP";
    setHorizontalHeaderLabels(headers);
}

void TradesTableModel::clear() {
    removeRows(0, rowCount());
}

// Nouvelle implémentation pour travailler avec des be::Trade
void TradesTableModel::updateData(const std::vector<std::shared_ptr<be::Trade>>& trades)
{
    beginResetModel();
    
    // Effacer les données existantes
    removeRows(0, rowCount());
    
    if (trades.empty()) {
        endResetModel();
        return;
    }
    
    // Configurer les en-têtes
    QStringList headers;
    headers << "#" << "Type" << "Taille" << "Prix d'entrée" << "Prix de sortie" 
            << "PnL" << "PnL %" << "Durée" << "Date d'entrée" << "Date de sortie"
            << "SL initial" << "TP" << "B.E." << "Tag";  // Renommé "SL" en "SL initial" et ajouté "B.E."
    setHorizontalHeaderLabels(headers);
    
    // Ajouter les nouvelles données
    setRowCount(static_cast<int>(trades.size()));
    
    for (int row = 0; row < static_cast<int>(trades.size()); ++row) {
        const auto& trade = trades[row];
        if (!trade) continue;
        
        // # (numéro de trade)
        setItem(row, 0, new QStandardItem(QString::number(row + 1)));
        
        // Type (LONG/SHORT basé sur la taille)
        QString tradeType = trade->size() > 0 ? "LONG" : "SHORT";
        QStandardItem* typeItem = new QStandardItem(tradeType);
        typeItem->setForeground(trade->size() > 0 ? Qt::darkGreen : Qt::darkRed);
        setItem(row, 1, typeItem);
        
        // Taille (valeur absolue)
        setItem(row, 2, new QStandardItem(formatNumber(std::abs(trade->size()), 4)));

        // Prix d'entrée
        setItem(row, 3, new QStandardItem(formatNumber(trade->entryPrice(), 2)));

        // Prix de sortie
        setItem(row, 4, new QStandardItem(formatNumber(trade->exitPrice(), 2)));

        // PnL
        double pnl = trade->pl();
        QStandardItem* pnlItem = new QStandardItem(formatNumber(pnl, 2));
        pnlItem->setForeground(pnl >= 0 ? Qt::darkGreen : Qt::darkRed);
        QFont boldFont = pnlItem->font(); // Récupère la police actuelle
        boldFont.setBold(true);           // Active le gras
        pnlItem->setFont(boldFont);       // Applique la police modifiée
        setItem(row, 5, pnlItem);
        
        // PnL %
        double returnPct = trade->plPercent();
        QStandardItem* pctItem = new QStandardItem(formatNumber(returnPct * 100, 2) + "%");
        pctItem->setForeground(returnPct >= 0 ? Qt::darkGreen : Qt::darkRed);
        QFont pctFont = pctItem->font();
        pctFont.setBold(true);
        pctItem->setFont(pctFont);
        setItem(row, 6, pctItem);
        
        // Durée - calculer à partir des dates
        QDateTime entryDT = dateToQDateTime(trade->entryDate());
        QDateTime exitDT = dateToQDateTime(trade->exitDate());
        qint64 durationSecs = entryDT.secsTo(exitDT);
        
        QString durationStr;
        if (durationSecs < 60) {
            durationStr = QString("%1s").arg(durationSecs);
        } else if (durationSecs < 3600) {
            durationStr = QString("%1m %2s").arg(durationSecs / 60).arg(durationSecs % 60);
        } else if (durationSecs < 86400) {
            int hours = durationSecs / 3600;
            int mins = (durationSecs % 3600) / 60;
            durationStr = QString("%1h %2m").arg(hours).arg(mins);
        } else {
            int days = durationSecs / 86400;
            int hours = (durationSecs % 86400) / 3600;
            durationStr = QString("%1j %2h").arg(days).arg(hours);
        }
        setItem(row, 7, new QStandardItem(durationStr));
        
        // Date d'entrée
        setItem(row, 8, new QStandardItem(formatDateTime(entryDT)));
        
        // Date de sortie
        setItem(row, 9, new QStandardItem(formatDateTime(exitDT)));
        
        // Stop Loss (si disponible)
        QString slText = "-";
        if (trade->isBreakEven() && trade->initialSlPrice() > 0) {
            // Pour les trades en break-even, afficher le SL initial
            slText = formatNumber(trade->initialSlPrice(), 2);
        } else if (trade->sl() > 0) {
            // Pour les autres trades, afficher le SL actuel
            slText = formatNumber(trade->sl(), 2);
        }
        setItem(row, 10, new QStandardItem(slText));
        
        // Take Profit (si disponible)
        QString tpText = "-";
        if (trade->tp() > 0) {
            tpText = formatNumber(trade->tp(), 2);
        }
        setItem(row, 11, new QStandardItem(tpText));

        // Break-Even status
        QString beStatus = trade->isBreakEven() ? "Oui" : "-";
        QStandardItem* beItem = new QStandardItem(beStatus);
        if (trade->isBreakEven()) {
            beItem->setForeground(Qt::darkBlue);
            QFont beFont = beItem->font();
            beFont.setBold(true);
            beItem->setFont(beFont);
        }
        setItem(row, 12, beItem);
        
        // Tag (si disponible)
        QString tag = "-";
        if (!trade->tag().empty()) {
            tag = QString::fromStdString(trade->tag());
        }
        setItem(row, 13, new QStandardItem(tag));

        // Appliquer un style spécial si c'est un trade en break-even
        if (trade->isBreakEven() && std::abs(trade->pl()) < 1) {
            // Parcourir toutes les cellules de la ligne et appliquer une couleur de fond légère
            for (int col = 0; col < columnCount(); ++col) {
                QStandardItem* item = this->item(row, col);
                if (item) {
                    item->setData(QColor(220, 240, 255), Qt::BackgroundRole); // Bleu très clair
                }
            }
        }
        else if (pnl >= 0) {
            for (int col = 0; col < columnCount(); ++col) {
                QStandardItem* item = this->item(row, col);
                if (item) {
                    item->setData(QColor(220, 255, 220), Qt::BackgroundRole); // Vert très clair
                }
            }
        }
        else {
            for (int col = 0; col < columnCount(); ++col) {
                QStandardItem* item = this->item(row, col);
                if (item) {
                    item->setData(QColor(255, 220, 220), Qt::BackgroundRole); // Rouge très clair
                }
            }
        }
    }

    endResetModel();
}

// Méthodes utilitaires
QString TradesTableModel::formatNumber(double value, int precision)
{
    return QString::number(value, 'f', precision);
}

QString TradesTableModel::formatDateTime(const QDateTime& dateTime)
{
    if (!dateTime.isValid()) {
        return "-";
    }
    return dateTime.toString("dd/MM hh:mm:ss");
}

// Implémentation de StatsView
StatsView::StatsView(QWidget* parent)
    : BaseView(parent),
      m_tradesModel(new TradesTableModel(this)),
      m_tablesCreated(false),
      m_currentResults(nullptr),
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
        {"time", m_timeLayout},
        {"performance", m_performanceLayout},
        {"risk", m_riskLayout},
        {"general", m_generalLayout}
    };
    
    // Créer tous les widgets de métriques à partir des définitions
    for (const auto& metric : m_metricDefinitions) {
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

    m_statsGridLayout->addWidget(legendWidget, 3, 0, 1, 2); // Span sur 2 colonnes
}

void StatsView::arrangePanels() {
    // Ajouter m_timeGroup seul sur la première ligne (span sur 3 colonnes)
    m_statsGridLayout->addWidget(m_timeGroup, 0, 0, 1, 3);          // Première ligne, span sur 3 colonnes
    
    // Ajouter les 3 autres groupes sur la deuxième ligne
    m_statsGridLayout->addWidget(m_performanceGroup, 1, 0);         // Deuxième ligne, première colonne
    m_statsGridLayout->addWidget(m_riskGroup, 1, 1);               // Deuxième ligne, deuxième colonne
    m_statsGridLayout->addWidget(m_generalGroup, 1, 2);            // Deuxième ligne, troisième colonne
    
    // Ajouter le placeholder en dessous de la grille (span sur 3 colonnes)
    QWidget* placeholderWidget = new QWidget();
    placeholderWidget->setLayout(m_statsContentLayout);
    m_statsGridLayout->addWidget(placeholderWidget, 2, 0, 1, 3); // Span sur 3 colonnes
}

void StatsView::updateData(BacktestResults* results)
{
    // Récupérer les résultats depuis l'App
    BacktestResults* appResults = m_app ? m_app->getBacktestResults() : nullptr;
    
    // Stocker les résultats pour les mises à jour ultérieures
    m_currentResults = appResults;
    
    if (!appResults) {
        qWarning() << "Résultats nuls reçus";
        clear();
        return;
    }

    // std::cout << m_currentResults->stats << std::endl;

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
        populateMetrics(appResults->stats);
        
        // Mettre à jour la table des trades
        populateTrades(appResults->stats.trades);
        
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
        {"win_rate", "Taux de réussite:", "Pourcentage de trades rentables", "general",
            [](const be::Stats& s) { 
                return s.winRatePct > 50 ? MetricStatus::Good : 
                      (s.winRatePct < 50 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.winRatePct, 'f', 2)); }
        },
        {"winning_trades", "Trades gagnants:", "Nombre de trades gagnants", "general",
            [](const be::Stats& s) { return MetricStatus::Good; },
            [](const be::Stats& s) { return QString::number(s.numWinningTrades); }
        },
        {"losing_trades", "Trades perdants:", "Nombre de trades perdants", "general",
            [](const be::Stats& s) { 
                return s.numLosingTrades <= s.numWinningTrades ? MetricStatus::Neutral : MetricStatus::Bad; 
            },
            [](const be::Stats& s) { return QString::number(s.numLosingTrades); }
        },
        {"neutral_trades", "Trades neutres:", "Nombre de trades neutres", "general",
            [](const be::Stats& s) { return s.numNeutralTrades <= s.numWinningTrades ? MetricStatus::Neutral : MetricStatus::Bad; },
            [](const be::Stats& s) { return QString::number(s.numNeutralTrades); }
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
    m_tradesTable->setColumnWidth(12, 50);  // Break-Even (nouvelle colonne)
    m_tradesTable->setColumnWidth(13, 80);  // Tag (déplacé)

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

std::vector<std::shared_ptr<be::Trade>> StatsView::getFilteredTrades(const std::vector<std::shared_ptr<be::Trade>>& allTrades) {
    std::vector<std::shared_ptr<be::Trade>> filteredTrades = allTrades;
    
    if (m_tradesLimitCombo) {
        int limit = m_tradesLimitCombo->currentData().toInt();
        if (limit > 0 && filteredTrades.size() > static_cast<size_t>(limit)) {
            filteredTrades = std::vector<std::shared_ptr<be::Trade>>(
                filteredTrades.end() - limit, filteredTrades.end()
            );
        }
    }
    
    return filteredTrades;
}

void StatsView::populateTrades(const std::vector<std::shared_ptr<be::Trade>>& trades)
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