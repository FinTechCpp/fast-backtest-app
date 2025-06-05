#include "views/chart_view.h"
#include "app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_cachedResults(nullptr)
    , m_dataExtracted(false)
    , m_currentResults(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_settingsTitle(nullptr)
    , m_chartWidget(nullptr)
    , m_leftPanel(nullptr)
    , m_app(nullptr)
{
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
    
    // Construire l'interface
    setupUI();

    // // Ajouter un RSI avec période 14
    // int rsi14Id = m_chartWidget->addRSI(14);

    // // Ajouter un autre RSI avec période 9
    // int rsi9Id = m_chartWidget->addRSI(9);

    // // Modifier la période du premier RSI
    // m_chartWidget->setRSIPeriod(rsi14Id, 21);

    // // Changer la couleur du second RSI
    // m_chartWidget->setRSIColor(rsi9Id, 0x0000FF); // Bleu

    // // Masquer temporairement un indicateur
    // m_chartWidget->setRSIVisible(rsi9Id, false);

    // // Modifier la hauteur du premier RSI
    // m_chartWidget->setRSIHeight(rsi14Id, 150);

    // // Supprimer le second RSI
    // m_chartWidget->removeRSI(rsi9Id);

    // // Connexion aux signaux
    // connect(m_chartWidget, &ChartWidget::rsiAdded, [](int id, int period) {
    //     qDebug() << "RSI ajouté:" << "id=" << id << "période=" << period;
    // });

    // connect(m_chartWidget, &ChartWidget::rsiChanged, [](int id, int period) {
    //     qDebug() << "RSI modifié:" << "id=" << id << "période=" << period;
    // });

    // connect(m_chartWidget, &ChartWidget::rsiRemoved, [](int id) {
    //     qDebug() << "RSI supprimé:" << "id=" << id;
    // });
}

ChartView::~ChartView()
{
}

void ChartView::setupUI()
{
    // Configurer le layout principal pour occuper tout l'espace
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Créer un layout horizontal pour les panneaux gauche et droit
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);
    
    // Créer le panneau gauche avec une largeur fixe
    m_leftPanel = new QWidget();
    m_leftPanel->setObjectName("leftPanel");
    m_leftPanel->setStyleSheet("#leftPanel { background-color: #BADDFF; }");
    m_leftPanel->setFixedWidth(155);
    
    // Ajouter un layout vertical au panneau gauche
    QVBoxLayout* leftPanelLayout = new QVBoxLayout(m_leftPanel);
    leftPanelLayout->setContentsMargins(8, 8, 8, 8);
    leftPanelLayout->setSpacing(10);
    
    // Ajouter un titre au panneau gauche
    m_settingsTitle = new QLabel("Settings");
    m_settingsTitle->setAlignment(Qt::AlignCenter);
    m_settingsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    leftPanelLayout->addWidget(m_settingsTitle);
    
    // Ajouter le sélecteur de type de graphique
    QLabel* chartTypeLabel = new QLabel("Chart Type");
    chartTypeLabel->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(chartTypeLabel);
    
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItem("CandleStick", "CandleStick");
    m_chartTypeCombo->addItem("Heikin Ashi", "HeikinAshi");
    m_chartTypeCombo->addItem("Line", "Close");
    m_chartTypeCombo->addItem("OHLC", "OHLC");
    leftPanelLayout->addWidget(m_chartTypeCombo);
    
    // Connecter le signal de changement à notre slot
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartView::onChartTypeChanged);
    
    // Ajouter un espace extensible en bas
    leftPanelLayout->addStretch();
    
    // Créer un séparateur vertical
    QFrame* separator = new QFrame();
    separator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    separator->setStyleSheet("color: #CCCCCC;"); // Couleur de la ligne
    
    // Créer le panneau droit qui contiendra le graphique
    m_rightPanel = new QWidget();
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Layout pour le panneau droit
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(m_rightPanel);
    rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->setSpacing(0);
    
    // Créer le placeholder initial
    m_chartPlaceholder = new QLabel("Exécutez le backtest pour afficher les graphiques");
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chartPlaceholder->setStyleSheet(
        "QLabel { "
        "background-color: #f5f5f5; "
        "border: 1px dashed #cccccc; "
        "color: #666666; "
        "font-size: 14px; "
        "}"
    );
    
    // Créer le widget de graphique
    m_chartWidget = new ChartWidget();
    m_chartWidget->setVisible(false); // Cacher initialement
    
    // Ajouter les widgets au layout du panneau droit
    rightPanelLayout->addWidget(m_chartPlaceholder);
    rightPanelLayout->addWidget(m_chartWidget);
    
    // Ajouter les composants au layout horizontal
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(separator);
    horizontalLayout->addWidget(m_rightPanel);
    
    // Configurer le widget pour s'étendre
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartView::updateData(BacktestResults* results)
{
    QTime start = QTime::currentTime();

    Q_UNUSED(results);
            
    // Récupérer les résultats depuis l'App
    BacktestResults* appResults = m_app ? m_app->getBacktestResults() : nullptr;
    
    // Mettre à jour les références locales
    m_currentResults = appResults;

    // TODO faire une vérification pour éviter les appels inutiles
    // Vérifier si les données ont déjà été extraites pour ces pointeurs
    // if (m_dataExtracted && m_cachedResults == appResults) {
    //     if (hasValidData()) {
    //         showChartWidget();
    //     }
    //     return;
    // }
    
    // Mettre en cache les nouveaux pointeurs
    m_cachedResults = appResults;
    
    if (!appResults || !appResults->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }
    
    try {
        // extractDataFromCpp(appResults);

        // Passer directement les objets du backtest au ChartWidget
        m_chartWidget->setBacktestData(results->data);
        m_chartWidget->setBacktestTrades(results->stats.trades);
        m_chartWidget->setEquityCurve(results->stats.equityCurve);

        m_dataExtracted = true;
        
        // Définir le type de graphique
        QString chartType = m_chartTypeCombo->currentData().toString();
        m_chartWidget->setChartType(m_chartWidget->stringToChartType(chartType));
        
        // Afficher le widget de graphique et masquer le placeholder
        showChartWidget();
        
        // Créer le graphique
        m_chartWidget->createChart();
        
        m_dataExtracted = true;
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++:" << e.what();
        showPlaceholder("Erreur lors de la création du graphique");
    } catch (...) {
        qCritical() << "Erreur inconnue dans ChartView::updateData()";
        showPlaceholder("Erreur inconnue");
    }

    int elapsed = start.msecsTo(QTime::currentTime());
}

void ChartView::clear()
{    
    m_currentResults = nullptr;
    m_cachedResults = nullptr;
    m_dataExtracted = false;
    
    // Nettoyer le widget de graphique
    if (m_chartWidget) {
        m_chartWidget->clearChart();
    }
    
    // Réafficher le placeholder
    showPlaceholder("Exécutez un backtest pour afficher les graphiques");
}

void ChartView::onChartTypeChanged(int index)
{
    if (!m_chartTypeCombo || !m_chartWidget) {
        return;
    }

    QVariant data = m_chartTypeCombo->itemData(index);
    if (data.isValid()) {
        QString chartType = data.toString();
        
        // Mettre à jour le type de graphique dans le widget
        m_chartWidget->setChartType(m_chartWidget->stringToChartType(chartType));
            
        // Si nous avons déjà des données valides, mettre à jour le graphique
        if (m_chartWidget->isVisible()) {
            m_chartWidget->updateChart();
        }
    }
}

void ChartView::showChartWidget()
{
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setVisible(false);
    }
    
    if (m_chartWidget) {
        m_chartWidget->setVisible(true);
    }
}

void ChartView::showPlaceholder(const QString& message)
{
    if (m_chartWidget) {
        m_chartWidget->setVisible(false);
    }
    
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
}
