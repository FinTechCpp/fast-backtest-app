#include "components/Managers/resultManager.h"
#include "ui/views/statsView.h"
#include "ui/views/tradesView.h"
#include "ui/views/chartView.h"
#include <QDebug>
#include <QTime>
#include <QResizeEvent>
#include <QTimer>



ResultManager::ResultManager(QWidget* parent) : QWidget(parent) 
{
    qDebug() << "ResultManager créé avec parent:" << parent;
    
    // Initialiser tous les attributs à nullptr d'abord
    m_statsView = nullptr;
    m_tradesView = nullptr;
    m_chartView = nullptr;
    
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
    
    // Définir la couleur de fond du TabWidget pour qu'elle corresponde à la palette Window
    QPalette tabPalette = m_tabWidget->palette();
    QColor windowColor = tabPalette.color(QPalette::Window);
    m_tabWidget->setStyleSheet(QString("QTabWidget::pane { background-color: %1; border: none; }").arg(windowColor.name()));
    
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
    m_tradesView = new TradesView(this);
    m_chartView = new ChartView(this);
    
    // Vérifier que les vues ont été créées
    if (!m_statsView || !m_tradesView || !m_chartView) {
        qCritical() << "Erreur lors de la création des vues";
        return;
    }
    
    // Ajouter les vues comme onglets (les vues SONT des widgets)
    m_tabWidget->addTab(m_statsView, "📊 Statistiques");
    m_tabWidget->addTab(m_tradesView, "📋 Trades");
    m_tabWidget->addTab(m_chartView, "📈 Graphiques");
    
    // Ajouter au map pour faciliter l'accès
    m_views["stats"] = m_statsView;
    m_views["trades"] = m_tradesView;
    m_views["chart"] = m_chartView;
}

void ResultManager::setupConnections()
{
    // Connecter le changement d'onglet
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &ResultManager::onTabChanged);
    
    // Connecter le signal de clic sur trade depuis TradesView
    if (m_tradesView) {
        connect(m_tradesView, &TradesView::tradeClicked,
                this, &ResultManager::onTradeClicked);
    }
    
    // Créer un timer pour surveiller les redimensionnements
    QTimer* resizeTimer = new QTimer(this);
    resizeTimer->setSingleShot(true);
    connect(resizeTimer, &QTimer::timeout, this, &ResultManager::onTabResized);
    
    // Stocker le timer comme membre si nécessaire
    m_resizeTimer = resizeTimer;
    
    qDebug() << "Connexions établies";
}

void ResultManager::onTradeClicked(const be::TradeData& trade)
{
    qDebug() << "Trade cliqué dans ResultManager - Entrée:" << trade.entryDate.toString().c_str() 
             << "Sortie:" << trade.exitDate.toString().c_str();
    
    // Changer vers l'onglet Chart (index 2: Stats=0, Trades=1, Chart=2)
    int chartTabIndex = -1;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == m_chartView) {
            chartTabIndex = i;
            break;
        }
    }
    
    if (chartTabIndex >= 0) {
        m_tabWidget->setCurrentIndex(chartTabIndex);
        
        // Demander à ChartView de zoomer sur ce trade
        if (m_chartView) {
            m_chartView->zoomToTrade(trade);
        }
    }
}