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
    
    // Configurer les en-têtes basés sur les données
    QStringList headers;
    headers << "#" << "Type" << "Taille" << "Prix d'entrée" << "Prix de sortie" 
            << "PnL" << "PnL %" << "Durée" << "Date d'entrée" << "Date de sortie";
    setHorizontalHeaderLabels(headers);
    
    // Ajouter les nouvelles données
    setRowCount(trades.size());
    
    for (int row = 0; row < trades.size(); ++row) {
        const QVariantMap& trade = trades[row];
        
        setItem(row, 0, new QStandardItem(QString::number(row + 1)));
        setItem(row, 1, new QStandardItem(trade.value("Size").toDouble() > 0 ? "LONG" : "SHORT"));
        setItem(row, 2, new QStandardItem(formatNumber(qAbs(trade.value("Size").toDouble()), 4)));
        setItem(row, 3, new QStandardItem(formatNumber(trade.value("EntryPrice").toDouble(), 2)));
        setItem(row, 4, new QStandardItem(formatNumber(trade.value("ExitPrice").toDouble(), 2)));
        setItem(row, 5, new QStandardItem(formatNumber(trade.value("PnL").toDouble(), 2)));
        setItem(row, 6, new QStandardItem(formatNumber(trade.value("ReturnPct").toDouble(), 2) + "%"));
        setItem(row, 7, new QStandardItem(trade.value("Duration").toString()));
        setItem(row, 8, new QStandardItem(trade.value("EntryTime").toString()));
        setItem(row, 9, new QStandardItem(trade.value("ExitTime").toString()));
        
        // Colorer les PnL selon le gain/perte
        double pnl = trade.value("PnL").toDouble();
        QColor color = pnl >= 0 ? Qt::darkGreen : Qt::darkRed;
        item(row, 5)->setForeground(color);
        item(row, 6)->setForeground(color);
    }
    
    endResetModel();
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
    , m_equityTable(nullptr)
    , m_showEquityBtn(nullptr)
    , m_equityLimitCombo(nullptr)
    , m_equityStack(nullptr)
    , m_tablesCreated(false)
    , m_currentStats(nullptr)
{
    qDebug() << "StatsView créée avec parent:" << parent;
    
    // Créer les modèles de données
    m_tradesModel = new TradesTableModel(this);
    m_equityModel = new TradesTableModel(this);
    
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
    // TODO: Implémenter la création de la table des trades
}

void StatsView::createEquityTable()
{
    // TODO: Implémenter la création de la table d'équité
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
                if (qtKey.contains("return") || qtKey.contains("drawdown") || 
                    qtKey.contains("volatility") || qtKey.contains("cagr") || 
                    qtKey.contains("alpha") || qtKey.contains("expectancy") ||
                    qtKey.endsWith("_trade")) {
                    // Pourcentages
                    formattedValue = formatPercentage(rawValue.toDouble());
                } else if (qtKey.contains("equity") || qtKey.contains("final") || 
                          qtKey.contains("peak")) {
                    // Valeurs monétaires
                    formattedValue = formatCurrency(rawValue.toDouble());
                } else if (qtKey.contains("duration")) {
                    // Durées
                    formattedValue = rawValue.toString();
                } else if (qtKey == "start" || qtKey == "end") {
                    // Dates - convertir le timestamp Python en date lisible
                    QDateTime dateTime = QDateTime::fromString(rawValue.toString(), Qt::ISODate);
                    if (dateTime.isValid()) {
                        formattedValue = dateTime.toString("yyyy-MM-dd hh:mm");
                    } else {
                        formattedValue = rawValue.toString();
                    }
                } else {
                    // Valeurs numériques standard
                    double value = rawValue.toDouble();
                    if (std::isnan(value) || std::isinf(value)) {
                        formattedValue = "N/A";
                    } else {
                        formattedValue = QString::number(value, 'f', 4);
                    }
                }
                
                updateMetricWidget(qtKey, pythonKey, formattedValue);
                
            } catch (const std::exception& e) {
                qWarning() << "Erreur lors de l'extraction de" << pythonKey << ":" << e.what();
                updateMetricWidget(qtKey, pythonKey, "N/A");
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
    
    // Si c'est déjà une chaîne formatée (ex: "584 days 00:00:00"), la retourner telle quelle
    if (strValue.contains("days") || strValue.contains("hours") || strValue.contains("minutes")) {
        return strValue;
    }
    
    // Sinon, essayer de la convertir
    bool ok;
    double numValue = value.toDouble(&ok);
    if (ok) {
        // Supposer que c'est en jours
        int days = static_cast<int>(numValue);
        double hours = (numValue - days) * 24;
        return QString("%1 jours %2h").arg(days).arg(QString::number(hours, 'f', 1));
    }
    
    return strValue;
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
    QList<QVariantMap> trades = pyManager.getTrades(m_currentStats);
    
    // Mettre à jour le modèle
    m_tradesModel->updateData(trades);
    
    qDebug() << "Table des trades mise à jour avec" << trades.size() << "trades";
}

void StatsView::refreshEquityTable()
{
    qDebug() << "StatsView::refreshEquityTable() appelé";
    
    if (!m_equityModel || !m_currentStats) {
        qWarning() << "Modèle d'équité ou stats non initialisés";
        return;
    }
    
    // Récupérer les données d'équité via PyBindingManager
    PyBindingManager& pyManager = PyBindingManager::getInstance();
    QList<QVariantMap> equity = pyManager.getEquityCurve(m_currentStats);
    
    // Mettre à jour le modèle
    m_equityModel->updateData(equity);
    
    qDebug() << "Table d'équité mise à jour avec" << equity.size() << "points";
}

void StatsView::toggleEquityTable()
{
    qDebug() << "StatsView::toggleEquityTable() appelé";
    
    if (!m_equityStack) {
        qWarning() << "Stack d'équité non initialisé";
        return;
    }
    
    // Alterner entre l'affichage de la table et un placeholder
    if (m_equityStack->currentIndex() == 0) {
        // Montrer la table d'équité
        refreshEquityTable();
        m_equityStack->setCurrentIndex(1);
        if (m_showEquityBtn) {
            m_showEquityBtn->setText("Masquer la courbe d'équité");
        }
    } else {
        // Cacher la table d'équité
        m_equityStack->setCurrentIndex(0);
        if (m_showEquityBtn) {
            m_showEquityBtn->setText("Afficher la courbe d'équité");
        }
    }
}

void StatsView::updateData(void* data, void* stats)
{
    Q_UNUSED(data);  // Les données ne sont pas utilisées pour les stats

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
            createEquityTable();
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