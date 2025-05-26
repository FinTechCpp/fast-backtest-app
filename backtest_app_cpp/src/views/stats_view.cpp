#include "stats_view.h"
#include <QDebug>
#include <QTime>

// Implémentation de TradesTableModel
TradesTableModel::TradesTableModel(QObject* parent)
    : QStandardItemModel(parent)
{
    // Définir les en-têtes par défaut
    QStringList headers;
    headers << "#" << "Date" << "Type" << "Entrée" << "Sortie" << "Dur." << "PnL" << "PnL%" << "SL" << "TP";
    setHorizontalHeaderLabels(headers);
}

void TradesTableModel::updateData(const QList<QVariantMap>& trades)
{
    beginResetModel();
    
    // Effacer les données existantes
    removeRows(0, rowCount());
    
    if (trades.isEmpty()) {
        endResetModel();
        return;
    }
    
    // Configurer les en-têtes selon les colonnes du DataFrame _trades
    QStringList headers;
    headers << "#" << "Type" << "Taille" << "Prix d'entrée" << "Prix de sortie" 
            << "PnL" << "PnL %" << "Durée" << "Date d'entrée" << "Date de sortie"
            << "SL" << "TP" << "Tag";
    setHorizontalHeaderLabels(headers);
    
    // Ajouter les nouvelles données
    setRowCount(trades.size());
    
    for (int row = 0; row < trades.size(); ++row) {
        const QVariantMap& trade = trades[row];
        
        // # (numéro de trade)
        setItem(row, 0, new QStandardItem(QString::number(row + 1)));
        
        // Type (LONG/SHORT basé sur la taille)
        double size = trade.value("Size").toDouble();
        QString tradeType = size > 0 ? "LONG" : "SHORT";
        QStandardItem* typeItem = new QStandardItem(tradeType);
        typeItem->setForeground(size > 0 ? Qt::darkGreen : Qt::darkRed);
        setItem(row, 1, typeItem);
        
        // Taille (valeur absolue)
        setItem(row, 2, new QStandardItem(formatNumber(qAbs(size), 4)));
        
        // Prix d'entrée
        setItem(row, 3, new QStandardItem(formatNumber(trade.value("EntryPrice").toDouble(), 2)));
        
        // Prix de sortie
        setItem(row, 4, new QStandardItem(formatNumber(trade.value("ExitPrice").toDouble(), 2)));
        
        // PnL
        double pnl = trade.value("PnL").toDouble();
        QStandardItem* pnlItem = new QStandardItem(formatNumber(pnl, 2));
        pnlItem->setForeground(pnl >= 0 ? Qt::darkGreen : Qt::darkRed);
        setItem(row, 5, pnlItem);
        
        // PnL %
        double returnPct = trade.value("ReturnPct").toDouble();
        QStandardItem* pctItem = new QStandardItem(formatNumber(returnPct * 100, 2) + "%");
        pctItem->setForeground(returnPct >= 0 ? Qt::darkGreen : Qt::darkRed);
        setItem(row, 6, pctItem);
        
        // Durée
        QString duration = trade.value("Duration").toString();
        setItem(row, 7, new QStandardItem(formatDuration(duration)));
        
        // Date d'entrée
        QString entryTime = trade.value("EntryTime").toString();
        setItem(row, 8, new QStandardItem(formatDateTime(entryTime)));
        
        // Date de sortie
        QString exitTime = trade.value("ExitTime").toString();
        setItem(row, 9, new QStandardItem(formatDateTime(exitTime)));
        
        // Stop Loss
        QVariant slValue = trade.value("SL");
        QString slText = slValue.isNull() || slValue.toString() == "None" ? "-" : 
                        formatNumber(slValue.toDouble(), 2);
        setItem(row, 10, new QStandardItem(slText));
        
        // Take Profit
        QVariant tpValue = trade.value("TP");
        QString tpText = tpValue.isNull() || tpValue.toString() == "None" ? "-" : 
                        formatNumber(tpValue.toDouble(), 2);
        setItem(row, 11, new QStandardItem(tpText));
        
        // Tag
        QString tag = trade.value("Tag").toString();
        if (tag.isEmpty() || tag == "None") {
            tag = "-";
        }
        setItem(row, 12, new QStandardItem(tag));
    }
    
    endResetModel();
}

// Ajouter cette méthode helper dans TradesTableModel
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

QString TradesTableModel::formatDateTime(const QString& dateTime)
{
    // Format "2025-03-27 15:30:40" -> "27/03 15:30:40"
    QDateTime dt = QDateTime::fromString(dateTime, "yyyy-MM-dd hh:mm:ss");
    if (dt.isValid()) {
        return dt.toString("dd/MM hh:mm:ss");
    }
    return dateTime;
}

QString TradesTableModel::formatNumber(double value, int precision)
{
    return QString::number(value, 'f', precision);
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
    , m_currentStats(nullptr)
{
    qDebug() << "StatsView créée avec parent:" << parent;
    
    // Créer les modèles de données
    m_tradesModel = new TradesTableModel(this);
    
    // Construire l'interface dans le constructeur
    setupUI();
}

StatsView::~StatsView()
{
    // Les modèles et widgets sont automatiquement détruits par Qt
}

void StatsView::setupUI()
{
    QTime start = QTime::currentTime();
    
    // Créer le scroll area principal
    m_scrollStats = new QScrollArea();
    m_scrollStats->setWidgetResizable(true);
    
    // Créer le widget de contenu
    m_statsContent = new QWidget();
    m_statsContentLayout = new QVBoxLayout(m_statsContent);
    
    // Placeholder initial
    m_statsPlaceholder = new QLabel("Exécutez le backtest pour afficher les statistiques");
    m_statsPlaceholder->setAlignment(Qt::AlignCenter);
    m_statsContentLayout->addWidget(m_statsPlaceholder);
    
    // Créer les sections de métriques
    m_performanceGroup = new QGroupBox("Performance");
    m_performanceLayout = new QGridLayout();
    m_performanceGroup->setLayout(m_performanceLayout);
    
    m_riskGroup = new QGroupBox("Risque");
    m_riskLayout = new QGridLayout();
    m_riskGroup->setLayout(m_riskLayout);
    
    m_generalGroup = new QGroupBox("Général");
    m_generalLayout = new QGridLayout();
    m_generalGroup->setLayout(m_generalLayout);
    
    // Créer les widgets de métriques
    createStatsWidgets();
    
    // Ajouter les groupes au layout du contenu
    m_statsContentLayout->addWidget(m_performanceGroup);
    m_statsContentLayout->addWidget(m_riskGroup);
    m_statsContentLayout->addWidget(m_generalGroup);
    
    // Configurer le scroll area
    m_scrollStats->setWidget(m_statsContent);
    
    // Ajouter le scroll area au layout principal (hérité de BaseView)
    m_mainLayout->addWidget(m_scrollStats);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "StatsView::setupUI() took" << elapsed << "ms";
}

void StatsView::createStatsWidgets()
{
    if (!m_performanceLayout || !m_riskLayout || !m_generalLayout) {
        qWarning() << "Layouts non initialisés dans createStatsWidgets";
        return;
    }
    
    qDebug() << "Création des widgets de métriques...";
    
    // Créer les métriques de performance
    createPerformanceSection(m_performanceLayout);
    
    // Créer les métriques de risque
    createRiskSection(m_riskLayout);
    
    // Créer les métriques générales
    createGeneralSection(m_generalLayout);
    
    qDebug() << "Widgets de métriques créés. Total:" << m_metricWidgets.size();
}

void StatsView::createPerformanceSection(QGridLayout* layout)
{
    if (!layout) {
        qWarning() << "Layout de performance null";
        return;
    }
    
    int row = 0;
    
    // Période
    createMetricWidget("start", "Début", "N/A", row, 0, layout);
    createMetricWidget("end", "Fin", "N/A", row++, 1, layout);
    createMetricWidget("duration", "Durée", "N/A", row++, 0, layout);
    
    // Performance principale
    createMetricWidget("total_return", "Rendement Total", "N/A", row, 0, layout);
    createMetricWidget("return_ann", "Rendement Annualisé", "N/A", row++, 1, layout);
    createMetricWidget("buy_hold_return", "Buy & Hold", "N/A", row, 0, layout);
    createMetricWidget("cagr", "CAGR", "N/A", row++, 1, layout);
    
    // Alpha/Beta
    createMetricWidget("alpha", "Alpha", "N/A", row, 0, layout);
    createMetricWidget("beta", "Beta", "N/A", row++, 1, layout);
    
    // Equity
    createMetricWidget("equity_final", "Equity Final", "N/A", row, 0, layout);
    createMetricWidget("equity_peak", "Equity Peak", "N/A", row++, 1, layout);
}

void StatsView::createRiskSection(QGridLayout* layout)
{
    if (!layout) {
        qWarning() << "Layout de risque null";
        return;
    }
    
    int row = 0;
    
    // Ratios de risque
    createMetricWidget("sharpe_ratio", "Ratio de Sharpe", "N/A", row, 0, layout);
    createMetricWidget("sortino_ratio", "Ratio de Sortino", "N/A", row++, 1, layout);
    createMetricWidget("calmar_ratio", "Ratio de Calmar", "N/A", row++, 0, layout);
    
    // Drawdown
    createMetricWidget("max_drawdown", "Drawdown Max", "N/A", row, 0, layout);
    createMetricWidget("avg_drawdown", "Drawdown Moyen", "N/A", row++, 1, layout);
    createMetricWidget("max_drawdown_duration", "Durée DD Max", "N/A", row, 0, layout);
    createMetricWidget("avg_drawdown_duration", "Durée DD Moy", "N/A", row++, 1, layout);
    
    // Volatilité
    createMetricWidget("volatility", "Volatilité Ann.", "N/A", row++, 0, layout);
}

void StatsView::createGeneralSection(QGridLayout* layout)
{
    if (!layout) {
        qWarning() << "Layout général null";
        return;
    }
    
    int row = 0;
    
    // Exposition et trades
    createMetricWidget("exposure_time", "Temps d'Exposition", "N/A", row, 0, layout);
    createMetricWidget("total_trades", "Nombre de Trades", "N/A", row++, 1, layout);
    
    // Statistiques de trades
    createMetricWidget("win_rate", "Taux de Réussite", "N/A", row, 0, layout);
    createMetricWidget("winning_trades", "Trades Gagnants", "N/A", row++, 1, layout);
    createMetricWidget("losing_trades", "Trades Perdants", "N/A", row, 0, layout);
    createMetricWidget("neutral_trades", "Trades Neutres", "N/A", row++, 1, layout);
    
    // Performance des trades
    createMetricWidget("best_trade", "Meilleur Trade", "N/A", row, 0, layout);
    createMetricWidget("worst_trade", "Pire Trade", "N/A", row++, 1, layout);
    createMetricWidget("avg_trade", "Trade Moyen", "N/A", row, 0, layout);
    createMetricWidget("profit_factor", "Facteur de Profit", "N/A", row++, 1, layout);
    
    // Durées des trades
    createMetricWidget("max_trade_duration", "Durée Max Trade", "N/A", row, 0, layout);
    createMetricWidget("avg_trade_duration", "Durée Moy Trade", "N/A", row++, 1, layout);
    
    // Métriques avancées
    createMetricWidget("expectancy", "Espérance", "N/A", row, 0, layout);
    createMetricWidget("sqn", "SQN", "N/A", row++, 1, layout);
    createMetricWidget("kelly_criterion", "Critère de Kelly", "N/A", row++, 0, layout);
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

void StatsView::populateMetrics(void* stats)
{
    if (!stats) {
        qWarning() << "Stats null dans populateMetrics";
        return;
    }
    
    qDebug() << "Population des métriques avec" << m_metricWidgets.size() << "widgets disponibles";
    
    // Mapping correct des clés selon compute_stats()
    QMap<QString, QString> keyMapping = {
        // Période et durée
        {"start", "Start"},
        {"end", "End"},
        {"duration", "Duration"},
        
        // Performance
        {"total_return", "Return [%]"},
        {"return_ann", "Return (Ann.) [%]"},
        {"buy_hold_return", "Buy & Hold Return [%]"},
        {"cagr", "CAGR [%]"},
        {"alpha", "Alpha [%]"},
        {"beta", "Beta"},
        
        // Risque
        {"sharpe_ratio", "Sharpe Ratio"},
        {"sortino_ratio", "Sortino Ratio"},
        {"calmar_ratio", "Calmar Ratio"},
        {"max_drawdown", "Max. Drawdown [%]"},
        {"avg_drawdown", "Avg. Drawdown [%]"},
        {"max_drawdown_duration", "Max. Drawdown Duration"},
        {"avg_drawdown_duration", "Avg. Drawdown Duration"},
        {"volatility", "Volatility (Ann.) [%]"},
        
        // Trading
        {"exposure_time", "Exposure Time [%]"},
        {"total_trades", "# Trades"},
        {"win_rate", "Win Rate [%]"},
        {"winning_trades", "# Winning Trades"},
        {"losing_trades", "# Losing Trades"},
        {"neutral_trades", "# Neutral Trades"},
        
        // Equity
        {"equity_final", "Equity Final [$]"},
        {"equity_peak", "Equity Peak [$]"},
        
        // Trade performance
        {"best_trade", "Best Trade [%]"},
        {"worst_trade", "Worst Trade [%]"},
        {"avg_trade", "Avg. Trade [%]"},
        {"max_trade_duration", "Max. Trade Duration"},
        {"avg_trade_duration", "Avg. Trade Duration"},
        {"profit_factor", "Profit Factor"},
        {"expectancy", "Expectancy [%]"},
        {"sqn", "SQN"},
        {"kelly_criterion", "Kelly Criterion"}
    };
    
    PyBindingManager& pyManager = PyBindingManager::getInstance();
    
    for (auto it = keyMapping.begin(); it != keyMapping.end(); ++it) {
        QString qtKey = it.key();
        QString pythonKey = it.value();
        
        if (m_metricWidgets.contains(qtKey)) {
            try {
                QVariant rawValue = pyManager.getStatValue(stats, pythonKey);
                QString formattedValue;
                
                // Formatage spécifique selon le type de métrique
                if (qtKey.endsWith("_duration") || pythonKey.contains("Duration")) {
                    // Formatage des durées
                    formattedValue = formatDuration(rawValue);
                }
                else if (pythonKey.contains("[%]")) {
                    // Formatage des pourcentages
                    bool ok;
                    double value = rawValue.toDouble(&ok);
                    if (ok && !std::isnan(value) && !std::isinf(value)) {
                        formattedValue = QString("%1%").arg(QString::number(value, 'f', 2));
                    } else {
                        formattedValue = "N/A";
                    }
                }
                else if (pythonKey.contains("[$]")) {
                    // Formatage des devises
                    bool ok;
                    double value = rawValue.toDouble(&ok);
                    if (ok && !std::isnan(value) && !std::isinf(value)) {
                        formattedValue = formatCurrency(value);
                    } else {
                        formattedValue = "N/A";
                    }
                }
                else if (qtKey == "total_trades" || qtKey == "winning_trades" || 
                         qtKey == "losing_trades" || qtKey == "neutral_trades") {
                    // Formatage des entiers
                    bool ok;
                    int value = rawValue.toInt(&ok);
                    if (ok) {
                        formattedValue = QString::number(value);
                    } else {
                        formattedValue = "N/A";
                    }
                }
                else if (qtKey == "start" || qtKey == "end") {
                    // Formatage des dates
                    formattedValue = rawValue.toString();
                    if (formattedValue.contains("T")) {
                        // Format ISO - extraire juste la date
                        formattedValue = formattedValue.split("T").first();
                    }
                }
                else {
                    // Formatage générique pour les ratios et autres valeurs numériques
                    bool ok;
                    double value = rawValue.toDouble(&ok);
                    if (ok && !std::isnan(value) && !std::isinf(value)) {
                        formattedValue = QString::number(value, 'f', 3);
                    } else {
                        formattedValue = rawValue.toString();
                        if (formattedValue.isEmpty() || formattedValue == "nan") {
                            formattedValue = "N/A";
                        }
                    }
                }
                
                // Mettre à jour le widget
                m_metricWidgets[qtKey]->updateValues(formattedValue);
                
            } catch (const std::exception& e) {
                qWarning() << "Erreur lors de l'extraction de" << pythonKey << ":" << e.what();
                m_metricWidgets[qtKey]->updateValues("N/A");
            }
        }
    }
    
    qDebug() << "Population des métriques terminée";
}

void StatsView::populateTrades(void* stats)
{
    if (!stats || !m_tradesModel) {
        return;
    }
    
    qDebug() << "Population de la table des trades";
    
    try {
        PyBindingManager& pyManager = PyBindingManager::getInstance();
        QList<QVariantMap> trades = pyManager.getTradesData(stats);
        
        m_tradesModel->updateData(trades);
        
        if (m_tradesGroup) {
            m_tradesGroup->setVisible(!trades.isEmpty());
        }
        
        // Mettre à jour le bouton avec le nombre total
        if (m_showAllTradesBtn) {
            m_showAllTradesBtn->setText(QString("Afficher tous les trades (%1)").arg(trades.size()));
        }
        
        qDebug() << "Table des trades mise à jour avec" << trades.size() << "trades";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la population des trades:" << e.what();
    }
}

void StatsView::populateEquity(void* stats)
{
    if (!stats || !m_equityModel) {
        return;
    }
    
    qDebug() << "Population de la table d'équité";
    
    try {
        PyBindingManager& pyManager = PyBindingManager::getInstance();
        QList<QVariantMap> equity = pyManager.getEquityData(stats);
        
        m_equityModel->updateData(equity);
        
        if (m_equityGroup) {
            m_equityGroup->setVisible(!equity.isEmpty());
        }
        
        qDebug() << "Table d'équité mise à jour avec" << equity.size() << "entrées";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la population de l'équité:" << e.what();
    }
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

QString StatsView::formatDuration(const QVariant& value)
{
    QString strValue = value.toString();
    
    // Si c'est déjà une chaîne formatée pandas (ex: "8 days 23:30:20")
    if (strValue.contains("days") && strValue.contains(":")) {
        // Parser le format pandas: "8 days 23:30:20"
        QStringList parts = strValue.split(" ");
        if (parts.size() >= 3) {
            int days = parts[0].toInt();
            QStringList timeParts = parts[2].split(":");
            if (timeParts.size() >= 3) {
                int hours = timeParts[0].toInt();
                int minutes = timeParts[1].toInt();
                int seconds = timeParts[2].toInt();
                
                return formatDetailedDuration(days, hours, minutes, seconds);
            }
        }
    }
    
    // Si c'est "0 days 00:00:00" ou équivalent
    if (strValue.contains("0 days 00:00:00") || strValue == "0") {
        return "0sec";
    }
    
    // Si c'est NaN ou vide
    if (strValue.isEmpty() || strValue == "nan" || strValue == "NaT") {
        return "N/A";
    }
    
    // Essayer de convertir en nombre (peut-être en jours décimaux)
    bool ok;
    double numValue = value.toDouble(&ok);
    if (ok && !std::isnan(numValue) && !std::isinf(numValue)) {
        if (numValue == 0) {
            return "0sec";
        }
        
        // Convertir les jours décimaux en composants
        int totalSeconds = static_cast<int>(numValue * 24 * 3600);
        int days = totalSeconds / (24 * 3600);
        totalSeconds %= (24 * 3600);
        int hours = totalSeconds / 3600;
        totalSeconds %= 3600;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        
        return formatDetailedDuration(days, hours, minutes, seconds);
    }
    
    return strValue;
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

void StatsView::refreshTradesTable()
{
    qDebug() << "StatsView::refreshTradesTable() appelé";
    
    if (!m_tradesModel || !m_currentStats) {
        qWarning() << "Modèle de trades ou stats non initialisés";
        return;
    }
    
    // Récupérer les données de trades via PyBindingManager
    PyBindingManager& pyManager = PyBindingManager::getInstance();
    QList<QVariantMap> allTrades = pyManager.getTradesData(m_currentStats);
    
    // Appliquer la limitation si nécessaire
    QList<QVariantMap> trades = allTrades;
    if (m_tradesLimitCombo) {
        int limit = m_tradesLimitCombo->currentData().toInt();
        if (limit > 0 && allTrades.size() > limit) {
            // Prendre les derniers trades (les plus récents)
            trades = allTrades.mid(allTrades.size() - limit);
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


void StatsView::updateData(void* data, void* stats)
{
    Q_UNUSED(data);

    QTime start = QTime::currentTime();
    qDebug() << "StatsView::updateData() appelé";
    
    // Stocker les stats pour les mises à jour ultérieures
    m_currentStats = stats;
    
    if (!stats) {
        qWarning() << "Stats nulles reçues";
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
        
        // Mettre à jour les métriques
        populateMetrics(stats);
        
        // Mettre à jour les tables
        populateTrades(stats);
        populateEquity(stats);
        
        qInfo() << "StatsView mise à jour avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la mise à jour StatsView:" << e.what();
    }
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "StatsView::updateData() took" << elapsed << "ms";
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
    m_currentStats = nullptr;
    
    qDebug() << "StatsView nettoyée";
}