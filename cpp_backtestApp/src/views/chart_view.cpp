#include "views/chart_view.h"
#include "app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>
#include "views/chart_view.h"

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
    m_leftPanel->setFixedWidth(180); // Augmenter la largeur pour les contrôles d'indicateurs
    
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
    
    // Section des indicateurs techniques
    QLabel* indicatorsLabel = new QLabel("Technical Indicators");
    indicatorsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(indicatorsLabel);
    
    // Configuration des contrôles d'indicateurs
    setupIndicatorControls();
    leftPanelLayout->addWidget(m_indicatorsGroup);
    
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
    
    // Connecter les signaux du ChartWidget
    connect(m_chartWidget, &ChartWidget::rsiAdded, this, &ChartView::onRSIAdded);
    connect(m_chartWidget, &ChartWidget::rsiChanged, this, &ChartView::onRSIChanged);
    connect(m_chartWidget, &ChartWidget::rsiRemoved, this, &ChartView::onRSIRemoved);

    // Connecter les signaux EMA
    connect(m_chartWidget, &ChartWidget::emaAdded, this, &ChartView::onEMAAdded);
    connect(m_chartWidget, &ChartWidget::emaChanged, this, &ChartView::onEMAChanged);
    connect(m_chartWidget, &ChartWidget::emaRemoved, this, &ChartView::onEMARemoved);
    
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

void ChartView::setupIndicatorControls()
{
    // Créer le groupe pour les contrôles d'indicateurs
    m_indicatorsGroup = new QGroupBox("Active Indicators");
    QVBoxLayout* groupLayout = new QVBoxLayout(m_indicatorsGroup);
    
    // Créer le layout pour la liste des indicateurs
    m_indicatorsLayout = new QVBoxLayout();
    groupLayout->addLayout(m_indicatorsLayout);
    
    // Ajouter un bouton d'ajout et une combobox pour le type d'indicateur
    QHBoxLayout* addIndicatorLayout = new QHBoxLayout();
    
    m_addIndicatorButton = new QPushButton("Add");
    m_addIndicatorButton->setFixedWidth(50);
    
    m_indicatorTypeCombo = new QComboBox();
    m_indicatorTypeCombo->addItem("RSI", "RSI");
    m_indicatorTypeCombo->addItem("EMA", "EMA");
    // Ajouter d'autres types d'indicateurs ici au besoin
    
    addIndicatorLayout->addWidget(m_indicatorTypeCombo);
    addIndicatorLayout->addWidget(m_addIndicatorButton);
    
    groupLayout->addLayout(addIndicatorLayout);
    
    // Connecter les signaux
    connect(m_addIndicatorButton, &QPushButton::clicked, this, &ChartView::onAddIndicatorClicked);
    connect(m_indicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ChartView::onIndicatorTypeSelected);
}

void ChartView::onIndicatorTypeSelected(int index)
{
    // Cette méthode peut être utilisée pour ajouter un comportement spécifique
    // lorsque le type d'indicateur est modifié dans la liste déroulante
    Q_UNUSED(index);
}

void ChartView::onAddIndicatorClicked()
{
    if (!m_chartWidget->hasValidData()) {
        qDebug() << "Pas de données valides pour ajouter un indicateur";
        return;
    }
    
    QString indicatorType = m_indicatorTypeCombo->currentData().toString();
    
    if (indicatorType == "RSI") {
        // Ajouter un RSI avec la période par défaut (14)
        m_chartWidget->addRSI(14);
    }
    else if (indicatorType == "EMA") {
        // Pour EMA, ouvrir le dialogue de configuration directement
        onEditEMA();
    }
    // Ajouter d'autres types d'indicateurs ici
}

void ChartView::onRSIAdded(int id, int period)
{
    qDebug() << "RSI ajouté:" << "id=" << id << "période=" << period;
    
    // Créer les widgets pour ce RSI
    QString name = QString("RSI (%1)").arg(period);
    createIndicatorWidgets(id, name);
    
    // Mettre à jour le graphique
    if (m_chartWidget->hasValidData()) {
        m_chartWidget->updateChart();
    }
}

void ChartView::onRSIChanged(int id, int period)
{
    qDebug() << "RSI modifié:" << "id=" << id << "période=" << period;
    
    // Mettre à jour le libellé
    if (m_indicatorLabels.contains(id)) {
        m_indicatorLabels[id]->setText(QString("RSI (%1)").arg(period));
    }
    
    // Mettre à jour le graphique
    if (m_chartWidget->hasValidData()) {
        m_chartWidget->updateChart();
    }
}

void ChartView::onRSIRemoved(int id)
{
    qDebug() << "RSI supprimé:" << "id=" << id;
    refreshIndicatorsList();
    
    // Mettre à jour le graphique
    if (m_chartWidget->hasValidData()) {
        m_chartWidget->updateChart();
    }
}

void ChartView::onEMAAdded(int id, int period)
{
    qDebug() << "EMA ajouté:" << "id=" << id << "période=" << period;
    
    // Créer les widgets pour cet EMA
    QString name = QString("EMA (%1)").arg(period);
    createIndicatorWidgets(id, name);
    
    // Mettre à jour le graphique
    if (m_chartWidget->hasValidData()) {
        m_chartWidget->updateChart();
    }
}

void ChartView::onEMAChanged(int id, int period)
{
    qDebug() << "EMA modifié:" << "id=" << id << "période=" << period;
    
    // Mettre à jour le libellé
    if (m_indicatorLabels.contains(id)) {
        m_indicatorLabels[id]->setText(QString("EMA (%1)").arg(period));
    }
    
    // Mettre à jour le graphique
    if (m_chartWidget->hasValidData()) {
        m_chartWidget->updateChart();
    }
}

void ChartView::onEMARemoved(int id)
{
    qDebug() << "EMA supprimé:" << "id=" << id;
    refreshIndicatorsList();
    
    // Mettre à jour le graphique
    if (m_chartWidget->hasValidData()) {
        m_chartWidget->updateChart();
    }
}

void ChartView::onEditEMA()
{
    if (!m_chartWidget->hasValidData()) {
        qDebug() << "Pas de données valides pour éditer un EMA";
        return;
    }
    
    // Créer et afficher le dialogue d'édition pour tous les EMA
    EMADialog* dialog = new EMADialog(this, m_chartWidget);
    dialog->exec();
    delete dialog;
    
    // Rafraîchir la liste des indicateurs
    refreshIndicatorsList();
}

void ChartView::createIndicatorWidgets(int id, const QString &name)
{
    // Créer un widget horizontal pour cet indicateur
    QWidget* indicatorWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(indicatorWidget);
    layout->setContentsMargins(0, 2, 0, 2);
    
    // Ajouter un libellé avec le nom de l'indicateur
    QLabel* nameLabel = new QLabel(name);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    // Ajouter des boutons d'édition et de suppression
    QPushButton* editButton = new QPushButton("Edit");
    editButton->setFixedWidth(40);
    connect(editButton, &QPushButton::clicked, [this, id]() {
        this->onEditIndicator(id);
    });
    
    QPushButton* removeButton = new QPushButton("X");
    removeButton->setFixedWidth(20);
    connect(removeButton, &QPushButton::clicked, [this, id]() {
        this->onRemoveIndicator(id);
    });
    
    // Ajouter les widgets au layout
    layout->addWidget(nameLabel);
    layout->addWidget(editButton);
    layout->addWidget(removeButton);
    
    // Stocker les références
    m_indicatorLabels[id] = nameLabel;
    m_editButtons[id] = editButton;
    m_removeButtons[id] = removeButton;
    
    // Ajouter au layout principal des indicateurs
    m_indicatorsLayout->addWidget(indicatorWidget);
}

void ChartView::onEditIndicator(int id)
{
    // Rechercher l'instance RSI avec cet ID
    ChartWidget::RSIInstance* rsi = m_chartWidget->findRSI(id);
    if (rsi) {
        // Créer et afficher le dialogue d'édition pour RSI
        IndicatorDialog* dialog = new IndicatorDialog(this, m_chartWidget, id, *rsi);
        dialog->exec();
        delete dialog;
        return;
    }
    
    // Rechercher si c'est un EMA
    ChartWidget::EMAInstance* ema = m_chartWidget->findEMA(id);
    if (ema) {
        // Pour les EMA, on ouvre le dialogue général des EMA
        onEditEMA();
        return;
    }
    
    qDebug() << "Indicateur introuvable:" << id;
}

void ChartView::onRemoveIndicator(int id)
{
    // Essayer de supprimer comme RSI
    if (m_chartWidget->removeRSI(id)) {
        return;
    }
    
    // Sinon, essayer de supprimer comme EMA
    m_chartWidget->removeEMA(id);
}

void ChartView::refreshIndicatorsList()
{
    // Supprimer tous les widgets d'indicateurs existants
    QLayoutItem* child;
    while ((child = m_indicatorsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    
    // Vider les maps
    m_indicatorLabels.clear();
    m_editButtons.clear();
    m_removeButtons.clear();
    
    // Pour chaque RSI actif, recréer les widgets
    const std::vector<ChartWidget::RSIInstance>& rsiInstances = m_chartWidget->getRSIInstances();
    for (const auto& rsi : rsiInstances) {
        QString name = QString("RSI (%1)").arg(rsi.period);
        createIndicatorWidgets(rsi.id, name);
    }

    // Pour chaque EMA actif, recréer les widgets
    const std::vector<ChartWidget::EMAInstance>& emaInstances = m_chartWidget->getEMAInstances();
    for (const auto& ema : emaInstances) {
        if (ema.visible) {
            QString name = QString("EMA (%1)").arg(ema.period);
            createIndicatorWidgets(ema.id, name);
        }
    }
    
    // Ajouter un widget spécial pour configurer tous les EMA ensemble
    if (!emaInstances.empty()) {
        QWidget* emaConfigWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(emaConfigWidget);
        layout->setContentsMargins(0, 2, 0, 2);
        
        QLabel* nameLabel = new QLabel("Configure all EMAs");
        nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        
        QPushButton* editButton = new QPushButton("Edit");
        editButton->setFixedWidth(40);
        connect(editButton, &QPushButton::clicked, this, &ChartView::onEditEMA);
        
        layout->addWidget(nameLabel);
        layout->addWidget(editButton);
        
        m_indicatorsLayout->addWidget(emaConfigWidget);
    }
}

void ChartView::updateData(BacktestResults* results)
{
    QTime start = QTime::currentTime();
            
    // Récupérer les résultats depuis l'App
    BacktestResults* appResults = m_app ? m_app->getBacktestResults() : nullptr;
    
    // Mettre à jour les références locales
    m_currentResults = appResults;

    // Mettre en cache les nouveaux pointeurs
    m_cachedResults = appResults;
    
    if (!appResults || !appResults->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }
    
    try {
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
        
        // Rafraîchir la liste des indicateurs
        refreshIndicatorsList();
        
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

void ChartView::clear()
{    
    m_currentResults = nullptr;
    m_cachedResults = nullptr;
    m_dataExtracted = false;
    
    // Nettoyer le widget de graphique
    if (m_chartWidget) {
        m_chartWidget->clearChart();
    }
    
    // Vider la liste des indicateurs
    refreshIndicatorsList();
    
    // Réafficher le placeholder
    showPlaceholder("Exécutez un backtest pour afficher les graphiques");
}

// ================================================================
// Implémentation de la classe IndicatorDialog
// ================================================================

IndicatorDialog::IndicatorDialog(QWidget* parent, ChartWidget* chartWidget, int rsiId, const ChartWidget::RSIInstance& rsi)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
    , m_rsiId(rsiId)
    , m_originalRsi(rsi)
    , m_currentRsi(rsi)
{
    // Configuration du dialogue
    setWindowTitle("RSI Settings");
    setMinimumWidth(300);
    setModal(true);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Formulaire de paramètres
    QFormLayout* formLayout = new QFormLayout();
    
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(rsi.period);
    formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(rsi.height);
    formLayout->addRow("Height:", m_heightSpinBox);
    
    // Range
    m_rangeSpinBox = new QDoubleSpinBox();
    m_rangeSpinBox->setRange(5, 40);
    m_rangeSpinBox->setSingleStep(1);
    m_rangeSpinBox->setValue(rsi.range);
    formLayout->addRow("Range:", m_rangeSpinBox);
    
    // Couleur principale
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, rsi.color);
    formLayout->addRow("Line Color:", m_colorButton);
    
    // Couleur zone supérieure
    m_upperColorButton = new QPushButton();
    updateColorButtonStyle(m_upperColorButton, rsi.upperColor);
    formLayout->addRow("Upper Zone Color:", m_upperColorButton);
    
    // Couleur zone inférieure
    m_lowerColorButton = new QPushButton();
    updateColorButtonStyle(m_lowerColorButton, rsi.lowerColor);
    formLayout->addRow("Lower Zone Color:", m_lowerColorButton);
    
    // Ajouter le formulaire au layout
    mainLayout->addLayout(formLayout);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
    
    // Connecter les signaux
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &IndicatorDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &IndicatorDialog::onHeightChanged);
    connect(m_rangeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndicatorDialog::onRangeChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &IndicatorDialog::onColorButtonClicked);
    connect(m_upperColorButton, &QPushButton::clicked, this, &IndicatorDialog::onUpperColorButtonClicked);
    connect(m_lowerColorButton, &QPushButton::clicked, this, &IndicatorDialog::onLowerColorButtonClicked);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &IndicatorDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &IndicatorDialog::onCancel);
}

IndicatorDialog::~IndicatorDialog()
{
}

void IndicatorDialog::updateColorButtonStyle(QPushButton* button, int color)
{
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    
    QString styleSheet = QString("background-color: rgb(%1, %2, %3); ")
                          .arg(r).arg(g).arg(b);
                          
    // Ajuster le texte pour qu'il soit lisible sur la couleur de fond
    int brightness = (r * 299 + g * 587 + b * 114) / 1000;
    if (brightness > 125) {
        styleSheet += "color: black;";
    } else {
        styleSheet += "color: white;";
    }
    
    button->setStyleSheet(styleSheet);
    button->setText(QString("#%1").arg(color, 6, 16, QChar('0')));
}

void IndicatorDialog::onPeriodChanged(int period)
{
    m_currentRsi.period = period;
    // updateRSI();
}

void IndicatorDialog::onHeightChanged(int height)
{
    m_currentRsi.height = height;
    // updateRSI();
}

void IndicatorDialog::onRangeChanged(double range)
{
    m_currentRsi.range = range;
    // updateRSI();
}

void IndicatorDialog::onColorButtonClicked()
{
    int r = (m_currentRsi.color >> 16) & 0xFF;
    int g = (m_currentRsi.color >> 8) & 0xFF;
    int b = m_currentRsi.color & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentRsi.color = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_colorButton, m_currentRsi.color);
        // updateRSI();
    }
}

void IndicatorDialog::onUpperColorButtonClicked()
{
    int r = (m_currentRsi.upperColor >> 16) & 0xFF;
    int g = (m_currentRsi.upperColor >> 8) & 0xFF;
    int b = m_currentRsi.upperColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentRsi.upperColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_upperColorButton, m_currentRsi.upperColor);
        // updateRSI();
    }
}

void IndicatorDialog::onLowerColorButtonClicked()
{
    int r = (m_currentRsi.lowerColor >> 16) & 0xFF;
    int g = (m_currentRsi.lowerColor >> 8) & 0xFF;
    int b = m_currentRsi.lowerColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentRsi.lowerColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_lowerColorButton, m_currentRsi.lowerColor);
        // updateRSI();
    }
}

void IndicatorDialog::updateRSI()
{
    // Mettre à jour les paramètres du RSI en temps réel
    // m_chartWidget->setRSIPeriod(m_rsiId, m_currentRsi.period);
    // m_chartWidget->setRSIHeight(m_rsiId, m_currentRsi.height);
    // m_chartWidget->setRSIRange(m_rsiId, m_currentRsi.range);
    // m_chartWidget->setRSIColor(m_rsiId, m_currentRsi.color);
    
    // Note: ChartWidget devrait avoir des méthodes pour ces paramètres
    // si ce n'est pas le cas, il faudra les ajouter
}

void IndicatorDialog::onApply()
{
    m_chartWidget->setRSIPeriod(m_rsiId, m_currentRsi.period);
    m_chartWidget->setRSIHeight(m_rsiId, m_currentRsi.height);
    m_chartWidget->setRSIRange(m_rsiId, m_currentRsi.range);
    m_chartWidget->setRSIColor(m_rsiId, m_currentRsi.color);

    // Note: Si ces setters n'existent pas encore, vous devrez les implémenter dans ChartWidget
    // m_chartWidget->setRSIUpperColor(m_rsiId, m_currentRsi.upperColor);
    // m_chartWidget->setRSILowerColor(m_rsiId, m_currentRsi.lowerColor);

    m_chartWidget->updateChart();

    accept();
}

void IndicatorDialog::onCancel()
{
    // Restaurer les paramètres d'origine
    m_chartWidget->setRSIPeriod(m_rsiId, m_originalRsi.period);
    m_chartWidget->setRSIHeight(m_rsiId, m_originalRsi.height);
    m_chartWidget->setRSIRange(m_rsiId, m_originalRsi.range);
    m_chartWidget->setRSIColor(m_rsiId, m_originalRsi.color);
    
    // Mettre à jour le graphique
    m_chartWidget->updateChart();
    
    // Fermer la boîte de dialogue
    reject();
}

// ================================================================
// Implémentation de la classe EMADialog
// ================================================================

EMADialog::EMADialog(QWidget* parent, ChartWidget* chartWidget)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
    , m_nextRowId(0)
{
    // Configuration du dialogue
    setWindowTitle("EMA Settings");
    setMinimumWidth(450);
    setMinimumHeight(350);
    setModal(true);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Section de titre
    QLabel* titleLabel = new QLabel("Configure Exponential Moving Averages");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // En-tête des colonnes
    QWidget* headerWidget = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(5, 0, 5, 0);
    
    QLabel* enabledLabel = new QLabel("Enabled");
    enabledLabel->setFixedWidth(60);
    enabledLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* periodLabel = new QLabel("Period");
    periodLabel->setFixedWidth(70);
    periodLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* colorLabel = new QLabel("Color");
    colorLabel->setFixedWidth(80);
    colorLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* actionLabel = new QLabel("Action");
    actionLabel->setFixedWidth(60);
    actionLabel->setAlignment(Qt::AlignCenter);
    
    headerLayout->addWidget(enabledLabel);
    headerLayout->addWidget(periodLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(colorLabel);
    headerLayout->addWidget(actionLabel);
    
    mainLayout->addWidget(headerWidget);
    
    // Zone de défilement pour les lignes d'EMA
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* scrollWidget = new QWidget();
    m_emaListLayout = new QVBoxLayout(scrollWidget);
    m_emaListLayout->setContentsMargins(0, 0, 0, 0);
    m_emaListLayout->setSpacing(5);
    
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);
    
    // Récupérer les instances EMA existantes
    const std::vector<ChartWidget::EMAInstance>& emaInstances = m_chartWidget->getEMAInstances();
    m_originalEMAs = emaInstances;
    m_currentEMAs = emaInstances;
    
    // Créer les lignes pour les EMA existants
    refreshEMAList();
    
    // Bouton d'ajout d'un nouvel EMA
    m_addEMAButton = new QPushButton("Add New EMA");
    connect(m_addEMAButton, &QPushButton::clicked, this, &EMADialog::onAddEMA);
    mainLayout->addWidget(m_addEMAButton);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &EMADialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &EMADialog::onCancel);
}

EMADialog::~EMADialog()
{
}

void EMADialog::updateColorButtonStyle(QPushButton* button, int color)
{
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    
    QString styleSheet = QString("background-color: rgb(%1, %2, %3); ")
                          .arg(r).arg(g).arg(b);
                          
    // Ajuster le texte pour qu'il soit lisible sur la couleur de fond
    int brightness = (r * 299 + g * 587 + b * 114) / 1000;
    if (brightness > 125) {
        styleSheet += "color: black;";
    } else {
        styleSheet += "color: white;";
    }
    
    button->setStyleSheet(styleSheet);
    button->setText(QString("#%1").arg(color, 6, 16, QChar('0')));
}

QWidget* EMADialog::createEMARow(const ChartWidget::EMAInstance& ema, int row)
{
    QWidget* rowWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(5, 0, 5, 0);
    
    // Case à cocher pour activer/désactiver
    QCheckBox* enabledCheckBox = new QCheckBox();
    enabledCheckBox->setChecked(ema.visible);
    enabledCheckBox->setFixedWidth(60);
    connect(enabledCheckBox, &QCheckBox::toggled, [this, row](bool checked) {
        this->onEnabledStateChanged(row, checked);
    });
    
    // Sélecteur de période
    QSpinBox* periodSpinBox = new QSpinBox();
    periodSpinBox->setRange(2, 200);
    periodSpinBox->setValue(ema.period);
    periodSpinBox->setFixedWidth(70);
    connect(periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, row](int value) {
        this->onPeriodChanged(row, value);
    });
    
    // Bouton de sélection de couleur
    QPushButton* colorButton = new QPushButton();
    updateColorButtonStyle(colorButton, ema.color);
    colorButton->setFixedWidth(80);
    connect(colorButton, &QPushButton::clicked, [this, row]() {
        this->onColorButtonClicked(row);
    });
    
    // Bouton de suppression
    QPushButton* removeButton = new QPushButton("Remove");
    removeButton->setFixedWidth(60);
    connect(removeButton, &QPushButton::clicked, [this, row]() {
        this->onRemoveEMA(row);
    });
    
    // Ajouter les widgets au layout
    layout->addWidget(enabledCheckBox);
    layout->addWidget(periodSpinBox);
    layout->addStretch();
    layout->addWidget(colorButton);
    layout->addWidget(removeButton);
    
    // Ajouter une propriété pour identifier la ligne
    rowWidget->setProperty("row", row);
    
    return rowWidget;
}

void EMADialog::refreshEMAList()
{
    // Effacer tous les widgets existants
    QLayoutItem* child;
    while ((child = m_emaListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    
    // Recréer toutes les lignes
    m_rowToEMAId.clear();
    int row = 0;
    
    for (size_t i = 0; i < m_currentEMAs.size(); ++i) {
        const auto& ema = m_currentEMAs[i];
        QWidget* rowWidget = createEMARow(ema, row);
        m_emaListLayout->addWidget(rowWidget);
        m_rowToEMAId[row] = ema.id;
        row++;
    }
    
    // Si la liste est vide, créer une ligne par défaut
    if (m_currentEMAs.empty()) {
        onAddEMA();
    }
}

void EMADialog::onEnabledStateChanged(int row, bool enabled)
{
    if (row < 0 || row >= static_cast<int>(m_currentEMAs.size())) return;
    
    // Mettre à jour l'état de visibilité de l'EMA
    auto it = std::find_if(m_currentEMAs.begin(), m_currentEMAs.end(), 
                         [this, row](const ChartWidget::EMAInstance& ema) {
                             return ema.id == m_rowToEMAId[row];
                         });
    
    if (it != m_currentEMAs.end()) {
        it->visible = enabled;
    }
}

void EMADialog::onPeriodChanged(int row, int period)
{
    if (row < 0 || row >= static_cast<int>(m_currentEMAs.size())) return;
    
    // Mettre à jour la période de l'EMA
    auto it = std::find_if(m_currentEMAs.begin(), m_currentEMAs.end(), 
                         [this, row](const ChartWidget::EMAInstance& ema) {
                             return ema.id == m_rowToEMAId[row];
                         });
    
    if (it != m_currentEMAs.end()) {
        it->period = period;
    }
}

void EMADialog::onColorButtonClicked(int row)
{
    if (row < 0 || row >= static_cast<int>(m_currentEMAs.size())) return;
    
    // Trouver l'EMA correspondant à cette ligne
    auto it = std::find_if(m_currentEMAs.begin(), m_currentEMAs.end(), 
                         [this, row](const ChartWidget::EMAInstance& ema) {
                             return ema.id == m_rowToEMAId[row];
                         });
    
    if (it == m_currentEMAs.end()) return;
    
    // Ouvrir le sélecteur de couleur
    int r = (it->color >> 16) & 0xFF;
    int g = (it->color >> 8) & 0xFF;
    int b = it->color & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        // Mettre à jour la couleur
        it->color = (color.red() << 16) | (color.green() << 8) | color.blue();
        
        // Mettre à jour l'apparence du bouton
        QWidget* widget = m_emaListLayout->itemAt(row)->widget();
        if (widget) {
            QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(widget->layout());
            if (layout) {
                // Le bouton de couleur est généralement le 3e widget (index 2)
                QLayoutItem* item = layout->itemAt(3);
                if (item && item->widget()) {
                    QPushButton* colorButton = qobject_cast<QPushButton*>(item->widget());
                    if (colorButton) {
                        updateColorButtonStyle(colorButton, it->color);
                    }
                }
            }
        }
    }
}

void EMADialog::onAddEMA()
{
    // Créer un nouvel EMA avec des valeurs par défaut
    ChartWidget::EMAInstance newEMA;
    newEMA.id = -1 - m_nextRowId; // ID temporaire négatif
    newEMA.period = 20;
    newEMA.visible = true;
    
    // Attribuer une couleur spécifique selon le nombre d'EMA
    // Utiliser des couleurs distinctes pour faciliter la distinction
    static const int colors[] = {
        0x0000FF,  // Bleu
        0xFF0000,  // Rouge
        0x00AA00,  // Vert
        0xAA00AA,  // Violet
        0xFF8800,  // Orange
        0x008888,  // Cyan
        0x880088   // Magenta
    };
    newEMA.color = colors[m_currentEMAs.size() % 7];
    
    // Ajouter à la liste
    m_currentEMAs.push_back(newEMA);
    
    // Créer la nouvelle ligne dans l'interface
    int row = m_currentEMAs.size() - 1;
    QWidget* rowWidget = createEMARow(newEMA, row);
    m_emaListLayout->addWidget(rowWidget);
    m_rowToEMAId[row] = newEMA.id;
    
    m_nextRowId++;
}

void EMADialog::onRemoveEMA(int row)
{
    if (row < 0 || row >= static_cast<int>(m_currentEMAs.size())) return;
    
    // Supprimer l'EMA de la liste
    auto it = std::find_if(m_currentEMAs.begin(), m_currentEMAs.end(), 
                         [this, row](const ChartWidget::EMAInstance& ema) {
                             return ema.id == m_rowToEMAId[row];
                         });
    
    if (it != m_currentEMAs.end()) {
        m_currentEMAs.erase(it);
    }
    
    // Rafraîchir la liste
    refreshEMAList();
}

void EMADialog::onApply()
{
    // 1. Supprimer tous les EMA existants dont l'ID est positif (EMA réels dans le chartWidget)
    auto it = m_currentEMAs.begin();
    while (it != m_currentEMAs.end()) {
        if (it->id > 0) {
            m_chartWidget->removeEMA(it->id);
        }
        ++it;
    }
    
    // 2. Ajouter/recréer tous les EMA de la liste courante
    for (const auto& ema : m_currentEMAs) {
        int id = m_chartWidget->addEMA(ema.period);
        m_chartWidget->setEMAVisible(id, ema.visible);
        m_chartWidget->setEMAColor(id, ema.color);
    }
    
    // 3. Mettre à jour le graphique
    m_chartWidget->updateChart();
    
    // 4. Fermer la boîte de dialogue
    accept();
}

void EMADialog::onCancel()
{
    // 1. Supprimer tous les EMA existants dont l'ID est positif (EMA réels dans le chartWidget)
    for (const auto& ema : m_currentEMAs) {
        if (ema.id > 0) {
            m_chartWidget->removeEMA(ema.id);
        }
    }
    
    // 2. Restaurer les EMA d'origine
    for (const auto& ema : m_originalEMAs) {
        int id = m_chartWidget->addEMA(ema.period);
        m_chartWidget->setEMAVisible(id, ema.visible);
        m_chartWidget->setEMAColor(id, ema.color);
    }
    
    // 3. Mettre à jour le graphique
    m_chartWidget->updateChart();
    
    // 4. Fermer la boîte de dialogue
    reject();
}