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
    
    qDebug() << "StatsView créée avec parent:" << parent << "et app:" << m_app;
    
    // Créer les modèles de données
    m_tradesModel = new TradesTableModel(this);
    
    // Construire l'interface dans le constructeur
    setupUI();
}

StatsView::~StatsView()
{
    // Les modèles et widgets sont automatiquement détruits par Qt
}

void StatsView::updateData(BacktestResults* results)
{
    QTime start = QTime::currentTime();
    
    // Ignorer le pointeur passé et utiliser celui de l'App
    Q_UNUSED(results);
    
    qDebug() << "StatsView::updateData() appelé";
    
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

void StatsView::setupUI()
{
    // Créer le scroll area principal
    m_scrollStats = new QScrollArea();
    m_scrollStats->setWidgetResizable(true);
    
    // Créer le widget de contenu avec un style pour les groupes
    m_statsContent = new QWidget();
    m_statsContentLayout = new QVBoxLayout(m_statsContent);
    m_statsContentLayout->setSpacing(15); // Plus d'espace entre les groupes
    
    // Placeholder initial
    m_statsPlaceholder = new QLabel("Exécutez le backtest pour afficher les statistiques");
    m_statsPlaceholder->setAlignment(Qt::AlignCenter);
    m_statsContentLayout->addWidget(m_statsPlaceholder);
    
    // Créer les sections de métriques avec des titres plus descriptifs
    m_performanceGroup = new QGroupBox("Résultats et Performance");
    m_performanceLayout = new QGridLayout();
    m_performanceLayout->setVerticalSpacing(10);
    m_performanceLayout->setHorizontalSpacing(20);
    m_performanceGroup->setLayout(m_performanceLayout);
    
    m_riskGroup = new QGroupBox("Mesures de Risque et Volatilité");
    m_riskLayout = new QGridLayout();
    m_riskLayout->setVerticalSpacing(10);
    m_riskLayout->setHorizontalSpacing(20);
    m_riskGroup->setLayout(m_riskLayout);
    
    m_generalGroup = new QGroupBox("Statistiques de Trading");
    m_generalLayout = new QGridLayout();
    m_generalLayout->setVerticalSpacing(10);
    m_generalLayout->setHorizontalSpacing(20);
    m_generalGroup->setLayout(m_generalLayout);
    
    // Créer un nouveau groupe pour les métriques temporelles
    m_timeGroup = new QGroupBox("Période et Exposition");
    m_timeLayout = new QGridLayout();
    m_timeLayout->setVerticalSpacing(10);
    m_timeLayout->setHorizontalSpacing(20);
    m_timeGroup->setLayout(m_timeLayout);
    
    // Créer les widgets de métriques
    createStatsWidgets();
    
    // Ajouter les groupes dans un ordre logique
    m_statsContentLayout->addWidget(m_timeGroup);
    m_statsContentLayout->addWidget(m_performanceGroup);
    m_statsContentLayout->addWidget(m_riskGroup);
    m_statsContentLayout->addWidget(m_generalGroup);
    
    // Configurer le scroll area
    m_scrollStats->setWidget(m_statsContent);
    
    // Ajouter le scroll area au layout principal
    m_mainLayout->addWidget(m_scrollStats);


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
    
    // Ajouter la légende au layout principal
    m_statsContentLayout->addWidget(legendWidget);
}

// Nouvelle organisation des sections
void StatsView::createTimeSection(QGridLayout* layout)
{
    int row = 0;
    
    // Période
    createMetricWidget("start", "Début", "N/A", row, 0, layout)
        ->setTooltip("Date de début du backtest");
    createMetricWidget("end", "Fin", "N/A", row++, 1, layout)
        ->setTooltip("Date de fin du backtest");
    createMetricWidget("duration", "Durée", "N/A", row, 0, layout)
        ->setTooltip("Durée totale du backtest");
    createMetricWidget("exposure_time", "Temps en position", "N/A", row++, 1, layout)
        ->setTooltip("Pourcentage du temps avec des positions ouvertes");
}

void StatsView::createPerformanceSection(QGridLayout* layout)
{
    int row = 0;
    
    // Résultats principaux
    createMetricWidget("equity_final", "Capital final", "N/A", row, 0, layout)
        ->setTooltip("Montant final du capital");
    createMetricWidget("equity_peak", "Capital maximal", "N/A", row++, 1, layout)
        ->setTooltip("Montant maximal atteint par le capital");
    
    // Rendements
    createMetricWidget("total_return", "Rendement total", "N/A", row, 0, layout)
        ->setTooltip("Pourcentage de gain/perte sur l'ensemble du backtest");
    createMetricWidget("buy_hold_return", "Buy & Hold", "N/A", row++, 1, layout)
        ->setTooltip("Rendement d'une stratégie passive d'achat et maintien");
    
    createMetricWidget("return_ann", "Rendement annualisé", "N/A", row, 0, layout)
        ->setTooltip("Rendement annuel équivalent");
    createMetricWidget("cagr", "CAGR", "N/A", row++, 1, layout)
        ->setTooltip("Taux de croissance annuel composé");
        
    // Alpha/Beta
    createMetricWidget("alpha", "Alpha", "N/A", row, 0, layout)
        ->setTooltip("Surperformance par rapport au marché (ajustée au risque)");
    createMetricWidget("beta", "Beta", "N/A", row++, 1, layout)
        ->setTooltip("Corrélation avec les mouvements du marché");
}

void StatsView::createRiskSection(QGridLayout* layout)
{
    int row = 0;
    
    // Drawdowns
    createMetricWidget("max_drawdown", "Drawdown maximal", "N/A", row, 0, layout)
        ->setTooltip("Perte maximale depuis un sommet précédent");
    createMetricWidget("avg_drawdown", "Drawdown moyen", "N/A", row++, 1, layout)
        ->setTooltip("Perte moyenne depuis un sommet précédent");
    
    createMetricWidget("max_drawdown_duration", "Durée DD max", "N/A", row, 0, layout)
        ->setTooltip("Durée de la plus longue période de drawdown");
    createMetricWidget("avg_drawdown_duration", "Durée DD moyenne", "N/A", row++, 1, layout)
        ->setTooltip("Durée moyenne des périodes de drawdown");
    
    // Ratios de risque
    createMetricWidget("sharpe_ratio", "Ratio de Sharpe", "N/A", row, 0, layout)
        ->setTooltip("Rendement excédentaire par unité de risque total");
    createMetricWidget("sortino_ratio", "Ratio de Sortino", "N/A", row++, 1, layout)
        ->setTooltip("Rendement excédentaire par unité de risque négatif");
    
    createMetricWidget("calmar_ratio", "Ratio de Calmar", "N/A", row, 0, layout)
        ->setTooltip("Rendement annualisé divisé par le drawdown maximal");
    createMetricWidget("volatility", "Volatilité annualisée", "N/A", row++, 1, layout)
        ->setTooltip("Mesure de la variabilité des rendements");
}

void StatsView::createGeneralSection(QGridLayout* layout)
{
    int row = 0;
    
    // Stats des trades
    createMetricWidget("total_trades", "Nombre de trades", "N/A", row, 0, layout)
        ->setTooltip("Nombre total de transactions effectuées");
    createMetricWidget("win_rate", "Taux de réussite", "N/A", row++, 1, layout)
        ->setTooltip("Pourcentage de trades rentables");
    
    createMetricWidget("winning_trades", "Trades gagnants", "N/A", row, 0, layout);
    createMetricWidget("losing_trades", "Trades perdants", "N/A", row++, 1, layout);
    
    // Performance des trades
    createMetricWidget("best_trade", "Meilleur trade", "N/A", row, 0, layout)
        ->setTooltip("Pourcentage de gain du meilleur trade");
    createMetricWidget("worst_trade", "Pire trade", "N/A", row++, 1, layout)
        ->setTooltip("Pourcentage de perte du pire trade");
    
    createMetricWidget("avg_trade", "Trade moyen", "N/A", row, 0, layout)
        ->setTooltip("Rendement moyen par trade");
    createMetricWidget("profit_factor", "Facteur de profit", "N/A", row++, 1, layout)
        ->setTooltip("Ratio des gains sur les pertes (>1 est profitable)");
    
    // Durée des trades
    createMetricWidget("max_trade_duration", "Durée max trade", "N/A", row, 0, layout);
    createMetricWidget("avg_trade_duration", "Durée moy trade", "N/A", row++, 1, layout);
    
    // Métriques avancées
    createMetricWidget("expectancy", "Espérance", "N/A", row, 0, layout)
        ->setTooltip("Gain moyen attendu par unité de risque");
    createMetricWidget("sqn", "SQN", "N/A", row++, 1, layout)
        ->setTooltip("System Quality Number - qualité du système de trading");
    
    createMetricWidget("kelly_criterion", "Critère de Kelly", "N/A", row++, 0, layout)
        ->setTooltip("Taille de position optimale selon le critère de Kelly");
}

// Mise à jour de la méthode populateMetrics pour utiliser les méthodes toString() 
// et ajouter la coloration conditionnelle
void StatsView::populateMetrics(const be::Stats& stats)
{
    qDebug() << "Population des métriques avec coloration conditionnelle";
    
    // Utiliser les méthodes toString() pour les durées
    if (m_metricWidgets.contains("duration")) {
        m_metricWidgets["duration"]->updateValues(
            QString::fromStdString(stats.duration.toString()),
            MetricStatus::Neutral
        );
    }
    
    if (m_metricWidgets.contains("max_drawdown_duration")) {
        m_metricWidgets["max_drawdown_duration"]->updateValues(
            QString::fromStdString(stats.maxDrawdownDuration.toString()),
            MetricStatus::Neutral
        );
    }
    
    if (m_metricWidgets.contains("avg_drawdown_duration")) {
        m_metricWidgets["avg_drawdown_duration"]->updateValues(
            QString::fromStdString(stats.avgDrawdownDuration.toString()),
            MetricStatus::Neutral
        );
    }
    
    if (m_metricWidgets.contains("max_trade_duration")) {
        m_metricWidgets["max_trade_duration"]->updateValues(
            QString::fromStdString(stats.maxTradeDuration.toString()),
            MetricStatus::Neutral
        );
    }
    
    if (m_metricWidgets.contains("avg_trade_duration")) {
        m_metricWidgets["avg_trade_duration"]->updateValues(
            QString::fromStdString(stats.avgTradeDuration.toString()),
            MetricStatus::Neutral
        );
    }
    
    // Dates de début et fin avec coloration neutre
    if (m_metricWidgets.contains("start")) {
        m_metricWidgets["start"]->updateValues(
            QString::fromStdString(stats.start.toString()),
            MetricStatus::Neutral
        );
    }
    
    if (m_metricWidgets.contains("end")) {
        m_metricWidgets["end"]->updateValues(
            QString::fromStdString(stats.end.toString()),
            MetricStatus::Neutral
        );
    }
    
    // Rendements avec coloration conditionnelle
    if (m_metricWidgets.contains("total_return")) {
        QString value = QString("%1%").arg(QString::number(stats.returnPct, 'f', 2));
        MetricStatus status = stats.returnPct > 0 ? MetricStatus::Good : 
                             (stats.returnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["total_return"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("return_ann")) {
        QString value = QString("%1%").arg(QString::number(stats.returnAnnPct, 'f', 2));
        MetricStatus status = stats.returnAnnPct > 0 ? MetricStatus::Good : 
                             (stats.returnAnnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["return_ann"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("buy_hold_return")) {
        QString value = QString("%1%").arg(QString::number(stats.buyHoldReturnPct, 'f', 2));
        MetricStatus status = stats.buyHoldReturnPct > 0 ? MetricStatus::Good : 
                             (stats.buyHoldReturnPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["buy_hold_return"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("cagr")) {
        QString value = QString("%1%").arg(QString::number(stats.cagrPct, 'f', 2));
        MetricStatus status = stats.cagrPct > 0 ? MetricStatus::Good : 
                             (stats.cagrPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["cagr"]->updateValues(value, status);
    }
    
    // Métriques de risque avec coloration conditionnelle
    if (m_metricWidgets.contains("sharpe_ratio")) {
        QString value = isValidNumber(stats.sharpeRatio) ? 
                      QString::number(stats.sharpeRatio, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.sharpeRatio) ? 
                            (stats.sharpeRatio > 1 ? MetricStatus::Good : 
                             (stats.sharpeRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["sharpe_ratio"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("sortino_ratio")) {
        QString value = isValidNumber(stats.sortinoRatio) ? 
                      QString::number(stats.sortinoRatio, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.sortinoRatio) ? 
                            (stats.sortinoRatio > 1 ? MetricStatus::Good : 
                             (stats.sortinoRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["sortino_ratio"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("calmar_ratio")) {
        QString value = isValidNumber(stats.calmarRatio) ? 
                      QString::number(stats.calmarRatio, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.calmarRatio) ? 
                            (stats.calmarRatio > 1 ? MetricStatus::Good : 
                             (stats.calmarRatio < 0 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["calmar_ratio"]->updateValues(value, status);
    }
    
    // Drawdowns toujours en rouge (plus c'est bas, mieux c'est)
    if (m_metricWidgets.contains("max_drawdown")) {
        QString value = QString("%1%").arg(QString::number(stats.maxDrawdownPct, 'f', 2));
        MetricStatus status = stats.maxDrawdownPct > 10 ? MetricStatus::Bad : 
                             (stats.maxDrawdownPct > 5 ? MetricStatus::Neutral : MetricStatus::Good);
        m_metricWidgets["max_drawdown"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("avg_drawdown")) {
        QString value = QString("%1%").arg(QString::number(stats.avgDrawdownPct, 'f', 2));
        MetricStatus status = stats.avgDrawdownPct > 5 ? MetricStatus::Bad : 
                             (stats.avgDrawdownPct > 2 ? MetricStatus::Neutral : MetricStatus::Good);
        m_metricWidgets["avg_drawdown"]->updateValues(value, status);
    }
    
    // Alpha/Beta
    if (m_metricWidgets.contains("alpha")) {
        QString value = QString("%1%").arg(QString::number(stats.alphaPct, 'f', 2));
        MetricStatus status = stats.alphaPct > 0 ? MetricStatus::Good : 
                             (stats.alphaPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["alpha"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("beta")) {
        QString value = isValidNumber(stats.beta) ? 
                      QString::number(stats.beta, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.beta) ? 
                            (stats.beta < 0.8 ? MetricStatus::Good : 
                             (stats.beta > 1.2 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["beta"]->updateValues(value, status);
    }
    
    // Volatilité
    if (m_metricWidgets.contains("volatility")) {
        QString value = QString("%1%").arg(QString::number(stats.volatilityAnnPct, 'f', 2));
        MetricStatus status = stats.volatilityAnnPct < 10 ? MetricStatus::Good : 
                             (stats.volatilityAnnPct > 25 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["volatility"]->updateValues(value, status);
    }
    
    // Statistiques de trading
    if (m_metricWidgets.contains("win_rate")) {
        QString value = QString("%1%").arg(QString::number(stats.winRatePct, 'f', 1));
        MetricStatus status = stats.winRatePct > 50 ? MetricStatus::Good : 
                             (stats.winRatePct < 40 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["win_rate"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("exposure_time")) {
        QString value = QString("%1%").arg(QString::number(stats.exposureTimePct, 'f', 1));
        m_metricWidgets["exposure_time"]->updateValues(value, MetricStatus::Neutral);
    }
    
    // Capital
    if (m_metricWidgets.contains("equity_final")) {
        QString value = QString("$%1").arg(QString::number(stats.equityFinal, 'f', 2));
        MetricStatus status = stats.equityFinal > stats.equityPeak * 0.9 ? MetricStatus::Good : 
                             (stats.equityFinal < stats.equityPeak * 0.7 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["equity_final"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("equity_peak")) {
        QString value = QString("$%1").arg(QString::number(stats.equityPeak, 'f', 2));
        m_metricWidgets["equity_peak"]->updateValues(value, MetricStatus::Good);
    }
    
    // Autres métriques...
    // Continuer à ajouter les métriques restantes avec leur coloration conditionnelle appropriée
    
    // Nombre de trades
    if (m_metricWidgets.contains("total_trades")) {
        QString value = QString::number(stats.numTrades);
        m_metricWidgets["total_trades"]->updateValues(value, MetricStatus::Neutral);
    }
    
    if (m_metricWidgets.contains("winning_trades")) {
        QString value = QString::number(stats.numWinningTrades);
        m_metricWidgets["winning_trades"]->updateValues(value, MetricStatus::Good);
    }
    
    if (m_metricWidgets.contains("losing_trades")) {
        QString value = QString::number(stats.numLosingTrades);
        m_metricWidgets["losing_trades"]->updateValues(value, stats.numLosingTrades > stats.numWinningTrades ? MetricStatus::Bad : MetricStatus::Neutral);
    }
    
    // Best/Worst trades
    if (m_metricWidgets.contains("best_trade")) {
        QString value = QString("%1%").arg(QString::number(stats.bestTradePct, 'f', 2));
        m_metricWidgets["best_trade"]->updateValues(value, MetricStatus::Good);
    }
    
    if (m_metricWidgets.contains("worst_trade")) {
        QString value = QString("%1%").arg(QString::number(stats.worstTradePct, 'f', 2));
        m_metricWidgets["worst_trade"]->updateValues(value, MetricStatus::Bad);
    }
    
    if (m_metricWidgets.contains("avg_trade")) {
        QString value = QString("%1%").arg(QString::number(stats.avgTradePct, 'f', 2));
        MetricStatus status = stats.avgTradePct > 0 ? MetricStatus::Good : 
                             (stats.avgTradePct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["avg_trade"]->updateValues(value, status);
    }
    
    // Profit factor
    if (m_metricWidgets.contains("profit_factor")) {
        QString value = isValidNumber(stats.profitFactor) ? 
                      QString::number(stats.profitFactor, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.profitFactor) ? 
                            (stats.profitFactor > 1.5 ? MetricStatus::Good : 
                             (stats.profitFactor < 1 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["profit_factor"]->updateValues(value, status);
    }
    
    // Expectancy et SQN
    if (m_metricWidgets.contains("expectancy")) {
        QString value = QString("%1%").arg(QString::number(stats.expectancyPct, 'f', 2));
        MetricStatus status = stats.expectancyPct > 0 ? MetricStatus::Good : 
                             (stats.expectancyPct < 0 ? MetricStatus::Bad : MetricStatus::Neutral);
        m_metricWidgets["expectancy"]->updateValues(value, status);
    }
    
    if (m_metricWidgets.contains("sqn")) {
        QString value = isValidNumber(stats.sqn) ? 
                      QString::number(stats.sqn, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.sqn) ? 
                            (stats.sqn > 2 ? MetricStatus::Good : 
                             (stats.sqn < 1 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["sqn"]->updateValues(value, status);
    }
    
    // Kelly Criterion
    if (m_metricWidgets.contains("kelly_criterion")) {
        QString value = isValidNumber(stats.kellyCriterion) ? 
                      QString::number(stats.kellyCriterion, 'f', 2) : "N/A";
        MetricStatus status = isValidNumber(stats.kellyCriterion) ? 
                            (stats.kellyCriterion > 0 ? MetricStatus::Good : 
                             (stats.kellyCriterion < -0.5 ? MetricStatus::Bad : MetricStatus::Neutral)) :
                            MetricStatus::NA;
        m_metricWidgets["kelly_criterion"]->updateValues(value, status);
    }
    
    qDebug() << "Population des métriques terminée avec coloration";
}

void StatsView::createStatsWidgets()
{
    if (!m_performanceLayout || !m_riskLayout || !m_generalLayout) {
        qWarning() << "Layouts non initialisés dans createStatsWidgets";
        return;
    }
    
    qDebug() << "Création des widgets de métriques...";

    // Créer les métriques temporelles
    createTimeSection(m_timeLayout);
    
    // Créer les métriques de performance
    createPerformanceSection(m_performanceLayout);
    
    // Créer les métriques de risque
    createRiskSection(m_riskLayout);
    
    // Créer les métriques générales
    createGeneralSection(m_generalLayout);
    
    qDebug() << "Widgets de métriques créés. Total:" << m_metricWidgets.size();
}


MetricWidget* StatsView::createMetricWidget(const QString& key, const QString& label, 
                                           const QString& value, int row, int col, 
                                           QGridLayout* layout)
{
    if (!layout) {
        qWarning() << "Layout null pour la création du widget métrique:" << key;
        return nullptr;
    }
    
    qDebug() << "Création du widget métrique:" << key << "à la position" << row << "," << col;
    
    MetricWidget* widget = new MetricWidget(label, value);
    layout->addWidget(widget, row, col);
    
    // Stocker le widget dans la map
    m_metricWidgets[key] = widget;
    
    qDebug() << "Widget métrique créé et stocké:" << key;
    
    return widget;
}

void StatsView::createTradesTable()
{
    if (m_tablesCreated) {
        return;
    }
    
    qDebug() << "Création de la table des trades...";
    
    // Créer le groupe pour les trades
    m_tradesGroup = new QGroupBox("Trades Réalisés");
    m_tradesLayout = new QVBoxLayout(m_tradesGroup);
    
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
    
    m_tradesTable->setStyleSheet(tableStyle);
    
    // Ajouter les widgets au layout
    m_tradesLayout->addLayout(tradesControlsLayout);
    m_tradesLayout->addWidget(m_tradesTable);
    
    // Connecter les signaux
    connect(m_tradesLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatsView::refreshTradesTable);
    connect(m_showAllTradesBtn, &QPushButton::clicked, [this]() {
        m_tradesLimitCombo->setCurrentIndex(3); // Index pour "Tous"
        refreshTradesTable();
    });
    
    // Ajouter le groupe à la layout principale
    m_statsContentLayout->addWidget(m_tradesGroup);
    
    // Masquer initialement
    m_tradesGroup->setVisible(false);
    
    qDebug() << "Table des trades créée avec succès";
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

// Nouvelle méthode populateTrades adaptée pour be::Trade
void StatsView::populateTrades(const std::vector<std::shared_ptr<be::Trade>>& trades)
{
    if (!m_tradesModel) {
        return;
    }
    
    qDebug() << "Population de la table des trades";
    
    // Appliquer la limitation si nécessaire
    std::vector<std::shared_ptr<be::Trade>> filteredTrades = trades;
    if (m_tradesLimitCombo) {
        int limit = m_tradesLimitCombo->currentData().toInt();
        if (limit > 0 && filteredTrades.size() > static_cast<size_t>(limit)) {
            // Prendre les derniers trades (les plus récents)
            filteredTrades = std::vector<std::shared_ptr<be::Trade>>(
                filteredTrades.end() - limit, filteredTrades.end()
            );
        }
    }
    
    // Mettre à jour le modèle
    m_tradesModel->updateData(filteredTrades);
    
    // Mettre à jour le texte du bouton avec le nombre total
    if (m_showAllTradesBtn) {
        m_showAllTradesBtn->setText(QString("Afficher tous les trades (%1)").arg(trades.size()));
    }
    
    // Afficher le groupe de trades
    if (m_tradesGroup) {
        m_tradesGroup->setVisible(!trades.empty());
    }
    
    qDebug() << "Table des trades mise à jour avec" << filteredTrades.size() << "/" << trades.size() << "trades";
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