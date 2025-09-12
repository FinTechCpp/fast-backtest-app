#include "ui/views/chartView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_comparisonMode(false)
    , m_chartPlaceholder(nullptr)
    , m_leftPanel(nullptr)
    , m_rightPanel(nullptr)
    , m_chartWidget(nullptr)
    , m_chartWidget2(nullptr)
{
    setupUI();
}

ChartView::~ChartView()
{
}

void ChartView::setupUI()
{
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Layout horizontal pour les panneaux gauche et droite
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);
    
    // Création du panneau de contrôle gauche
    m_leftPanel = new ChartControlPanel(this);

    // Séparateur vertical
    QFrame* verticalSeparator = new QFrame();
    verticalSeparator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    verticalSeparator->setStyleSheet("color: #CCCCCC;");

    // Panneau droit qui contiendra le graphique
    m_rightPanel = new QWidget();
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Layout pour le panneau droit
    m_rightPanelLayout = new QStackedLayout(m_rightPanel);
    m_rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    m_rightPanelLayout->setSpacing(0);

    // Placeholder initial
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
    m_chartContainer = new QWidget();
    QHBoxLayout* chartLayout = new QHBoxLayout(m_chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);

    // Widget de graphique
    m_chartWidget = new ChartWidget();
    m_chartWidget2 = new ChartWidget(); // Pour comparaison verticale
    m_chartWidget->setVisible(true);
    m_chartWidget2->setVisible(false);

    // Associer le widget de graphique au panneau de contrôle
    m_leftPanel->setChartWidget(m_chartWidget);

    // Connecter les signaux du panneau de contrôle
    connect(m_leftPanel, &ChartControlPanel::chartTypeChanged, [this](const QString& chartType) {
        m_chartWidget->setChartType(chart::stringToChartType(chartType.toStdString()));
        if (m_comparisonMode && m_chartWidget2) {
            m_chartWidget2->setChartType(chart::stringToChartType(chartType.toStdString()));
        }
    });
    
    connect(m_leftPanel, &ChartControlPanel::rulerToolToggled, [this](bool checked) {
        m_chartWidget->setRulerToolEnabled(checked);
        if (m_comparisonMode && m_chartWidget2) {
            m_chartWidget2->setRulerToolEnabled(checked);
        }
    });

    connect(m_leftPanel, &ChartControlPanel::transferDataForComparison, this, [this]() {
        if (!m_chartWidget || !m_chartWidget2 || !m_currentResults) return;

        // Connecter les signaux pour la synchronisation
        connect(m_chartWidget, &ChartWidget::aggregationChanged, 
                m_chartWidget2, &ChartWidget::setCurrentAggregation);
        connect(m_chartWidget2, &ChartWidget::aggregationChanged, 
                m_chartWidget, &ChartWidget::setCurrentAggregation);
        connect(m_chartWidget, &ChartWidget::viewportChanged, 
                m_chartWidget2, &ChartWidget::setViewport);
        connect(m_chartWidget2, &ChartWidget::viewportChanged, 
                m_chartWidget, &ChartWidget::setViewport);
        connect(m_chartWidget, &ChartWidget::trackFinanceUpdated,
                m_chartWidget2, &ChartWidget::forceUpdateTrackFinance);
        connect(m_chartWidget2, &ChartWidget::trackFinanceUpdated,
                m_chartWidget, &ChartWidget::forceUpdateTrackFinance);

        m_chartWidget->setSyncPartner(m_chartWidget2);
        m_chartWidget2->setSyncPartner(m_chartWidget);

        m_chartWidget2->setVisible(true);

        m_chartWidget2->setBacktestResults(m_currentResults);

        m_comparisonMode = true;
        m_leftPanel->setComparisonMode(true);
    });
    
    connect(m_leftPanel, &ChartControlPanel::exitComparisonMode, this, [this]() {
        if (!m_chartWidget || !m_chartWidget2)
            return;

        // Déconnecter les signaux de synchronisation
        disconnect(m_chartWidget, &ChartWidget::aggregationChanged, 
                    m_chartWidget2, &ChartWidget::setCurrentAggregation);
        disconnect(m_chartWidget2, &ChartWidget::aggregationChanged, 
                    m_chartWidget, &ChartWidget::setCurrentAggregation);
        disconnect(m_chartWidget, &ChartWidget::viewportChanged, 
                    m_chartWidget2, &ChartWidget::setViewport);
        disconnect(m_chartWidget2, &ChartWidget::viewportChanged, 
                    m_chartWidget, &ChartWidget::setViewport);
        disconnect(m_chartWidget, &ChartWidget::trackFinanceUpdated,
                    m_chartWidget2, &ChartWidget::forceUpdateTrackFinance);
        disconnect(m_chartWidget2, &ChartWidget::trackFinanceUpdated,
                    m_chartWidget, &ChartWidget::forceUpdateTrackFinance);
        
        m_chartWidget->setSyncPartner(nullptr);
        m_chartWidget2->setSyncPartner(nullptr);

        m_chartWidget2->setVisible(false);
        
        m_comparisonMode = false;
        m_leftPanel->setComparisonMode(false);
    });
    
    connect(m_leftPanel, &ChartControlPanel::aggregationValueChanged, [this](int value) {
        m_chartWidget->setMaxDisplayPoints(value);
        if (m_comparisonMode && m_chartWidget2) {
            m_chartWidget2->setMaxDisplayPoints(value);
        }
    });

    chartLayout->addWidget(m_chartWidget, /*stretch=*/1);
    chartLayout->addWidget(m_chartWidget2, /*stretch=*/1);

    // Ajouter les widgets au layout du panneau droit
    m_rightPanelLayout->addWidget(m_chartPlaceholder);  
    m_rightPanelLayout->addWidget(m_chartContainer);

    // Ajouter les composants au layout horizontal
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(verticalSeparator);
    horizontalLayout->addWidget(m_rightPanel);

    // Configurer le widget pour s'étirer
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartView::updateData(BacktestResults* results) {
    m_currentResults = results;

    if (!results || !results->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }

    m_chartWidget->setBacktestResults(results);
    
    // Afficher le widget de graphique
    showChartWidget();

    // Configurer les indicateurs de stratégie
    m_leftPanel->configureStrategyIndicators(results->indicators);
    
    // Actualiser la liste des indicateurs
    m_leftPanel->refreshIndicatorsList();
}

void ChartView::showChartWidget() {
    m_rightPanelLayout->setCurrentWidget(m_chartContainer);
}

void ChartView::showPlaceholder(const QString& message) {
    m_chartPlaceholder->setText(message);
    m_rightPanelLayout->setCurrentWidget(m_chartPlaceholder);
}

void ChartView::clear() {
    m_leftPanel->refreshIndicatorsList();
    showPlaceholder("Exécutez un backtest pour afficher les graphiques");
}

void ChartView::zoomToTrade(const be::TradeData& trade) {
    qDebug() << "ChartView::zoomToTrade appelé pour trade avec entrée:" << trade.entryDate.toString().c_str();
    
    // S'assurer que le widget de graphique est visible
    showChartWidget();
    
    // Déléguer le zoom au ChartWidget
    m_chartWidget->zoomToTrade(trade);
}
