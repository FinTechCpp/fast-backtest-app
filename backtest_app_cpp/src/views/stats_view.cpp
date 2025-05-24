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
    // Effacer le modèle existant
    removeRows(0, rowCount());
    
    // TODO: Ajouter les nouvelles données
    int row = 0;
    for (const QVariantMap& trade : trades) {
        // Implémenter l'ajout des données de trade
        Q_UNUSED(trade);
        row++;
    }
}

QString TradesTableModel::formatNumber(double value, int precision)
{
    return QString::number(value, 'f', precision);
}

// Implémentation de StatsView
StatsView::StatsView(QObject* parent)
    : BaseView(parent)
    , m_tradesModel(nullptr)  // CORRECTION: Initialiser à nullptr
    , m_equityModel(nullptr)  // CORRECTION: Initialiser à nullptr
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
    // CORRECTION: Ne pas créer de widgets Qt dans le constructeur
    qDebug() << "StatsView créée avec parent:" << parent;
}

StatsView::~StatsView()
{
    // Les modèles et widgets sont automatiquement détruits par Qt
}

QWidget* StatsView::create(QWidget* parentWidget)
{
    QTime start = QTime::currentTime();
    
    // Stocker le widget parent pour utilisation ultérieure
    m_parentWidget = parentWidget;
    
    // CORRECTION: Créer les modèles ici, pas dans le constructeur
    m_tradesModel = new TradesTableModel(this);
    m_equityModel = new TradesTableModel(this);
    
    QWidget* statsTab = new QWidget(parentWidget);
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
}

void StatsView::createPerformanceSection(QGridLayout* layout)
{
    // TODO: Implémenter la création de la section performance
    Q_UNUSED(layout);
}

void StatsView::createRiskSection(QGridLayout* layout)
{
    // TODO: Implémenter la création de la section risque
    Q_UNUSED(layout);
}

void StatsView::createGeneralSection(QGridLayout* layout)
{
    // TODO: Implémenter la création de la section générale
    Q_UNUSED(layout);
}

void StatsView::createTradesTable()
{
    // TODO: Implémenter la création de la table des trades
}

void StatsView::createEquityTable()
{
    // TODO: Implémenter la création de la table d'équité
}

MetricWidget* StatsView::createMetricWidget(const QString& key, const QString& label, 
                                           const QString& value, int row, int col, 
                                           QGridLayout* layout)
{
    // TODO: Implémenter la création du widget de métrique
    Q_UNUSED(key); Q_UNUSED(label); Q_UNUSED(value); Q_UNUSED(row); Q_UNUSED(col); Q_UNUSED(layout);
    return nullptr;
}

int StatsView::getLimitValue(QComboBox* combo)
{
    Q_UNUSED(combo);
    return 50;
}

void StatsView::refreshTradesTable()
{
    // TODO: Implémenter le rafraîchissement de la table des trades
}

void StatsView::refreshEquityTable()
{
    // TODO: Implémenter le rafraîchissement de la table d'équité
}

void StatsView::toggleEquityTable()
{
    // TODO: Implémenter le basculement de la table d'équité
}

void StatsView::update(void* data, void* stats)
{
    Q_UNUSED(data);
    qDebug() << "StatsView::update() appelé avec stats:" << stats;
    
    if (!stats) {
        clear();
        return;
    }
    
    // TODO: Implémenter la mise à jour avec les données
    populateMetrics(stats);
    populateTrades(stats);
    populateEquity(stats);
}

void StatsView::clear()
{
    if (m_statsPlaceholder) {
        m_statsPlaceholder->setVisible(true);
    }
    
    if (m_title) m_title->setVisible(false);
    if (m_performanceGroup) m_performanceGroup->setVisible(false);
    if (m_riskGroup) m_riskGroup->setVisible(false);
    if (m_generalGroup) m_generalGroup->setVisible(false);
    if (m_tradesGroup) m_tradesGroup->setVisible(false);
}

void StatsView::populateMetrics(void* stats)
{
    Q_UNUSED(stats);
    // TODO: Implémenter la population des métriques
}

void StatsView::populateTrades(void* stats)
{
    Q_UNUSED(stats);
    // TODO: Implémenter la population des trades
}

void StatsView::populateEquity(void* stats)
{
    Q_UNUSED(stats);
    // TODO: Implémenter la population de l'équité
}