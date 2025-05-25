#include "result_manager.h"
#include "stats_view.h"
#include "chart_view.h"
#include "histogram_view.h"
#include <QDebug>
#include <QTime>

ResultManager::ResultManager(QObject* parent) :
    BaseView(parent), 
    m_resultsWidget(nullptr),
    m_resultsLayout(nullptr),
    m_tabWidget(nullptr),
    m_statsView(nullptr),
    m_chartView(nullptr),
    m_histogramView(nullptr)
{
    qDebug() << "ResultManager créé avec parent:" << parent;
}

ResultManager::~ResultManager()
{
    // Qt gère automatiquement la destruction des widgets enfants
}

QWidget* ResultManager::create(QWidget* parentWidget)
{
    QTime start = QTime::currentTime();
    
    // Créer le widget principal des résultats
    m_resultsWidget = new QWidget(parentWidget);
    m_resultsLayout = new QVBoxLayout(m_resultsWidget);
    
    // Créer les onglets pour séparer les différentes vues
    m_tabWidget = new QTabWidget();
    
    // Passer 'this' comme parent QObject pour les vues
    m_statsView = new StatsView(this);
    m_histogramView = new HistogramView(this);
    m_chartView = new ChartView(this);
    
    // Vérifier que les vues ont été créées correctement
    if (!m_statsView || !m_histogramView || !m_chartView) {
        qCritical() << "Erreur lors de la création des vues";
        return nullptr;
    }
    
    // Créer les widgets des vues en passant le widget parent approprié
    QWidget* statsWidget = m_statsView->create(m_resultsWidget);
    QWidget* histogramWidget = m_histogramView->create(m_resultsWidget);
    QWidget* chartWidget = m_chartView->create(m_resultsWidget);
    
    // Vérifier que les widgets ont été créés
    if (!statsWidget || !histogramWidget || !chartWidget) {
        qCritical() << "Erreur lors de la création des widgets de vue";
        return nullptr;
    }
    
    m_tabWidget->addTab(statsWidget, "📊 Statistiques");
    m_tabWidget->addTab(histogramWidget, "📊 Histogramme PnL");
    m_tabWidget->addTab(chartWidget, "📈 Graphiques");
    
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
    updateAll(data, stats);
}

void ResultManager::clear()
{
    for (auto it = m_views.begin(); it != m_views.end(); ++it) {
        it.value()->clear();
    }
}

void ResultManager::updateAll(void* data, void* stats)
{
    QTime start = QTime::currentTime();
    
    qDebug() << "updateAll appelé avec des pointeurs opaques data et stats";
    
    try {
        // Définir l'ordre d'exécution pour éviter les conflits
        QStringList updateOrder = {"stats", "histogram", "chart"};
        
        for (const QString& viewName : updateOrder) {
            if (m_views.contains(viewName)) {
                qDebug() << "Mise à jour de la vue:" << viewName;
                m_views[viewName]->update(data, stats);
            }
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