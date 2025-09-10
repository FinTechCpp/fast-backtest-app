#include "ui/views/chartView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_dataExtracted(false)
    , m_chartPlaceholder(nullptr)
    , m_leftPanel(nullptr)
    , m_rightPanel(nullptr)
    , m_chartWidget(nullptr)
    , m_app(nullptr)
{
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }

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
    QHBoxLayout* rightPanelLayout = new QHBoxLayout(m_rightPanel);
    rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->setSpacing(0);

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

    // Widget de graphique
    m_chartWidget = new ChartWidget();
    // m_chartWidget2 = new ChartWidget(); // Pour comparaison verticale
    m_chartWidget->setVisible(false);
    // m_chartWidget2->setVisible(false);

    // // Configurer la synchronisation entre les deux widgets
    // m_chartWidget->setSyncPartner(m_chartWidget2);
    // m_chartWidget2->setSyncPartner(m_chartWidget);

    // // Connecter les signaux pour la synchronisation
    // connect(m_chartWidget, &ChartWidget::aggregationChanged, 
    //         m_chartWidget2, &ChartWidget::setCurrentAggregation);
    // connect(m_chartWidget2, &ChartWidget::aggregationChanged, 
    //         m_chartWidget, &ChartWidget::setCurrentAggregation);
    // connect(m_chartWidget, &ChartWidget::viewportChanged, 
    //         m_chartWidget2, &ChartWidget::setViewport);
    // connect(m_chartWidget2, &ChartWidget::viewportChanged, 
    //         m_chartWidget, &ChartWidget::setViewport);

    // Associer le widget de graphique au panneau de contrôle
    m_leftPanel->setChartWidget(m_chartWidget);

    // Connecter les signaux du panneau de contrôle
    connect(m_leftPanel, &ChartControlPanel::chartTypeChanged, [this](const QString& chartType) {
        m_chartWidget->setChartType(chart::stringToChartType(chartType.toStdString()));
        // m_chartWidget2->setChartType(chart::stringToChartType(chartType.toStdString()));
    });
    
    connect(m_leftPanel, &ChartControlPanel::rulerToolToggled, [this](bool checked) {
        m_chartWidget->setRulerToolEnabled(checked);
        // m_chartWidget2->setRulerToolEnabled(checked);
    });
    
    connect(m_leftPanel, &ChartControlPanel::aggregationValueChanged, [this](int value) {
        m_chartWidget->setMaxDisplayPoints(value);
        // m_chartWidget2->setMaxDisplayPoints(value);
    });

    // Ajouter les widgets au layout du panneau droit
    rightPanelLayout->addWidget(m_chartPlaceholder);
    rightPanelLayout->addWidget(m_chartWidget);
    // rightPanelLayout->addWidget(m_chartWidget2);

    // Ajouter les composants au layout horizontal
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(verticalSeparator);
    horizontalLayout->addWidget(m_rightPanel);

    // Configurer le widget pour s'étirer
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (m_app) 
        connect(m_app, &App::windowResizeStarted, m_chartWidget, [this]() {
            m_chartWidget->setResizing(true);
        });
}

void ChartView::updateData(BacktestResults* results) {
    m_currentResults = results;

    QTime start = QTime::currentTime();

    if (!results || !results->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }

    m_chartWidget->setBacktestResults(results);
    // m_chartWidget2->setBacktestResults(results); // Pour comparaison verticale
    m_dataExtracted = true;
    
    // Afficher le widget de graphique
    showChartWidget();

    // Configurer les indicateurs de stratégie
    m_leftPanel->configureStrategyIndicators(results->indicators);
    
    // Actualiser la liste des indicateurs
    m_leftPanel->refreshIndicatorsList();
    
    m_dataExtracted = true;

    int elapsed = start.msecsTo(QTime::currentTime());
}

void ChartView::showChartWidget() {
    if (m_chartPlaceholder) m_chartPlaceholder->setVisible(false);
    if (m_chartWidget) m_chartWidget->setVisible(true);
    if (m_chartWidget2) m_chartWidget2->setVisible(true);
}

void ChartView::showPlaceholder(const QString& message) {
    if (m_chartWidget) m_chartWidget->setVisible(false);
    if (m_chartWidget2) m_chartWidget2->setVisible(false);

    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
}

void ChartView::clear() {
    m_dataExtracted = false;
    m_leftPanel->refreshIndicatorsList();
    showPlaceholder("Exécutez un backtest pour afficher les graphiques");
}

void ChartView::zoomToTrade(const be::TradeData& trade) {
    qDebug() << "ChartView::zoomToTrade appelé pour trade avec entrée:" << trade.entryDate.toString().c_str();
    
    if (!m_chartWidget) {
        qDebug() << "Impossible de zoomer: ChartWidget non initialisé";
        return;
    }
    
    // S'assurer que le widget de graphique est visible
    showChartWidget();
    
    // Déléguer le zoom au ChartWidget
    m_chartWidget->zoomToTrade(trade);
    // Je ne sais pas comment cela peut reagir avec deux graphiques mais je pense qu'il faut zoomer par rapport au graph1 et le deux suivra
    // m_chartWidget2->zoomToTrade(trade); // Pour comparaison verticale 
}