#include "components/result_manager.h"
#include "views/stats_view.h"
#include "views/chart_view.h"
#include "views/histogram_view.h"
#include <QDebug>
#include <QTime>
#include <QResizeEvent>
#include <QTimer>



ResultManager::ResultManager(QWidget* parent) : QWidget(parent) 
{
    qDebug() << "ResultManager créé avec parent:" << parent;
    
    // Initialiser tous les attributs à nullptr d'abord
    m_statsView = nullptr;
    m_chartView = nullptr;
    m_histogramView = nullptr;
    
    // Construire l'interface dans le constructeur
    setupUI();
    setupViews();
    setupConnections();
    
    qDebug() << "ResultManager initialisé avec succès";
}

void ResultManager::updateAllViews(BacktestResults* results)
{
    QTime start = QTime::currentTime();
    qDebug() << "updateAllViews appelé avec BacktestResults:" << results;

    try {
        for (auto it = m_views.begin(); it != m_views.end(); ++it) {
            BaseView* view = it.value();
            if (view) {
                QTime viewStart = QTime::currentTime();
                view->updateData(results);  // Mise à jour pour utiliser la nouvelle signature
                int viewElapsed = viewStart.msecsTo(QTime::currentTime());
                std::cout << "Vue '" << it.key().toStdString() << "' mise à jour en " << viewElapsed << " ms" << std::endl;
            }
        }

        qInfo() << "Toutes les vues mises à jour avec succès";
    }
    catch (const std::exception& e) {
        qCritical() << "Erreur lors de la mise à jour des vues:" << e.what();
    }

    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ResultManager::updateAllViews() took" << elapsed << "ms";
}

void ResultManager::clearAllViews()
{
    for (auto it = m_views.begin(); it != m_views.end(); ++it) {
        it.value()->clear();
    }
}

void ResultManager::setCurrentTab(int index)
{
    if (m_tabWidget && index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

void ResultManager::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);  // Appeler la méthode parent
    
    qDebug() << "ResultManager redimensionné:" << event->size();
    
    // Utiliser le timer pour éviter trop de redimensionnements
    if (m_resizeTimer) {
        m_resizeTimer->stop();
        m_resizeTimer->start(150);
    }
}

void ResultManager::onTabResized()
{
    qDebug() << "Traitement du redimensionnement des onglets";
    
    if (!m_tabWidget || !m_chartView) {
        return;
    }
    
    // Obtenir la taille disponible
    QSize availableSize = m_tabWidget->size();
    int tabWidth = availableSize.width();
    
    if (tabWidth > 100) {
        // Calculer la nouvelle largeur pour le graphique
        int newChartWidth = std::max(800, tabWidth - 40);
        
        qDebug() << "Redimensionnement du graphique à:" << newChartWidth;
        
        // Redimensionner le graphique via la vue
        // m_chartView->resizeChart(newChartWidth);
    }
}

void ResultManager::onTabChanged(int index)
{
    qDebug() << "Onglet changé vers l'index:" << index;
    
    // Optionnel : effectuer des actions spécifiques lors du changement d'onglet
    if (index >= 0 && index < m_tabWidget->count()) {
        // Par exemple, rafraîchir la vue active
        QString currentViewName;
        if (index == 0) currentViewName = "stats";
        else if (index == 1) currentViewName = "histogram";
        else if (index == 2) currentViewName = "chart";
        
        qDebug() << "Vue active:" << currentViewName;
    }
}

void ResultManager::setupUI()
{
    // Créer le layout principal pour ce widget
    m_mainLayout = new QVBoxLayout(this);  // 'this' devient le widget parent
    
    // Créer le TabWidget
    m_tabWidget = new QTabWidget(this);
    
    // Ajouter le TabWidget au layout
    m_mainLayout->addWidget(m_tabWidget);
    
    // Configurer les marges si nécessaire
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    qDebug() << "Interface UI créée";
}

void ResultManager::setupViews()
{
    // Créer les vues (elles sont des widgets, donc directement utilisables)
    m_statsView = new StatsView(this);
    m_histogramView = new HistogramView(this);
    m_chartView = new ChartView(this);
    
    // Vérifier que les vues ont été créées
    if (!m_statsView || !m_histogramView || !m_chartView) {
        qCritical() << "Erreur lors de la création des vues";
        return;
    }
    
    // Ajouter les vues comme onglets (les vues SONT des widgets)
    m_tabWidget->addTab(m_statsView, "📊 Statistiques");
    m_tabWidget->addTab(m_histogramView, "📊 Histogramme PnL");
    m_tabWidget->addTab(m_chartView, "📈 Graphiques");
    
    // Ajouter au map pour faciliter l'accès
    m_views["stats"] = m_statsView;
    m_views["histogram"] = m_histogramView;
    m_views["chart"] = m_chartView;
}

void ResultManager::setupConnections()
{
    // Connecter le changement d'onglet
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &ResultManager::onTabChanged);
    
    // Créer un timer pour surveiller les redimensionnements
    QTimer* resizeTimer = new QTimer(this);
    resizeTimer->setSingleShot(true);
    connect(resizeTimer, &QTimer::timeout, this, &ResultManager::onTabResized);
    
    // Stocker le timer comme membre si nécessaire
    m_resizeTimer = resizeTimer;
    
    qDebug() << "Connexions établies";
}
