#include "views/stats_view.h"
#include "app.h"
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
            << "SL" << "TP" << "Tag";
    setHorizontalHeaderLabels(headers);
    
    // Ajouter les nouvelles données
    setRowCount(trades.size());
    
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
        setItem(row, 5, pnlItem);
        
        // PnL %
        double returnPct = trade->plPercent();
        QStandardItem* pctItem = new QStandardItem(formatNumber(returnPct * 100, 2) + "%");
        pctItem->setForeground(returnPct >= 0 ? Qt::darkGreen : Qt::darkRed);
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
        if (trade->sl() > 0) {
            slText = formatNumber(trade->sl(), 2);
        }
        setItem(row, 10, new QStandardItem(slText));
        
        // Take Profit (si disponible)
        QString tpText = "-";
        if (trade->tp() > 0) {
            tpText = formatNumber(trade->tp(), 2);
        }
        setItem(row, 11, new QStandardItem(tpText));
        
        // Tag (si disponible)
        QString tag = "-";
        if (!trade->tag().empty()) {
            tag = QString::fromStdString(trade->tag());
        }
        setItem(row, 12, new QStandardItem(tag));
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

QString TradesTableModel::formatDuration(const QString& duration)
{
    if (duration.contains("days") && duration.contains(":")) {
        // Parse "0 days 00:00:40" format
        QStringList parts = duration.split(" ");
        if (parts.size() >= 3) {
            int days = parts[0].toInt();
            QStringList timeParts = parts[2].split(":");
            if (timeParts.size() >= 3) {
                int hours = timeParts[0].toInt();
                int minutes = timeParts[1].toInt();
                int seconds = timeParts[2].toInt();
                
                if (days > 0) {
                    return QString("%1j %2h%3m").arg(days).arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0'));
                } else if (hours > 0) {
                    return QString("%1h%2m%3s").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
                } else if (minutes > 0) {
                    return QString("%1m%2s").arg(minutes).arg(seconds, 2, 10, QChar('0'));
                } else {
                    return QString("%1s").arg(seconds);
                }
            }
        }
    }
    return duration;
}

// Implémentation de StatsView
StatsView::StatsView(QWidget* parent)
    : BaseView(parent)
    , m_tradesModel(nullptr)
    , m_equityModel(nullptr)
    , m_scrollStats(nullptr)
    , m_statsContent(nullptr)
    , m_statsContentLayout(nullptr)
    , m_statsPlaceholder(nullptr)
    , m_performanceGroup(nullptr)
    , m_performanceLayout(nullptr)
    , m_riskGroup(nullptr)
    , m_riskLayout(nullptr)
    , m_generalGroup(nullptr)
    , m_generalLayout(nullptr)
    , m_tradesGroup(nullptr)
    , m_tradesLayout(nullptr)
    , m_tradesTable(nullptr)
    , m_tradesLimitCombo(nullptr)
    , m_showAllTradesBtn(nullptr)
    , m_equityGroup(nullptr)
    , m_equityLayout(nullptr)
    , m_showEquityBtn(nullptr)
    , m_equityLimitCombo(nullptr)
    , m_equityStack(nullptr)
    , m_tablesCreated(false)
    , m_currentResults(nullptr)
    , m_app(nullptr)  // Ajouter cette ligne
{
    // Ajouter ce bloc après les initialisations
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
        
    // Créer les modèles de données
    m_tradesModel = new TradesTableModel(this);
    
    initializeMetricDefinitions();
    
    // Construire l'interface dans le constructeur
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
    // Regrouper les métriques par section
    QMap<QString, QVBoxLayout*> sectionLayouts = {
        {"time", m_timeLayout},
        {"performance", m_performanceLayout},
        {"risk", m_riskLayout},
        {"general", m_generalLayout}
    };
    
    // Créer tous les widgets de métriques à partir de la définition centralisée
    for (const auto& metric : m_metricDefinitions) {
        QVBoxLayout* targetLayout = sectionLayouts[metric.section];
        if (!targetLayout) continue;
        
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
    // Ajouter les groupes dans une grille 2x2
    m_statsGridLayout->addWidget(m_timeGroup, 0, 0);      // Première ligne, première colonne
    m_statsGridLayout->addWidget(m_performanceGroup, 0, 1); // Première ligne, deuxième colonne
    m_statsGridLayout->addWidget(m_riskGroup, 1, 0);      // Deuxième ligne, première colonne
    m_statsGridLayout->addWidget(m_generalGroup, 1, 1);   // Deuxième ligne, deuxième colonne
    
    // Ajouter le placeholder en dessous de la grille
    QWidget* placeholderWidget = new QWidget();
    placeholderWidget->setLayout(m_statsContentLayout);
    m_statsGridLayout->addWidget(placeholderWidget, 2, 0, 1, 2); // Span sur 2 colonnes
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

    std::cout << m_currentResults->stats << std::endl;

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
        
        // Mettre à jour l'équité
        populateEquity(appResults->stats);
        
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
                return s.equityFinal >= s.equityPeak * 0.9 ? MetricStatus::Good :
                       (s.equityFinal < s.equityPeak * 0.7 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("$%1").arg(QString::number(s.equityFinal, 'f', 2)); }
        },
        {"equity_peak", "Capital maximal:", "Montant maximal atteint par le capital", "performance",
            [](const be::Stats& s) { return MetricStatus::Good; },
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
            [](const be::Stats& s) { 
                return s.maxDrawdownPct < 5 ? MetricStatus::Good : 
                      (s.maxDrawdownPct > 10 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
            [](const be::Stats& s) { return QString("%1%").arg(QString::number(s.maxDrawdownPct, 'f', 2)); }
        },
        {"avg_drawdown", "Drawdown moyen:", "Perte moyenne depuis un sommet précédent", "risk",
            [](const be::Stats& s) { 
                return s.avgDrawdownPct < 2 ? MetricStatus::Good : 
                      (s.avgDrawdownPct > 5 ? MetricStatus::Bad : MetricStatus::Neutral); 
            },
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
        {"win_rate", "Taux de réussite:", "Pourcentage de trades rentables", "general",
            [](const be::Stats& s) { 
                return s.winRatePct > 50 ? MetricStatus::Good : 
                      (s.winRatePct < 40 ? MetricStatus::Bad : MetricStatus::Neutral); 
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
        {"profit_factor", "Facteur de profit:", "Ratio des gains sur les pertes (>1 est profitable)", "general",
            [](const be::Stats& s) {
                if (std::isnan(s.profitFactor)) return MetricStatus::NA;
                return s.profitFactor > 1.5 ? MetricStatus::Good : 
                      (s.profitFactor < 1 ? MetricStatus::Bad : MetricStatus::Neutral);
            },
            [](const be::Stats& s) {
                return std::isnan(s.profitFactor) ? QString("N/A") : QString::number(s.profitFactor, 'f', 2);
            }
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

// Nouvelle organisation des sections
void StatsView::createTimeSection(QVBoxLayout* layout)
{    
    // Helper function to reduce repetition
    auto addMetricRow = [this, layout](const QString& key, const QString& label, const QString& tooltip) {
        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 5, 5, 5);
        createMetricWidget(key, label, "N/A", rowLayout)->setTooltip(tooltip);
        layout->addWidget(row);
    };

    // Période
    addMetricRow("start", "Début:", "Date de début du backtest");
    addMetricRow("end", "Fin:", "Date de fin du backtest");
    addMetricRow("duration", "Durée:", "Durée totale du backtest");
    addMetricRow("exposure_time", "Temps en position:", "Pourcentage du temps avec des positions ouvertes");
}

void StatsView::createPerformanceSection(QVBoxLayout* layout)
{
    // Helper function to reduce repetition
    auto addMetricRow = [this, layout](const QString& key, const QString& label, const QString& tooltip) {
        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 5, 5, 5);
        createMetricWidget(key, label, "N/A", rowLayout)->setTooltip(tooltip);
        layout->addWidget(row);
    };
    
    // Capital metrics
    addMetricRow("equity_final", "Capital final:", "Montant final du capital");
    addMetricRow("equity_peak", "Capital maximal:", "Montant maximal atteint par le capital");
    
    // Return metrics
    addMetricRow("total_return", "Rendement total:", "Pourcentage de gain/perte sur l'ensemble du backtest");
    addMetricRow("buy_hold_return", "Buy & Hold:", "Rendement d'une stratégie passive d'achat et maintien");
    addMetricRow("return_ann", "Rendement annualisé:", "Rendement annualisé du backtest");
    addMetricRow("cagr", "CAGR:", "Taux de croissance annuel composé");
    
    // Alpha/Beta metrics
    addMetricRow("alpha", "Alpha:", "Surperformance par rapport au marché (ajustée au risque)");
    addMetricRow("beta", "Beta:", "Corrélation avec les mouvements du marché");
}

void StatsView::createRiskSection(QVBoxLayout* layout)
{
    // Helper function to reduce repetition
    auto addMetricRow = [this, layout](const QString& key, const QString& label, const QString& tooltip) {
        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 5, 5, 5);
        createMetricWidget(key, label, "N/A", rowLayout)->setTooltip(tooltip);
        layout->addWidget(row);
    };

    addMetricRow("max_drawdown", "Drawdown maximal:", "Perte maximale depuis un sommet précédent");
    addMetricRow("avg_drawdown", "Drawdown moyen:", "Perte moyenne depuis un sommet précédent");

    addMetricRow("max_drawdown_duration", "Durée DD max:", "Durée de la plus longue période de drawdown");
    addMetricRow("avg_drawdown_duration", "Durée DD moyenne:", "Durée moyenne des périodes de drawdown");

    // Ratios de risque
    addMetricRow("sharpe_ratio", "Ratio de Sharpe:", "Rendement excédentaire par unité de risque total");
    addMetricRow("sortino_ratio", "Ratio de Sortino:", "Rendement excédentaire par unité de risque négatif");

    addMetricRow("calmar_ratio", "Ratio de Calmar:", "Rendement annualisé divisé par le drawdown maximal");
    addMetricRow("volatility", "Volatilité annualisée:", "Mesure de la variabilité des rendements");
}

void StatsView::createGeneralSection(QVBoxLayout* layout)
{
    // Helper function to reduce repetition
    auto addMetricRow = [this, layout](const QString& key, const QString& label, const QString& tooltip) {
        QWidget* row = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(5, 5, 5, 5);
        createMetricWidget(key, label, "N/A", rowLayout)->setTooltip(tooltip);
        layout->addWidget(row);
    };

    // Créer les widgets de métriques pour la section générale
    addMetricRow("total_trades", "Nombre de trades:", "Nombre total de transactions effectuées");
    addMetricRow("win_rate", "Taux de réussite:", "Pourcentage de trades rentables");
    addMetricRow("winning_trades", "Trades gagnants:", "Nombre de trades gagnants");
    addMetricRow("losing_trades", "Trades perdants:", "Nombre de trades perdants");
    addMetricRow("best_trade", "Meilleur trade:", "Pourcentage de gain du meilleur trade");
    addMetricRow("worst_trade", "Pire trade:", "Pourcentage de perte du pire trade");
    addMetricRow("avg_trade", "Trade moyen:", "Rendement moyen par trade");

    addMetricRow("profit_factor", "Facteur de profit:", "Ratio des gains sur les pertes (>1 est profitable)");
    addMetricRow("max_trade_duration", "Durée max trade:", "Durée maximale d'un trade");

    addMetricRow("avg_trade_duration", "Durée moy trade:", "Durée moyenne d'un trade");
    addMetricRow("expectancy", "Espérance:", "Gain moyen attendu par trade");
    addMetricRow("sqn", "SQN:", "System Quality Number - mesure de la qualité du système de trading");
    addMetricRow("kelly_criterion", "Critère de Kelly:", "Taille de position optimale selon le critère de Kelly");
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
    m_tradesTable->setColumnWidth(0, 50);  // #
    m_tradesTable->setColumnWidth(1, 80);  // Type
    m_tradesTable->setColumnWidth(2, 100); // Taille
    m_tradesTable->setColumnWidth(3, 120); // Prix d'entrée
    m_tradesTable->setColumnWidth(4, 120); // Prix de sortie
    m_tradesTable->setColumnWidth(5, 100); // PnL
    m_tradesTable->setColumnWidth(6, 80);  // PnL %
    m_tradesTable->setColumnWidth(7, 100); // Durée

    // Hauteur de la table
    m_tradesTable->setMaximumHeight(1000);  // Très grande table
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


QString StatsView::formatCurrency(double value)
{
    if (std::isnan(value) || std::isinf(value)) {
        return "N/A";
    }
    
    QString formatted = QString::number(value, 'f', 2);
    
    // Ajouter le symbole de devise
    if (value >= 0) {
        return QString("$%1").arg(formatted);
    } else {
        return QString("-$%1").arg(QString::number(std::abs(value), 'f', 2));
    }
}

QString StatsView::formatPercentage(double value)
{
    if (std::isnan(value) || std::isinf(value)) {
        return "N/A";
    }
    
    // Convertir en pourcentage (multiplier par 100) et formater
    return QString("%1%").arg(QString::number(value * 100, 'f', 2));
}

QString StatsView::formatDetailedDuration(int days, int hours, int minutes, int seconds)
{
    QStringList parts;
    
    if (days > 0) {
        parts.append(QString("%1J").arg(days));
    }
    
    if (hours > 0) {
        parts.append(QString("%1h").arg(hours, 2, 10, QChar('0')));
    }
    
    if (minutes > 0) {
        parts.append(QString("%1min").arg(minutes, 2, 10, QChar('0')));
    }
    
    if (seconds > 0 || parts.isEmpty()) {
        parts.append(QString("%1sec").arg(seconds, 2, 10, QChar('0')));
    }
    
    return parts.join(" ");
}

void StatsView::updateMetricWidget(const QString& key, const QString& label, const QString& value)
{
    Q_UNUSED(label);
    if (m_metricWidgets.contains(key)) {
        m_metricWidgets[key]->updateValues(value);
    } else {
        qWarning() << "Widget métrique non trouvé pour la clé:" << key;
    }
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

// Nouvelle méthode populateEquity adaptée pour be::Stats
void StatsView::populateEquity(const be::Stats& stats)
{
    qDebug() << "Population de la table d'équité";
    
    // Pour l'instant, on ne fait rien car le modèle d'équité n'est pas implémenté
    // Mais ce serait ici qu'on traiterait stats.equityCurve pour l'afficher
    
    if (m_equityGroup) {
        m_equityGroup->setVisible(!stats.equityCurve.empty());
    }
    
    qDebug() << "Équité disponible:" << stats.equityCurve.size() << "points";
}

// Nouvelle méthode refreshTradesTable adaptée pour les nouvelles structures
void StatsView::refreshTradesTable()
{
    qDebug() << "StatsView::refreshTradesTable() appelé";
    
    // S'assurer d'avoir les derniers résultats
    if (m_app) {
        m_currentResults = m_app->getBacktestResults();
    }
    
    if (!m_tradesModel || !m_currentResults) {
        return;
    }
    
    // Récupérer les trades depuis les résultats C++
    const auto& allTrades = m_currentResults->stats.trades;
    
    // Appliquer la limitation si nécessaire
    std::vector<std::shared_ptr<be::Trade>> trades = allTrades;
    if (m_tradesLimitCombo) {
        int limit = m_tradesLimitCombo->currentData().toInt();
        if (limit > 0 && trades.size() > static_cast<size_t>(limit)) {
            // Prendre les derniers trades (les plus récents)
            trades = std::vector<std::shared_ptr<be::Trade>>(
                trades.end() - limit, trades.end()
            );
        }
    }
    
    // Mettre à jour le modèle
    m_tradesModel->updateData(trades);
    
    // Mettre à jour le texte du bouton avec le nombre total
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText(QString("Afficher tous les trades (%1)").arg(allTrades.size()));
    }
    
    qDebug() << "Table des trades mise à jour avec" << trades.size() << "/" << allTrades.size() << "trades";
}

void StatsView::clear()
{
    qDebug() << "StatsView::clear() appelé";
    
    // Réinitialiser tous les widgets de métriques
    for (auto it = m_metricWidgets.begin(); it != m_metricWidgets.end(); ++it) {
        it.value()->updateValues("N/A");
    }
    
    // Vider les modèles de tables
    if (m_tradesModel) {
        m_tradesModel->clear();
        m_tradesModel->setHorizontalHeaderLabels({
            "Date d'entrée", "Date de sortie", "Type", "Taille", 
            "Prix d'entrée", "Prix de sortie", "P&L", "P&L %", "Durée"
        });
    }
    
    if (m_equityModel) {
        m_equityModel->clear();
        m_equityModel->setHorizontalHeaderLabels({
            "Date", "Equity", "Drawdown", "Drawdown %"
        });
    }
    
    // Réafficher le placeholder
    if (m_statsPlaceholder) {
        m_statsPlaceholder->setText("Exécutez un backtest pour voir les statistiques");
        m_statsPlaceholder->setVisible(true);
    }
    
    // Masquer les sections
    if (m_performanceGroup) m_performanceGroup->setVisible(false);
    if (m_riskGroup) m_riskGroup->setVisible(false);
    if (m_generalGroup) m_generalGroup->setVisible(false);
    if (m_tradesGroup) m_tradesGroup->setVisible(false);
    if (m_equityGroup) m_equityGroup->setVisible(false);
    
    // Réinitialiser l'état
    m_currentResults = nullptr;
    
    qDebug() << "StatsView nettoyée";
}

// Méthodes restantes comme formatCurrency, formatPercentage, etc. restent inchangées