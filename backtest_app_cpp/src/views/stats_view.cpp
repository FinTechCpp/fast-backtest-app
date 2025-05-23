#include "stats_view.h"
#include <QDebug>
#include <QTime>
#include <QObject>  // AJOUT MANQUANT pour connect()

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
    // Effacer le modèle existant
    removeRows(0, rowCount());
    
    // Ajouter les nouvelles données
    int row = 0;
    for (const QVariantMap& trade : trades) {
        QList<QStandardItem*> items;
        
        // Index
        items.append(new QStandardItem(QString::number(row + 1)));
        
        // Entry Time (convertir de l'ISO à un format lisible)
        QDateTime entryTime = QDateTime::fromString(trade["EntryTime"].toString(), Qt::ISODate);
        items.append(new QStandardItem(entryTime.toString("dd.MM.yyyy hh:mm")));
        
        // Type
        QString type = trade["Size"].toDouble() > 0 ? "Buy" : "Sell";
        QStandardItem* typeItem = new QStandardItem(type);
        typeItem->setForeground(trade["Size"].toDouble() > 0 ? QColor("green") : QColor("red"));
        items.append(typeItem);
        
        // Entry Price
        items.append(new QStandardItem(formatNumber(trade["EntryPrice"].toDouble(), 4)));
        
        // Exit Price
        items.append(new QStandardItem(formatNumber(trade["ExitPrice"].toDouble(), 4)));
        
        // Duration
        QString duration = trade["Duration"].toString();
        items.append(new QStandardItem(duration));
        
        // PnL
        double pnl = trade["PnL"].toDouble();
        QStandardItem* pnlItem = new QStandardItem(formatNumber(pnl, 2));
        pnlItem->setForeground(pnl >= 0 ? QColor("green") : QColor("red"));
        items.append(pnlItem);
        
        // PnL%
        double returnPct = trade["ReturnPct"].toDouble() * 100;
        QStandardItem* returnItem = new QStandardItem(formatNumber(returnPct, 2) + "%");
        returnItem->setForeground(returnPct >= 0 ? QColor("green") : QColor("red"));
        items.append(returnItem);
        
        // SL level
        double sl = trade.value("StopLoss", 0).toDouble();
        items.append(new QStandardItem(sl == 0 ? "-" : formatNumber(sl, 4)));
        
        // TP level
        double tp = trade.value("TakeProfit", 0).toDouble();
        items.append(new QStandardItem(tp == 0 ? "-" : formatNumber(tp, 4)));
        
        // Ajouter la ligne au modèle
        appendRow(items);
        row++;
    }
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
    , m_title(nullptr)
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
{
    m_tradesModel = new TradesTableModel(this->m_parent);
    m_equityModel = new TradesTableModel(this->m_parent);
}

StatsView::~StatsView()
{
    // Les modèles sont automatiquement détruits par Qt
}

QWidget* StatsView::create()
{
    QTime start = QTime::currentTime();
    
    QWidget* statsTab = new QWidget(m_parent);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsTab);
    
    m_scrollStats = new QScrollArea();
    m_scrollStats->setWidgetResizable(true);
    
    m_statsContent = new QWidget();
    m_statsContentLayout = new QVBoxLayout(m_statsContent);
    
    m_statsPlaceholder = new QLabel("Exécutez le backtest pour afficher les statistiques");
    m_statsPlaceholder->setAlignment(Qt::AlignCenter);
    m_statsContentLayout->addWidget(m_statsPlaceholder);
    
    // Pré-créer la structure des widgets
    createStatsWidgets();
    
    m_scrollStats->setWidget(m_statsContent);
    statsLayout->addWidget(m_scrollStats);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "StatsView::create() took" << elapsed << "ms";
    
    return statsTab;
}

void StatsView::createStatsWidgets()
{
    // Titre
    m_title = new QLabel("Statistiques du backtest");
    m_title->setStyleSheet("font-size: 18px; font-weight: bold; color: #0066cc;");
    m_title->setVisible(false);
    m_statsContentLayout->addWidget(m_title);
    
    // Groupe des métriques de performance
    m_performanceGroup = new QGroupBox("Performance");
    m_performanceLayout = new QGridLayout();
    createPerformanceSection(m_performanceLayout);
    m_performanceGroup->setLayout(m_performanceLayout);
    m_performanceGroup->setVisible(false);
    m_statsContentLayout->addWidget(m_performanceGroup);
    
    // Groupe des métriques de risque
    m_riskGroup = new QGroupBox("Métriques de risque");
    m_riskLayout = new QGridLayout();
    createRiskSection(m_riskLayout);
    m_riskGroup->setLayout(m_riskLayout);
    m_riskGroup->setVisible(false);
    m_statsContentLayout->addWidget(m_riskGroup);
    
    // Groupe des métriques générales
    m_generalGroup = new QGroupBox("Informations générales");
    m_generalLayout = new QGridLayout();
    createGeneralSection(m_generalLayout);
    m_generalGroup->setLayout(m_generalLayout);
    m_generalGroup->setVisible(false);
    m_statsContentLayout->addWidget(m_generalGroup);
    
    // Groupe des trades
    m_tradesGroup = new QGroupBox("Trades");
    m_tradesLayout = new QVBoxLayout();
    m_tradesGroup->setLayout(m_tradesLayout);
    m_tradesGroup->setVisible(false);
    m_statsContentLayout->addWidget(m_tradesGroup);
    
    // Groupe de l'équité
    m_equityGroup = new QGroupBox("Courbe d'équité");
    m_equityLayout = new QVBoxLayout();
    m_equityGroup->setLayout(m_equityLayout);
    m_equityGroup->setVisible(false);
    m_statsContentLayout->addWidget(m_equityGroup);
}

void StatsView::createPerformanceSection(QGridLayout* layout)
{
    // Rendement total
    createMetricWidget("return_pct", "Rendement total", "", 0, 0, layout);
    
    // Rendement annualisé
    createMetricWidget("return_ann", "Rend. annualisé", "", 0, 1, layout);
    
    // Ratio de Sharpe
    createMetricWidget("sharpe", "Ratio de Sharpe", "", 0, 2, layout);
    
    // Ratio de Sortino
    createMetricWidget("sortino", "Ratio de Sortino", "", 0, 3, layout);
    
    // Ratio Calmar
    createMetricWidget("calmar", "Ratio Calmar", "", 1, 0, layout);
    
    // Nombre de trades
    createMetricWidget("num_trades", "Nombre de trades", "", 1, 1, layout);
    
    // Pourcentage de trades gagnants
    createMetricWidget("win_rate", "% trades gagnants", "", 1, 2, layout);
    
    // Profit Factor
    createMetricWidget("profit_factor", "Profit Factor", "", 1, 3, layout);
}

void StatsView::createRiskSection(QGridLayout* layout)
{
    // Drawdown maximum
    createMetricWidget("max_drawdown", "Drawdown max.", "", 0, 0, layout);
    
    // Durée du drawdown maximum
    createMetricWidget("max_drawdown_duration", "Durée DD max.", "", 0, 1, layout);
    
    // Volatilité annualisée
    createMetricWidget("volatility", "Volatilité ann.", "", 0, 2, layout);
    
    // Ratio Profit/Perte
    createMetricWidget("avg_win_loss", "Profit/Perte moy.", "", 0, 3, layout);
    
    // Gain moyen
    createMetricWidget("avg_win", "Gain moyen", "", 1, 0, layout);
    
    // Perte moyenne
    createMetricWidget("avg_loss", "Perte moyenne", "", 1, 1, layout);
    
    // Trade consécutifs gagnants
    createMetricWidget("consecutive_wins", "Gains consécutifs", "", 1, 2, layout);
    
    // Trades consécutifs perdants
    createMetricWidget("consecutive_losses", "Pertes consécutives", "", 1, 3, layout);
}

void StatsView::createGeneralSection(QGridLayout* layout)
{
    // Période d'analyse
    createMetricWidget("period", "Période", "", 0, 0, layout);
    
    // Durée totale
    createMetricWidget("duration", "Durée", "", 0, 1, layout);
    
    // Exposition au marché
    createMetricWidget("exposure", "Exposition", "", 0, 2, layout);
    
    // Equity finale
    createMetricWidget("equity_final", "Equity finale", "", 0, 3, layout);
    
    // Equity maximum
    createMetricWidget("equity_peak", "Equity max.", "", 1, 0, layout);
    
    // Buy & Hold
    createMetricWidget("buy_hold_return", "Buy & Hold", "", 1, 1, layout);
    
    // Alpha
    createMetricWidget("alpha", "Alpha", "", 1, 2, layout);
    
    // Beta
    createMetricWidget("beta", "Beta", "", 1, 3, layout);
}

void StatsView::createTradesTable()
{
    if (!m_tradesGroup || !m_tradesLayout) {
        return;
    }
    
    // Widget de contrôle pour limiter le nombre de trades
    QWidget* controlWidget = new QWidget();
    QHBoxLayout* controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(0, 0, 0, 10);
    
    // Label et combobox pour limiter les trades
    controlLayout->addWidget(new QLabel("Limite:"));
    m_tradesLimitCombo = new QComboBox();
    m_tradesLimitCombo->addItems({"10", "20", "50", "100", "Tous"});
    m_tradesLimitCombo->setCurrentIndex(0);
    QObject::connect(m_tradesLimitCombo, 
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &StatsView::refreshTradesTable);
    controlLayout->addWidget(m_tradesLimitCombo);
    
    // Bouton pour voir tous les trades
    m_showAllTradesBtn = new QPushButton("Voir tous les trades");
    connect(m_showAllTradesBtn, &QPushButton::clicked, [this]() {
        m_tradesLimitCombo->setCurrentText("Tous");
        refreshTradesTable();
    });
    controlLayout->addWidget(m_showAllTradesBtn);
    
    controlLayout->addStretch();
    
    m_tradesLayout->addWidget(controlWidget);
    
    // Tableau des trades
    m_tradesTable = new QTableView();
    m_tradesTable->setModel(m_tradesModel);
    m_tradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tradesTable->setSortingEnabled(true);
    m_tradesTable->verticalHeader()->setVisible(false);
    m_tradesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tradesTable->horizontalHeader()->setStretchLastSection(true);
    m_tradesTable->setAlternatingRowColors(true);
    
    m_tradesLayout->addWidget(m_tradesTable);
}

void StatsView::createEquityTable()
{
    if (!m_equityGroup || !m_equityLayout) {
        return;
    }
    
    // Widget de contrôle pour l'équité
    QWidget* controlWidget = new QWidget();
    QHBoxLayout* controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(0, 0, 0, 10);
    
    // Bouton pour afficher/masquer l'équité
    m_showEquityBtn = new QPushButton("Afficher la courbe d'équité");
    connect(m_showEquityBtn, &QPushButton::clicked, this, &StatsView::toggleEquityTable);
    controlLayout->addWidget(m_showEquityBtn);
    
    // Combo pour limiter le nombre de lignes
    controlLayout->addWidget(new QLabel("Limite:"));
    m_equityLimitCombo = new QComboBox();
    m_equityLimitCombo->addItems({"50", "100", "200", "500", "Tous"});
    m_equityLimitCombo->setCurrentIndex(0);
    connect(m_equityLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &StatsView::refreshEquityTable);
    controlLayout->addWidget(m_equityLimitCombo);
    
    controlLayout->addStretch();
    
    m_equityLayout->addWidget(controlWidget);
    
    // Stack pour afficher/masquer le tableau
    m_equityStack = new QStackedWidget();
    
    // Widget vide pour le premier état
    QWidget* emptyWidget = new QWidget();
    m_equityStack->addWidget(emptyWidget);
    
    // Tableau de l'équité
    m_equityTable = new QTableView();
    m_equityTable->setModel(m_equityModel);
    m_equityTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_equityTable->setSortingEnabled(false);
    m_equityTable->verticalHeader()->setVisible(false);
    m_equityTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_equityTable->horizontalHeader()->setStretchLastSection(true);
    m_equityTable->setAlternatingRowColors(true);
    
    m_equityStack->addWidget(m_equityTable);
    m_equityLayout->addWidget(m_equityStack);
}

MetricWidget* StatsView::createMetricWidget(const QString& key, const QString& label, 
                                           const QString& value, int row, int col, 
                                           QGridLayout* layout)
{
    MetricWidget* widget = new MetricWidget(label, value);
    layout->addWidget(widget, row, col);
    m_metricWidgets[key] = widget;
    return widget;
}

int StatsView::getLimitValue(QComboBox* combo)
{
    QString text = combo->currentText();
    if (text == "Tous") {
        return -1;  // Pas de limite
    }
    return text.toInt();
}

void StatsView::refreshTradesTable()
{
    if (!m_tablesCreated) {
        return;
    }
    
    // Implémenter l'actualisation de la table des trades en fonction de la limite
}

void StatsView::refreshEquityTable()
{
    if (!m_tablesCreated) {
        return;
    }
    
    // Implémenter l'actualisation de la table d'équité en fonction de la limite
}

void StatsView::toggleEquityTable()
{
    if (!m_tablesCreated) {
        return;
    }
    
    bool isVisible = m_equityStack->currentIndex() == 1;
    m_equityStack->setCurrentIndex(isVisible ? 0 : 1);
    m_showEquityBtn->setText(isVisible ? "Afficher la courbe d'équité" : "Masquer la courbe d'équité");
}

void StatsView::update(void* data, void* stats)
{
    if (!stats) {
        clear();
        return;
    }
    
    QTime start = QTime::currentTime();
    
    // Masquer le placeholder et afficher les groupes
    m_statsPlaceholder->setVisible(false);
    m_title->setVisible(true);
    m_performanceGroup->setVisible(true);
    m_riskGroup->setVisible(true);
    m_generalGroup->setVisible(true);
    m_tradesGroup->setVisible(true);
    m_equityGroup->setVisible(true);
    
    // Créer les tables si pas encore fait
    if (!m_tablesCreated) {
        createTradesTable();
        createEquityTable();
        m_tablesCreated = true;
    }
    
    // Peupler les données
    populateMetrics(stats);
    populateTrades(stats);
    populateEquity(stats);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "StatsView::update() took" << elapsed << "ms";
}

void StatsView::clear()
{
    // Réinitialiser tous les widgets de métriques
    for (auto it = m_metricWidgets.begin(); it != m_metricWidgets.end(); ++it) {
        it.value()->updateValues("");
    }
    
    // Réinitialiser les modèles de tables
    if (m_tradesModel) {
        m_tradesModel->removeRows(0, m_tradesModel->rowCount());
    }
    
    if (m_equityModel) {
        m_equityModel->removeRows(0, m_equityModel->rowCount());
    }
    
    // Afficher le placeholder et masquer les groupes
    m_statsPlaceholder->setVisible(true);
    m_title->setVisible(false);
    m_performanceGroup->setVisible(false);
    m_riskGroup->setVisible(false);
    m_generalGroup->setVisible(false);
    m_tradesGroup->setVisible(false);
    m_equityGroup->setVisible(false);
}

void StatsView::populateMetrics(void* stats)
{
    PyBindingManager* pyManager = PyBindingManager::getInstance();
    
    // Performance metrics
    // Rendement total
    double returnPct = pyManager->getStatValue(stats, "Return [%]").toDouble();
    m_metricWidgets["return_pct"]->updateValues(
        QString::number(returnPct, 'f', 2) + "%", 
        "", returnPct >= 0 ? "normal" : "inverse");
    
    // Rendement annualisé
    double returnAnn = pyManager->getStatValue(stats, "Return (Ann.) [%]").toDouble();
    m_metricWidgets["return_ann"]->updateValues(
        QString::number(returnAnn, 'f', 2) + "%", 
        "", returnAnn >= 0 ? "normal" : "inverse");
    
    // Ratio de Sharpe
    double sharpe = pyManager->getStatValue(stats, "Sharpe Ratio").toDouble();
    m_metricWidgets["sharpe"]->updateValues(
        QString::number(sharpe, 'f', 2), 
        "", sharpe >= 0 ? "normal" : "inverse");
    
    // Ratio de Sortino
    double sortino = pyManager->getStatValue(stats, "Sortino Ratio").toDouble();
    m_metricWidgets["sortino"]->updateValues(
        QString::number(sortino, 'f', 2), 
        "", sortino >= 0 ? "normal" : "inverse");
    
    // Ratio Calmar
    double calmar = pyManager->getStatValue(stats, "Calmar Ratio").toDouble();
    m_metricWidgets["calmar"]->updateValues(
        QString::number(calmar, 'f', 2), 
        "", calmar >= 0 ? "normal" : "inverse");
    
    // Nombre de trades
    int numTrades = pyManager->getStatValue(stats, "# Trades").toInt();
    m_metricWidgets["num_trades"]->updateValues(QString::number(numTrades));
    
    // Win rate
    double winRate = pyManager->getStatValue(stats, "Win Rate [%]").toDouble();
    m_metricWidgets["win_rate"]->updateValues(
        QString::number(winRate, 'f', 2) + "%", 
        "", winRate >= 50 ? "normal" : "inverse");
    
    // Profit Factor
    double profitFactor = pyManager->getStatValue(stats, "Profit Factor").toDouble();
    m_metricWidgets["profit_factor"]->updateValues(
        QString::number(profitFactor, 'f', 2), 
        "", profitFactor >= 1 ? "normal" : "inverse");
    
    // Risk metrics
    // Drawdown maximum
    double maxDrawdown = pyManager->getStatValue(stats, "Max. Drawdown [%]").toDouble();
    m_metricWidgets["max_drawdown"]->updateValues(
        QString::number(maxDrawdown, 'f', 2) + "%", 
        "", "inverse");
    
    // Durée du drawdown
    QString maxDrawdownDuration = pyManager->getStatValue(stats, "Max. Drawdown Duration").toString();
    m_metricWidgets["max_drawdown_duration"]->updateValues(maxDrawdownDuration);
    
    // Volatilité annualisée
    double volatility = pyManager->getStatValue(stats, "Volatility (Ann.) [%]").toDouble();
    m_metricWidgets["volatility"]->updateValues(
        QString::number(volatility, 'f', 2) + "%", 
        "", "normal");
    
    // Ratio Avg Win / Avg Loss
    double avgWin = pyManager->getStatValue(stats, "Avg. Trade [%]").toDouble();
    double avgLoss = pyManager->getStatValue(stats, "Avg. Loss [%]").toDouble();
    double avgWinLoss = (avgLoss != 0) ? std::abs(avgWin / avgLoss) : 0;
    m_metricWidgets["avg_win_loss"]->updateValues(
        QString::number(avgWinLoss, 'f', 2), 
        "", avgWinLoss >= 1 ? "normal" : "inverse");
    
    // Gain moyen
    m_metricWidgets["avg_win"]->updateValues(
        QString::number(avgWin, 'f', 2) + "%", 
        "", avgWin >= 0 ? "normal" : "inverse");
    
    // Perte moyenne
    m_metricWidgets["avg_loss"]->updateValues(
        QString::number(avgLoss, 'f', 2) + "%", 
        "", avgLoss >= 0 ? "normal" : "inverse");
    
    // Trades consécutifs gagnants
    int consecutiveWins = pyManager->getStatValue(stats, "Longest Win Streak").toInt();
    m_metricWidgets["consecutive_wins"]->updateValues(QString::number(consecutiveWins));
    
    // Trades consécutifs perdants
    int consecutiveLosses = pyManager->getStatValue(stats, "Longest Losing Streak").toInt();
    m_metricWidgets["consecutive_losses"]->updateValues(QString::number(consecutiveLosses));
    
    // General metrics
    // Période
    QDateTime start = pyManager->getStatValue(stats, "Start").toDateTime();
    QDateTime end = pyManager->getStatValue(stats, "End").toDateTime();
    m_metricWidgets["period"]->updateValues(
        start.toString("dd.MM.yyyy") + " - " + end.toString("dd.MM.yyyy"));
    
    // Durée
    QString duration = pyManager->getStatValue(stats, "Duration").toString();
    m_metricWidgets["duration"]->updateValues(duration);
    
    // Exposition
    double exposure = pyManager->getStatValue(stats, "Exposure Time [%]").toDouble();
    m_metricWidgets["exposure"]->updateValues(QString::number(exposure, 'f', 2) + "%");
    
    // Equity finale
    double equityFinal = pyManager->getStatValue(stats, "Equity Final [$]").toDouble();
    m_metricWidgets["equity_final"]->updateValues(QString::number(equityFinal, 'f', 2));
    
    // Equity max
    double equityPeak = pyManager->getStatValue(stats, "Equity Peak [$]").toDouble();
    m_metricWidgets["equity_peak"]->updateValues(QString::number(equityPeak, 'f', 2));
    
    // Buy & Hold
    double buyHold = pyManager->getStatValue(stats, "Buy & Hold Return [%]").toDouble();
    m_metricWidgets["buy_hold_return"]->updateValues(
        QString::number(buyHold, 'f', 2) + "%", 
        "", buyHold >= 0 ? "normal" : "inverse");
    
    // Alpha
    double alpha = pyManager->getStatValue(stats, "Alpha").toDouble();
    m_metricWidgets["alpha"]->updateValues(
        QString::number(alpha, 'f', 2), 
        "", alpha >= 0 ? "normal" : "inverse");
    
    // Beta
    double beta = pyManager->getStatValue(stats, "Beta").toDouble();
    m_metricWidgets["beta"]->updateValues(QString::number(beta, 'f', 2));
}

void StatsView::populateTrades(void* stats)
{
    PyBindingManager* pyManager = PyBindingManager::getInstance();
    QList<QVariantMap> trades = pyManager->getTrades(stats);
    
    // Mettre à jour le modèle avec les données
    m_tradesModel->updateData(trades);
    
    // Limiter le nombre de trades affichés
    int limit = getLimitValue(m_tradesLimitCombo);
    if (limit > 0 && limit < trades.size()) {
        for (int i = limit; i < trades.size(); ++i) {
            m_tradesModel->removeRow(limit);
        }
    }
}

void StatsView::populateEquity(void* stats)
{
    PyBindingManager* pyManager = PyBindingManager::getInstance();
    QList<QVariantMap> equity = pyManager->getEquity(stats);
    
    // Mettre à jour les en-têtes du modèle d'équité
    QStringList headers;
    if (!equity.isEmpty()) {
        headers << "#" << "Date";
        for (auto key : equity.first().keys()) {
            if (key != "Date") {
                headers << key;
            }
        }
    } else {
        headers << "#" << "Date" << "Equity" << "DrawdownPct";
    }
    m_equityModel->setHorizontalHeaderLabels(headers);
    
    // Ajouter les données d'équité
    int row = 0;
    for (const QVariantMap& point : equity) {
        QList<QStandardItem*> items;
        
        // Index
        items.append(new QStandardItem(QString::number(row + 1)));
        
        // Date
        QDateTime date = point["Date"].toDateTime();
        items.append(new QStandardItem(date.toString("dd.MM.yyyy hh:mm")));
        
        // Autres colonnes
        for (auto it = point.begin(); it != point.end(); ++it) {
            if (it.key() != "Date") {
                QStandardItem* item = new QStandardItem();
                if (it.value().type() == QVariant::Double) {
                    double value = it.value().toDouble();
                    item->setText(QString::number(value, 'f', 2));
                    
                    // Colorer la colonne Drawdown en rouge
                    if (it.key() == "DrawdownPct") {
                        item->setForeground(QColor("red"));
                    }
                } else {
                    item->setText(it.value().toString());
                }
                items.append(item);
            }
        }
        
        m_equityModel->appendRow(items);
        row++;
    }
    
    // Limiter le nombre de points d'équité affichés
    int limit = getLimitValue(m_equityLimitCombo);
    if (limit > 0 && limit < equity.size()) {
        for (int i = limit; i < equity.size(); ++i) {
            m_equityModel->removeRow(limit);
        }
    }
}