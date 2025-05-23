#include "result_manager.h"
#include "stats_view.h"
#include "chart_view.h"
#include "histogram_view.h"
#include <QDebug>
#include <QTime>

ResultManager::ResultManager(QObject* parent) :
    QObject(parent),
    BaseView(qobject_cast<QWidget*>(parent)),
    m_resultsWidget(nullptr),
    m_resultsLayout(nullptr),
    m_tabWidget(nullptr),
    m_statsView(nullptr),
    m_chartView(nullptr),        // CORRECTION: Initialiser dans le bon ordre
    m_histogramView(nullptr)     // CORRECTION: Après m_chartView
{
    // Les vues seront créées dans create()
}

ResultManager::~ResultManager()
{
    // Qt gère automatiquement la destruction des widgets enfants
}

QWidget* ResultManager::create()
{
    QTime start = QTime::currentTime();
    
    // Créer le widget principal des résultats
    m_resultsWidget = new QWidget();
    m_resultsLayout = new QVBoxLayout(m_resultsWidget);
    
    // Créer les onglets pour séparer les différentes vues
    m_tabWidget = new QTabWidget();
    
    // Créer les vues avec le bon parent
    m_statsView = new StatsView(m_resultsWidget);
    m_histogramView = new HistogramView(m_resultsWidget);
    m_chartView = new ChartView(m_resultsWidget);
    
    // Ajouter les vues aux onglets
    m_tabWidget->addTab(m_statsView->create(), "📊 Statistiques");
    m_tabWidget->addTab(m_histogramView->create(), "📊 Histogramme PnL");
    m_tabWidget->addTab(m_chartView->create(), "📈 Graphiques");
    
    // Ajouter au map pour faciliter l'accès
    m_views["stats"] = m_statsView;
    m_views["histogram"] = m_histogramView;
    m_views["chart"] = m_chartView;
    
    // Ajouter le widget d'onglets au layout principal
    m_resultsLayout->addWidget(m_tabWidget);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ResultManager::create() took" << elapsed << "ms";
    
    return m_resultsWidget;
}

void ResultManager::update(void* data, void* stats)
{
    // Déléguer à updateAll pour compatibilité
    updateAll(data, stats);
}

void ResultManager::clear()
{
    // Effacer toutes les vues
    for (auto it = m_views.begin(); it != m_views.end(); ++it) {
        it.value()->clear();
    }
}

void ResultManager::updateAll(void* data, void* stats)
{
    QTime start = QTime::currentTime();
    
    qDebug() << "updateAll appelé avec des pointeurs opaques data et stats";
    
    try {
        // Mettre à jour chaque vue
        for (auto it = m_views.begin(); it != m_views.end(); ++it) {
            qDebug() << "Mise à jour de la vue:" << it.key();
            it.value()->update(data, stats);
        }
        
        qInfo() << "Toutes les vues ont été mises à jour avec succès";
    }
    catch (const std::exception& e) {
        qCritical() << "Erreur lors de la mise à jour des vues:" << e.what();
    }
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ResultManager::updateAll() took" << elapsed << "ms";
}

void ResultManager::setCurrentTab(int index)
{
    if (m_tabWidget && index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

QTabWidget* ResultManager::getTabWidget() const
{
    return m_tabWidget;
}