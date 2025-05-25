#include "result_manager.h"
#include "stats_view.h"
#include "chart_view.h"
#include "histogram_view.h"
#include <QDebug>
#include <QTime>
#include <QResizeEvent>
#include <QTimer>

void ResultManager::resizeEvent(QResizeEvent* event)
{
    qDebug() << "🔧 🔧 🔧 RESIZE EVENT DETECTED! 🔧 🔧 🔧";
    qDebug() << "🔧 Old size:" << event->oldSize();
    qDebug() << "🔧 New size:" << event->size();
    
    BaseView::resizeEvent(event);
    
    // Utiliser un timer pour éviter trop de redimensionnements
    static QTimer* resizeTimer = nullptr;
    if (!resizeTimer) {
        resizeTimer = new QTimer(this);
        resizeTimer->setSingleShot(true);
        connect(resizeTimer, &QTimer::timeout, this, &ResultManager::onTabResized);
        qDebug() << "🔧 Timer créé et connecté";
    }
    
    resizeTimer->stop();
    resizeTimer->start(150); // Attendre 150ms après la fin du redimensionnement
    qDebug() << "🔧 Timer démarré (150ms)";
}

void ResultManager::onTabResized()
{
    qDebug() << "🔧 === RESIZE EVENT TRIGGERED ===";
    
    if (!m_tabWidget) {
        qDebug() << "🔧 ERROR: m_tabWidget is null";
        return;
    }
    
    if (!m_chartView) {
        qDebug() << "🔧 ERROR: m_chartView is null";
        return;
    }
    
    if (!m_chartWidget) {
        qDebug() << "🔧 ERROR: m_chartWidget is null";
        return;
    }
    
    // DEBUG DÉTAILLÉ DES TAILLES
    qDebug() << "🔧 === TAILLES DÉTECTÉES ===";
    
    // Taille du TabWidget
    int tabWidth = m_tabWidget->width();
    int tabHeight = m_tabWidget->height();
    qDebug() << "🔧 TabWidget:" << tabWidth << "x" << tabHeight;
    
    // Taille du ResultsWidget
    if (m_resultsWidget) {
        int resultsWidth = m_resultsWidget->width();
        int resultsHeight = m_resultsWidget->height();
        qDebug() << "🔧 ResultsWidget:" << resultsWidth << "x" << resultsHeight;
    }
    
    // Taille du ChartWidget
    int chartWidth = m_chartWidget->width();
    int chartHeight = m_chartWidget->height();
    qDebug() << "🔧 ChartWidget:" << chartWidth << "x" << chartHeight;
    
    // Onglet actuel
    int currentIndex = m_tabWidget->currentIndex();
    qDebug() << "🔧 Onglet actuel:" << currentIndex;
    
    if (currentIndex >= 0) {
        QWidget* currentTab = m_tabWidget->widget(currentIndex);
        if (currentTab) {
            qDebug() << "🔧 Onglet actuel size:" << currentTab->width() << "x" << currentTab->height();
        }
    }
    
    qDebug() << "🔧 === FIN TAILLES ===";
    
    // CALCUL ET APPLICATION DU REDIMENSIONNEMENT
    if (tabWidth > 100) {
        // Calculer la largeur du graphique (largeur onglet - marges)
        int newChartWidth = std::max(800, tabWidth - 40);
        
        qDebug() << "🔧 REDIMENSIONNEMENT: tabWidth:" << tabWidth << "-> newChartWidth:" << newChartWidth;
        
        // Redimensionner le graphique
        m_chartView->resizeChart(newChartWidth);
        
        qDebug() << "🔧 resizeChart() appelé avec:" << newChartWidth;
        
    } else {
        qDebug() << "🔧 REDIMENSIONNEMENT IGNORÉ: tabWidth trop petit:" << tabWidth;
    }
    
    qDebug() << "🔧 === FIN RESIZE EVENT ===";
}

ResultManager::ResultManager(QWidget* parent) :
    BaseView(parent), 
    m_resultsWidget(nullptr),
    m_resultsLayout(nullptr),
    m_tabWidget(nullptr),
    m_statsView(nullptr),
    m_chartView(nullptr),
    m_histogramView(nullptr),
    m_chartWidget(nullptr),     // AJOUT
    m_statsWidget(nullptr),     // AJOUT
    m_histogramWidget(nullptr)  // AJOUT
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
    m_statsView = new StatsView(m_resultsWidget);
    m_histogramView = new HistogramView(m_resultsWidget);
    m_chartView = new ChartView(m_resultsWidget);
    
    // Vérifier que les vues ont été créées correctement
    if (!m_statsView || !m_histogramView || !m_chartView) {
        qCritical() << "Erreur lors de la création des vues";
        return nullptr;
    }
    
    // CORRECTION : Créer les widgets et stocker les références
    m_statsWidget = m_statsView->create(m_resultsWidget);
    m_histogramWidget = m_histogramView->create(m_resultsWidget);
    m_chartWidget = m_chartView->create(m_resultsWidget);  // STOCKER LA RÉFÉRENCE
    
    // Vérifier que les widgets ont été créés
    if (!m_statsWidget || !m_histogramWidget || !m_chartWidget) {
        qCritical() << "Erreur lors de la création des widgets de vue";
        return nullptr;
    }
    
    m_tabWidget->addTab(m_statsWidget, "📊 Statistiques");
    m_tabWidget->addTab(m_histogramWidget, "📊 Histogramme PnL");
    m_tabWidget->addTab(m_chartWidget, "📈 Graphiques");
    
    // Ajouter au map pour faciliter l'accès
    m_views["stats"] = m_statsView;
    m_views["histogram"] = m_histogramView;
    m_views["chart"] = m_chartView;
    
    // Ajouter le widget d'onglets au layout principal
    m_resultsLayout->addWidget(m_tabWidget);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ResultManager::create() took" << elapsed << "ms";

    // AJOUT : Connecter un timer récurrent pour surveiller les changements de taille
    QTimer* sizeMonitor = new QTimer(this);
    connect(sizeMonitor, &QTimer::timeout, [this]() {
        static int lastWidth = 0;
        static int lastHeight = 0;
        
        if (m_tabWidget) {
            int currentWidth = m_tabWidget->width();
            int currentHeight = m_tabWidget->height();
            
            if (currentWidth != lastWidth || currentHeight != lastHeight) {
                qDebug() << "🔧 🔧 🔧 TAILLE CHANGÉE DÉTECTÉE VIA TIMER! 🔧 🔧 🔧";
                qDebug() << "🔧 Ancienne taille:" << lastWidth << "x" << lastHeight;
                qDebug() << "🔧 Nouvelle taille:" << currentWidth << "x" << currentHeight;
                
                lastWidth = currentWidth;
                lastHeight = currentHeight;
                
                // Déclencher le redimensionnement
                onTabResized();
            }
        }
    });
    sizeMonitor->start(250); // Vérifier toutes les 250ms
    qDebug() << "🔧 Timer de surveillance de taille démarré";
        
    
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