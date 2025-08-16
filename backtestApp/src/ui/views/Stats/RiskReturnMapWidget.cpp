#include "ui/views/Stats/RiskReturnMapWidget.h"
#include <QDebug>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QDialog>
#include <QPainter>
#include <QPixmap>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QColorDialog>
#include <QtMath>
#include <QPen>
#include <QLineEdit>
#include <QBrush>

RiskReturnMapWidget::RiskReturnMapWidget(QWidget* parent)
    : QWidget(parent),
      m_minVolatility(0.0),
      m_maxVolatility(30.0),
      m_minReturn(-10.0),
      m_maxReturn(30.0),
      m_chartMarginLeft(60),
      m_chartMarginRight(20),
      m_chartMarginTop(20),
      m_chartMarginBottom(40),
      m_currentMode(ReturnVsVolatility),
      m_showSharpeLines(true)
{
    // Créer le groupe box principal
    m_groupBox = new QGroupBox("Carte Performance/Risque");
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_groupBox);
    
    // Layout principal du group box
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    
    // Créer la ligne de contrôles
    m_controlsLayout = new QHBoxLayout();
    
    // Mode d'affichage
    QLabel* modeLabel = new QLabel("Mode :");
    m_displayModeCombo = new QComboBox();
    m_displayModeCombo->addItem("Rendement vs Volatilité", ReturnVsVolatility);
    m_displayModeCombo->addItem("Rendement vs Drawdown", ReturnVsDrawdown);
    m_displayModeCombo->addItem("Sharpe vs Sortino", SharpeVsSortino);
    m_controlsLayout->addWidget(modeLabel);
    m_controlsLayout->addWidget(m_displayModeCombo);
    
    // Lignes de Sharpe
    m_showSharpeLinesCheck = new QCheckBox("Afficher lignes Sharpe");
    m_showSharpeLinesCheck->setChecked(true);
    m_controlsLayout->addWidget(m_showSharpeLinesCheck);
    
    m_controlsLayout->addStretch();
    
    // Boutons d'action
    m_addStrategyBtn = new QPushButton("Ajouter stratégie");
    m_removeStrategyBtn = new QPushButton("Supprimer");
    m_loadStrategiesBtn = new QPushButton("Charger");
    m_saveStrategiesBtn = new QPushButton("Sauvegarder");
    
    m_controlsLayout->addWidget(m_addStrategyBtn);
    m_controlsLayout->addWidget(m_removeStrategyBtn);
    m_controlsLayout->addWidget(m_loadStrategiesBtn);
    m_controlsLayout->addWidget(m_saveStrategiesBtn);
    
    // Ajouter les contrôles au layout
    groupLayout->addLayout(m_controlsLayout);
    
    // Layout principal séparé en deux parties: graphique et tableau
    QHBoxLayout* mainContentLayout = new QHBoxLayout();
    
    // Graphique à gauche (70% de l'espace)
    QVBoxLayout* chartLayout = new QVBoxLayout();
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setMinimumHeight(400);
    m_view->setMinimumWidth(500);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setAlignment(Qt::AlignCenter);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    
    chartLayout->addWidget(m_view);
    mainContentLayout->addLayout(chartLayout, 7);
    
    // Tableau à droite (30% de l'espace)
    QVBoxLayout* tableLayout = new QVBoxLayout();
    m_strategiesModel = new QStandardItemModel(0, 5, this);
    m_strategiesModel->setHorizontalHeaderLabels(QStringList() 
        << "Stratégie" << "Rendement (%)" << "Volatilité (%)" 
        << "Sharpe" << "Max DD (%)");
    
    m_strategiesTable = new QTableView();
    m_strategiesTable->setModel(m_strategiesModel);
    m_strategiesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_strategiesTable->verticalHeader()->setVisible(false);
    m_strategiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_strategiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_strategiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_strategiesTable->setAlternatingRowColors(true);
    
    tableLayout->addWidget(new QLabel("Stratégies comparées :"));
    tableLayout->addWidget(m_strategiesTable);
    mainContentLayout->addLayout(tableLayout, 3);
    
    groupLayout->addLayout(mainContentLayout);
    
    // Connexion des signaux
    connect(m_displayModeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDisplayMode(int)));
    connect(m_showSharpeLinesCheck, SIGNAL(toggled(bool)), this, SLOT(toggleSharpeLines(bool)));
    connect(m_addStrategyBtn, SIGNAL(clicked()), this, SLOT(addCustomStrategy()));
    connect(m_removeStrategyBtn, SIGNAL(clicked()), this, SLOT(removeSelectedStrategy()));
    connect(m_loadStrategiesBtn, SIGNAL(clicked()), this, SLOT(loadStrategiesFromFile()));
    connect(m_saveStrategiesBtn, SIGNAL(clicked()), this, SLOT(saveStrategiesToFile()));
    connect(m_view, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    
    // Initialiser les points de référence
    initReferencePoints();
    
    // Construire le graphique initialement vide
    buildChart();
}

void RiskReturnMapWidget::initReferencePoints()
{
    // Définir quelques points de référence standards du marché
    m_referencePoints["SPY"] = StrategyPoint("SPY (S&P 500)", 10.0, 15.0, 0.66, 0.95, 33.9, QColor(220, 220, 220), false, true);
    m_referencePoints["QQQ"] = StrategyPoint("QQQ (NASDAQ)", 15.5, 22.0, 0.70, 0.97, 45.6, QColor(200, 200, 220), false, true);
    m_referencePoints["IWM"] = StrategyPoint("IWM (Russell 2000)", 9.2, 19.5, 0.47, 0.65, 40.7, QColor(220, 200, 200), false, true);
    m_referencePoints["GLD"] = StrategyPoint("GLD (Or)", 7.8, 16.3, 0.48, 0.72, 29.4, QColor(255, 215, 0), false, true);
    m_referencePoints["TLT"] = StrategyPoint("TLT (Bons du Trésor)", 5.2, 13.1, 0.40, 0.58, 26.8, QColor(200, 220, 200), false, true);
    
    // Ajouter ces points à notre liste de stratégies
    for (auto it = m_referencePoints.begin(); it != m_referencePoints.end(); ++it) {
        m_strategies.append(it.value());
    }
    
    // Mettre à jour le tableau
    updateStrategiesTable();
}

void RiskReturnMapWidget::updateData(const be::Stats& stats)
{
    // Créer un point représentant la stratégie courante
    StrategyPoint currentStrategy(
        "Stratégie actuelle",
        stats.returnAnnPct,
        stats.volatilityAnnPct,
        stats.sharpeRatio,
        stats.sortinoRatio,
        std::abs(stats.maxDrawdownPct),
        QColor(0, 120, 215),  // Bleu Microsoft
        true,
        false
    );
    
    // Remplacer l'ancienne stratégie courante s'il y en a une
    bool found = false;
    for (int i = 0; i < m_strategies.size(); ++i) {
        if (m_strategies[i].isCurrentStrategy) {
            m_strategies[i] = currentStrategy;
            found = true;
            break;
        }
    }
    
    // Si aucune stratégie courante n'a été trouvée, l'ajouter
    if (!found) {
        m_strategies.append(currentStrategy);
    }
    
    // Mettre à jour le tableau et le graphique
    updateStrategiesTable();
    recalculateRanges();
    buildChart();
    
    // Rendre le widget visible
    m_groupBox->setVisible(true);
}

void RiskReturnMapWidget::clear()
{
    // Supprimer toutes les stratégies sauf les points de référence
    QList<StrategyPoint> referenceOnly;
    for (const auto& strategy : m_strategies) {
        if (strategy.isReferencePoint) {
            referenceOnly.append(strategy);
        }
    }
    
    m_strategies = referenceOnly;
    
    // Mettre à jour le tableau et le graphique
    updateStrategiesTable();
    recalculateRanges();
    buildChart();
    
    // Cacher le widget car aucune stratégie active
    m_groupBox->setVisible(false);
}

void RiskReturnMapWidget::addCustomStrategy()
{
    // Créer une boîte de dialogue pour saisir les paramètres
    QDialog dialog(this);
    dialog.setWindowTitle("Ajouter une stratégie");
    
    QFormLayout* form = new QFormLayout(&dialog);
    
    // Champs de saisie
    QLineEdit* nameEdit = new QLineEdit(&dialog);
    nameEdit->setText("Nouvelle stratégie");
    
    QDoubleSpinBox* returnSpin = new QDoubleSpinBox(&dialog);
    returnSpin->setRange(-100.0, 100.0);
    returnSpin->setSuffix(" %");
    returnSpin->setDecimals(2);
    returnSpin->setValue(10.0);
    
    QDoubleSpinBox* volatilitySpin = new QDoubleSpinBox(&dialog);
    volatilitySpin->setRange(0.1, 100.0);
    volatilitySpin->setSuffix(" %");
    volatilitySpin->setDecimals(2);
    volatilitySpin->setValue(15.0);
    
    QDoubleSpinBox* sharpeSpin = new QDoubleSpinBox(&dialog);
    sharpeSpin->setRange(-10.0, 10.0);
    sharpeSpin->setDecimals(2);
    sharpeSpin->setValue(0.5);
    
    QDoubleSpinBox* sortinoSpin = new QDoubleSpinBox(&dialog);
    sortinoSpin->setRange(-10.0, 10.0);
    sortinoSpin->setDecimals(2);
    sortinoSpin->setValue(0.7);
    
    QDoubleSpinBox* maxDDSpin = new QDoubleSpinBox(&dialog);
    maxDDSpin->setRange(0.1, 100.0);
    maxDDSpin->setSuffix(" %");
    maxDDSpin->setDecimals(2);
    maxDDSpin->setValue(20.0);
    
    QPushButton* colorButton = new QPushButton("Choisir...");
    QColor selectedColor = QColor(100, 100, 255);
    colorButton->setStyleSheet("background-color: " + selectedColor.name());
    
    // Connecter le bouton de couleur
    connect(colorButton, &QPushButton::clicked, [&]() {
        QColor color = QColorDialog::getColor(selectedColor, this, "Choisir une couleur");
        if (color.isValid()) {
            selectedColor = color;
            colorButton->setStyleSheet("background-color: " + color.name());
        }
    });
    
    // Ajouter les champs au formulaire
    form->addRow("Nom:", nameEdit);
    form->addRow("Rendement annuel:", returnSpin);
    form->addRow("Volatilité annuelle:", volatilitySpin);
    form->addRow("Ratio de Sharpe:", sharpeSpin);
    form->addRow("Ratio de Sortino:", sortinoSpin);
    form->addRow("Drawdown maximal:", maxDDSpin);
    form->addRow("Couleur:", colorButton);
    
    // Boutons OK/Annuler
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form->addRow(buttonBox);
    
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    
    // Exécuter la boîte de dialogue
    if (dialog.exec() == QDialog::Accepted) {
        // Créer la nouvelle stratégie avec les valeurs saisies
        StrategyPoint newStrategy(
            nameEdit->text(),
            returnSpin->value(),
            volatilitySpin->value(),
            sharpeSpin->value(),
            sortinoSpin->value(),
            maxDDSpin->value(),
            selectedColor,
            false,
            false
        );
        
        // Ajouter la stratégie à la liste
        m_strategies.append(newStrategy);
        
        // Mettre à jour le tableau et le graphique
        updateStrategiesTable();
        recalculateRanges();
        buildChart();
        
        // Rendre le widget visible si nécessaire
        m_groupBox->setVisible(true);
    }
}

void RiskReturnMapWidget::removeSelectedStrategy()
{
    // Récupérer l'index de la ligne sélectionnée
    QModelIndexList selection = m_strategiesTable->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::information(this, "Information", "Veuillez sélectionner une stratégie à supprimer.");
        return;
    }
    
    int row = selection.first().row();
    
    // Vérifier que ce n'est pas un point de référence
    if (m_strategies[row].isReferencePoint) {
        QMessageBox::warning(this, "Impossible de supprimer", 
                           "Les points de référence standards ne peuvent pas être supprimés.");
        return;
    }
    
    // Supprimer la stratégie sélectionnée
    m_strategies.removeAt(row);
    
    // Mettre à jour le tableau et le graphique
    updateStrategiesTable();
    recalculateRanges();
    buildChart();
}

void RiskReturnMapWidget::loadStrategiesFromFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        "Charger des stratégies", "", "Fichiers JSON (*.json)");
        
    if (filePath.isEmpty()) {
        return;
    }
    
    loadStrategiesData(filePath);
}

void RiskReturnMapWidget::loadStrategiesData(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Erreur", "Impossible d'ouvrir le fichier.");
        return;
    }
    
    QByteArray fileData = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(fileData, &error);
    
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "Erreur de format", 
                           "Le fichier n'est pas un JSON valide: " + error.errorString());
        return;
    }
    
    if (!doc.isArray()) {
        QMessageBox::warning(this, "Erreur de format", "Le fichier doit contenir un tableau de stratégies.");
        return;
    }
    
    // Sauvegarder les points de référence et la stratégie courante
    QList<StrategyPoint> keepStrategies;
    for (const auto& strategy : m_strategies) {
        if (strategy.isReferencePoint || strategy.isCurrentStrategy) {
            keepStrategies.append(strategy);
        }
    }
    
    // Charger les nouvelles stratégies
    QJsonArray strategiesArray = doc.array();
    for (const QJsonValue& value : strategiesArray) {
        if (!value.isObject()) continue;
        
        QJsonObject obj = value.toObject();
        
        StrategyPoint strategy;
        strategy.name = obj["name"].toString();
        strategy.returnPct = obj["returnPct"].toDouble();
        strategy.volatilityPct = obj["volatilityPct"].toDouble();
        strategy.sharpeRatio = obj["sharpeRatio"].toDouble();
        strategy.sortinoRatio = obj["sortinoRatio"].toDouble();
        strategy.maxDrawdownPct = obj["maxDrawdownPct"].toDouble();
        
        QJsonArray colorArray = obj["color"].toArray();
        if (colorArray.size() >= 3) {
            strategy.color = QColor(
                colorArray[0].toInt(),
                colorArray[1].toInt(),
                colorArray[2].toInt()
            );
        }
        
        strategy.isCurrentStrategy = false;  // Jamais charger comme stratégie courante
        strategy.isReferencePoint = obj["isReferencePoint"].toBool();
        
        if (!strategy.isReferencePoint) {  // Ne pas dupliquer les points de référence
            keepStrategies.append(strategy);
        }
    }
    
    m_strategies = keepStrategies;
    
    // Mettre à jour le tableau et le graphique
    updateStrategiesTable();
    recalculateRanges();
    buildChart();
    
    // Rendre le widget visible
    m_groupBox->setVisible(true);
}

void RiskReturnMapWidget::saveStrategiesToFile()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        "Sauvegarder les stratégies", "", "Fichiers JSON (*.json)");
        
    if (filePath.isEmpty()) {
        return;
    }
    
    // Ajouter l'extension .json si nécessaire
    if (!filePath.endsWith(".json", Qt::CaseInsensitive)) {
        filePath += ".json";
    }
    
    saveStrategiesData(filePath);
}

void RiskReturnMapWidget::saveStrategiesData(const QString& filePath)
{
    QJsonArray strategiesArray;
    
    // Convertir toutes les stratégies en objets JSON
    for (const auto& strategy : m_strategies) {
        QJsonObject obj;
        obj["name"] = strategy.name;
        obj["returnPct"] = strategy.returnPct;
        obj["volatilityPct"] = strategy.volatilityPct;
        obj["sharpeRatio"] = strategy.sharpeRatio;
        obj["sortinoRatio"] = strategy.sortinoRatio;
        obj["maxDrawdownPct"] = strategy.maxDrawdownPct;
        
        QJsonArray colorArray;
        colorArray.append(strategy.color.red());
        colorArray.append(strategy.color.green());
        colorArray.append(strategy.color.blue());
        obj["color"] = colorArray;
        
        obj["isCurrentStrategy"] = strategy.isCurrentStrategy;
        obj["isReferencePoint"] = strategy.isReferencePoint;
        
        strategiesArray.append(obj);
    }
    
    QJsonDocument doc(strategiesArray);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erreur", "Impossible d'écrire dans le fichier.");
        return;
    }
    
    file.write(jsonData);
    file.close();
    
    QMessageBox::information(this, "Sauvegarde réussie", 
                          QString("%1 stratégies ont été sauvegardées.").arg(m_strategies.size()));
}

void RiskReturnMapWidget::changeDisplayMode(int index)
{
    m_currentMode = static_cast<DisplayMode>(m_displayModeCombo->itemData(index).toInt());
    recalculateRanges();
    buildChart();
}

void RiskReturnMapWidget::toggleSharpeLines(bool checked)
{
    m_showSharpeLines = checked;
    buildChart();
}

void RiskReturnMapWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // Recalculer les dimensions du graphique
    calculateChartDimensions();
    
    // Reconstruire le graphique après redimensionnement
    buildChart();
}

void RiskReturnMapWidget::calculateChartDimensions()
{
    // Calculer les dimensions disponibles pour le graphique
    QRect viewportRect = m_view->viewport()->rect();
    m_chartWidth = viewportRect.width() - m_chartMarginLeft - m_chartMarginRight;
    m_chartHeight = viewportRect.height() - m_chartMarginTop - m_chartMarginBottom;
}

void RiskReturnMapWidget::recalculateRanges()
{
    if (m_strategies.isEmpty()) {
        // Valeurs par défaut
        m_minVolatility = 0.0;
        m_maxVolatility = 30.0;
        m_minReturn = -10.0;
        m_maxReturn = 30.0;
        return;
    }
    
    // Trouver les valeurs min/max en fonction du mode d'affichage
    if (m_currentMode == ReturnVsVolatility) {
        // Initialiser avec les valeurs de la première stratégie
        m_minVolatility = m_strategies[0].volatilityPct;
        m_maxVolatility = m_strategies[0].volatilityPct;
        m_minReturn = m_strategies[0].returnPct;
        m_maxReturn = m_strategies[0].returnPct;
        
        // Parcourir toutes les stratégies
        for (const auto& strategy : m_strategies) {
            m_minVolatility = qMin(m_minVolatility, strategy.volatilityPct);
            m_maxVolatility = qMax(m_maxVolatility, strategy.volatilityPct);
            m_minReturn = qMin(m_minReturn, strategy.returnPct);
            m_maxReturn = qMax(m_maxReturn, strategy.returnPct);
        }
    } 
    else if (m_currentMode == ReturnVsDrawdown) {
        // Initialiser avec les valeurs de la première stratégie
        m_minVolatility = m_strategies[0].maxDrawdownPct;
        m_maxVolatility = m_strategies[0].maxDrawdownPct;
        m_minReturn = m_strategies[0].returnPct;
        m_maxReturn = m_strategies[0].returnPct;
        
        // Parcourir toutes les stratégies
        for (const auto& strategy : m_strategies) {
            m_minVolatility = qMin(m_minVolatility, strategy.maxDrawdownPct);
            m_maxVolatility = qMax(m_maxVolatility, strategy.maxDrawdownPct);
            m_minReturn = qMin(m_minReturn, strategy.returnPct);
            m_maxReturn = qMax(m_maxReturn, strategy.returnPct);
        }
    } 
    else if (m_currentMode == SharpeVsSortino) {
        // Initialiser avec les valeurs de la première stratégie
        m_minVolatility = m_strategies[0].sharpeRatio;
        m_maxVolatility = m_strategies[0].sharpeRatio;
        m_minReturn = m_strategies[0].sortinoRatio;
        m_maxReturn = m_strategies[0].sortinoRatio;
        
        // Parcourir toutes les stratégies
        for (const auto& strategy : m_strategies) {
            m_minVolatility = qMin(m_minVolatility, strategy.sharpeRatio);
            m_maxVolatility = qMax(m_maxVolatility, strategy.sharpeRatio);
            m_minReturn = qMin(m_minReturn, strategy.sortinoRatio);
            m_maxReturn = qMax(m_maxReturn, strategy.sortinoRatio);
        }
    }
    
    // Ajouter une marge de 10%
    double volatilityRange = m_maxVolatility - m_minVolatility;
    m_minVolatility -= volatilityRange * 0.1;
    m_maxVolatility += volatilityRange * 0.1;
    m_minVolatility = qMax(0.0, m_minVolatility); // Éviter les valeurs négatives pour volatilité et drawdown
    
    double returnRange = m_maxReturn - m_minReturn;
    m_minReturn -= returnRange * 0.1;
    m_maxReturn += returnRange * 0.1;
    
    m_volatilityRange = m_maxVolatility - m_minVolatility;
    m_returnRange = m_maxReturn - m_minReturn;
}

void RiskReturnMapWidget::buildChart()
{
    // Vider la scène
    m_scene->clear();
    
    // Calculer les dimensions du graphique
    calculateChartDimensions();
    
    // Dessiner le fond
    m_scene->setBackgroundBrush(QBrush(QColor(250, 250, 250)));
    
    // Dessiner la grille
    drawGrid();
    
    // Dessiner les axes
    drawAxes();
    
    // Dessiner les lignes de ratio de Sharpe (uniquement en mode ReturnVsVolatility)
    if (m_showSharpeLines && m_currentMode == ReturnVsVolatility) {
        drawSharpeLines();
    }
    
    // Dessiner tous les points
    for (const auto& strategy : m_strategies) {
        addStrategyPoint(strategy);
    }
    
    // Ajuster la vue pour afficher toute la scène
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void RiskReturnMapWidget::drawGrid()
{
    QPen gridPen(QColor(220, 220, 220), 1, Qt::DashLine);
    
    // Lignes horizontales (rendement)
    int numHorizontalLines = 10;
    double returnStep = m_returnRange / numHorizontalLines;
    
    for (int i = 0; i <= numHorizontalLines; i++) {
        double returnValue = m_minReturn + i * returnStep;
        int y = returnToPixel(returnValue);
        
        m_scene->addLine(
            m_chartMarginLeft, y,
            m_chartMarginLeft + m_chartWidth, y,
            gridPen
        );
    }
    
    // Lignes verticales (volatilité/risque)
    int numVerticalLines = 10;
    double volatilityStep = m_volatilityRange / numVerticalLines;
    
    for (int i = 0; i <= numVerticalLines; i++) {
        double volatility = m_minVolatility + i * volatilityStep;
        int x = volatilityToPixel(volatility);
        
        m_scene->addLine(
            x, m_chartMarginTop,
            x, m_chartMarginTop + m_chartHeight,
            gridPen
        );
    }
}

void RiskReturnMapWidget::drawAxes()
{
    // Stylos pour les axes et les étiquettes
    QPen axisPen(Qt::black, 2);
    QFont labelFont = this->font();
    labelFont.setBold(true);
    
    // Dessiner les axes X et Y
    // Axe X (horizontal)
    m_scene->addLine(
        m_chartMarginLeft, m_chartMarginTop + m_chartHeight,
        m_chartMarginLeft + m_chartWidth, m_chartMarginTop + m_chartHeight,
        axisPen
    );
    
    // Axe Y (vertical)
    m_scene->addLine(
        m_chartMarginLeft, m_chartMarginTop,
        m_chartMarginLeft, m_chartMarginTop + m_chartHeight,
        axisPen
    );
    
    // Ajouter les étiquettes d'axe
    QGraphicsTextItem* xAxisLabel = m_scene->addText("");
    xAxisLabel->setFont(labelFont);
    
    QGraphicsTextItem* yAxisLabel = m_scene->addText("Rendement annualisé (%)");
    yAxisLabel->setFont(labelFont);
    
    // Définir le libellé en fonction du mode
    if (m_currentMode == ReturnVsVolatility) {
        xAxisLabel->setPlainText("Volatilité annualisée (%)");
    }
    else if (m_currentMode == ReturnVsDrawdown) {
        xAxisLabel->setPlainText("Drawdown maximal (%)");
    }
    else if (m_currentMode == SharpeVsSortino) {
        xAxisLabel->setPlainText("Ratio de Sharpe");
        yAxisLabel->setPlainText("Ratio de Sortino");
    }
    
    // Positionner les étiquettes
    xAxisLabel->setPos(
        m_chartMarginLeft + m_chartWidth/2 - xAxisLabel->boundingRect().width()/2,
        m_chartMarginTop + m_chartHeight + 10
    );
    
    // Rotation de l'étiquette Y
    yAxisLabel->setRotation(-90);
    yAxisLabel->setPos(
        m_chartMarginLeft - 50,
        m_chartMarginTop + m_chartHeight/2 + yAxisLabel->boundingRect().width()/2
    );
    
    // Ajouter les graduations et valeurs sur les axes
    QFont tickFont = this->font();
    tickFont.setPointSize(8);
    
    // Graduations sur l'axe X (volatilité/risque)
    int numXTicks = 5;
    double xStep = m_volatilityRange / numXTicks;
    
    for (int i = 0; i <= numXTicks; i++) {
        double value = m_minVolatility + i * xStep;
        int x = volatilityToPixel(value);
        
        // Ligne de graduation
        m_scene->addLine(
            x, m_chartMarginTop + m_chartHeight,
            x, m_chartMarginTop + m_chartHeight + 5,
            axisPen
        );
        
        // Valeur
        QString valueStr;
        if (m_currentMode == SharpeVsSortino) {
            valueStr = QString::number(value, 'f', 2);
        } else {
            valueStr = QString::number(value, 'f', 1) + "%";
        }
        
        QGraphicsTextItem* textItem = m_scene->addText(valueStr);
        textItem->setFont(tickFont);
        textItem->setPos(
            x - textItem->boundingRect().width()/2,
            m_chartMarginTop + m_chartHeight + 5
        );
    }
    
    // Graduations sur l'axe Y (rendement)
    int numYTicks = 5;
    double yStep = m_returnRange / numYTicks;
    
    for (int i = 0; i <= numYTicks; i++) {
        double value = m_minReturn + i * yStep;
        int y = returnToPixel(value);
        
        // Ligne de graduation
        m_scene->addLine(
            m_chartMarginLeft - 5, y,
            m_chartMarginLeft, y,
            axisPen
        );
        
        // Valeur
        QString valueStr;
        if (m_currentMode == SharpeVsSortino) {
            valueStr = QString::number(value, 'f', 2);
        } else {
            valueStr = QString::number(value, 'f', 1) + "%";
        }
        
        QGraphicsTextItem* textItem = m_scene->addText(valueStr);
        textItem->setFont(tickFont);
        textItem->setPos(
            m_chartMarginLeft - 10 - textItem->boundingRect().width(),
            y - textItem->boundingRect().height()/2
        );
    }
}

void RiskReturnMapWidget::drawSharpeLines()
{
    // Dessiner les lignes de ratio de Sharpe
    QList<double> sharpeValues = { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5 };
    QStringList sharpeColors = { "#FFAAAA", "#FFCCAA", "#FFFFAA", "#CCFFAA", "#AAFFAA", "#88FF88" };
    
    for (int i = 0; i < sharpeValues.size(); ++i) {
        double sharpe = sharpeValues[i];
        QColor lineColor(sharpeColors[i]);
        QPen sharpePen(lineColor, 1, Qt::DashLine);
        
        // Équation de la ligne de Sharpe: Return = Sharpe * Volatility
        // Tracer la ligne en partant de l'origine (0,0)
        int startX = volatilityToPixel(m_minVolatility);
        int startY = returnToPixel(sharpe * m_minVolatility);
        int endX = volatilityToPixel(m_maxVolatility);
        int endY = returnToPixel(sharpe * m_maxVolatility);
        
        QGraphicsLineItem* sharpeLine = m_scene->addLine(startX, startY, endX, endY, sharpePen);
        
        // Ajouter un label pour la ligne
        QGraphicsTextItem* sharpeLabel = m_scene->addText(QString("Sharpe = %1").arg(sharpe));
        QFont labelFont = sharpeLabel->font();
        labelFont.setPointSize(8);
        sharpeLabel->setFont(labelFont);
        sharpeLabel->setDefaultTextColor(lineColor.darker(150));
        
        // Positionner le label
        double labelX = m_minVolatility + m_volatilityRange * 0.7;
        double labelY = sharpe * labelX;
        
        sharpeLabel->setPos(
            volatilityToPixel(labelX) - sharpeLabel->boundingRect().width() / 2,
            returnToPixel(labelY) - sharpeLabel->boundingRect().height() - 5
        );
        
        // Rotation pour aligner avec la ligne
        double angle = qAtan2(endY - startY, endX - startX) * 180 / M_PI;
        sharpeLabel->setRotation(angle);
    }
}

void RiskReturnMapWidget::addStrategyPoint(const StrategyPoint& point)
{
    // Obtenir les coordonnées selon le mode d'affichage
    double xValue, yValue;
    
    if (m_currentMode == ReturnVsVolatility) {
        xValue = point.volatilityPct;
        yValue = point.returnPct;
    }
    else if (m_currentMode == ReturnVsDrawdown) {
        xValue = point.maxDrawdownPct;
        yValue = point.returnPct;
    }
    else if (m_currentMode == SharpeVsSortino) {
        xValue = point.sharpeRatio;
        yValue = point.sortinoRatio;
    }
    
    int x = volatilityToPixel(xValue);
    int y = returnToPixel(yValue);
    
    // Taille du point selon son importance
    int pointSize = point.isCurrentStrategy ? 14 : 10;
    
    // Créer le point
    QBrush pointBrush(point.color);
    QPen pointPen(Qt::black, point.isCurrentStrategy ? 2 : 1);
    
    QGraphicsEllipseItem* ellipse = m_scene->addEllipse(
        x - pointSize/2, y - pointSize/2,
        pointSize, pointSize,
        pointPen, pointBrush
    );
    
    // Ajouter le nom de la stratégie
    QGraphicsTextItem* nameLabel = m_scene->addText(point.name);
    QFont labelFont = nameLabel->font();
    labelFont.setPointSize(8);
    if (point.isCurrentStrategy) {
        labelFont.setBold(true);
    }
    nameLabel->setFont(labelFont);
    
    // Positionner le texte
    nameLabel->setPos(x + pointSize/2 + 5, y - nameLabel->boundingRect().height()/2);
    
    // Ajouter un tooltip avec des informations détaillées
    QString tooltipText = QString(
        "%1\n"
        "Rendement annualisé: %2%\n"
        "Volatilité annualisée: %3%\n"
        "Ratio de Sharpe: %4\n"
        "Ratio de Sortino: %5\n"
        "Drawdown maximal: %6%"
    ).arg(point.name)
     .arg(point.returnPct, 0, 'f', 2)
     .arg(point.volatilityPct, 0, 'f', 2)
     .arg(point.sharpeRatio, 0, 'f', 2)
     .arg(point.sortinoRatio, 0, 'f', 2)
     .arg(point.maxDrawdownPct, 0, 'f', 2);
    
    ellipse->setToolTip(tooltipText);
}

void RiskReturnMapWidget::updateStrategiesTable()
{
    // Vider le tableau
    m_strategiesModel->setRowCount(0);
    
    // Remplir le tableau avec les stratégies
    for (int i = 0; i < m_strategies.size(); ++i) {
        const StrategyPoint& strategy = m_strategies[i];
        
        // Créer les items pour chaque colonne
        QStandardItem* nameItem = new QStandardItem(strategy.name);
        QStandardItem* returnItem = new QStandardItem(QString::number(strategy.returnPct, 'f', 2));
        QStandardItem* volatilityItem = new QStandardItem(QString::number(strategy.volatilityPct, 'f', 2));
        QStandardItem* sharpeItem = new QStandardItem(QString::number(strategy.sharpeRatio, 'f', 2));
        QStandardItem* maxDDItem = new QStandardItem(QString::number(strategy.maxDrawdownPct, 'f', 2));
        
        // Aligner les valeurs numériques à droite
        returnItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        volatilityItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sharpeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        maxDDItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        // Mettre en évidence la stratégie courante
        if (strategy.isCurrentStrategy) {
            QFont boldFont = nameItem->font();
            boldFont.setBold(true);
            
            nameItem->setFont(boldFont);
            returnItem->setFont(boldFont);
            volatilityItem->setFont(boldFont);
            sharpeItem->setFont(boldFont);
            maxDDItem->setFont(boldFont);
            
            nameItem->setBackground(QBrush(QColor(230, 240, 250)));
            returnItem->setBackground(QBrush(QColor(230, 240, 250)));
            volatilityItem->setBackground(QBrush(QColor(230, 240, 250)));
            sharpeItem->setBackground(QBrush(QColor(230, 240, 250)));
            maxDDItem->setBackground(QBrush(QColor(230, 240, 250)));
        }
        
        // Colorer le fond selon le type de stratégie
        if (strategy.isReferencePoint) {
            nameItem->setBackground(QBrush(QColor(245, 245, 245)));
            returnItem->setBackground(QBrush(QColor(245, 245, 245)));
            volatilityItem->setBackground(QBrush(QColor(245, 245, 245)));
            sharpeItem->setBackground(QBrush(QColor(245, 245, 245)));
            maxDDItem->setBackground(QBrush(QColor(245, 245, 245)));
        }
        
        // Définir un petit carré de couleur dans la première colonne
        nameItem->setData(strategy.color, Qt::DecorationRole);
        
        // Ajouter la ligne au modèle
        m_strategiesModel->setItem(i, 0, nameItem);
        m_strategiesModel->setItem(i, 1, returnItem);
        m_strategiesModel->setItem(i, 2, volatilityItem);
        m_strategiesModel->setItem(i, 3, sharpeItem);
        m_strategiesModel->setItem(i, 4, maxDDItem);
    }
}

double RiskReturnMapWidget::pixelToVolatility(int x) const
{
    return m_minVolatility + (x - m_chartMarginLeft) * m_volatilityRange / m_chartWidth;
}

double RiskReturnMapWidget::pixelToReturn(int y) const
{
    return m_maxReturn - (y - m_chartMarginTop) * m_returnRange / m_chartHeight;
}

int RiskReturnMapWidget::volatilityToPixel(double volatility) const
{
    return m_chartMarginLeft + static_cast<int>((volatility - m_minVolatility) * m_chartWidth / m_volatilityRange);
}

int RiskReturnMapWidget::returnToPixel(double returnValue) const
{
    return m_chartMarginTop + static_cast<int>((m_maxReturn - returnValue) * m_chartHeight / m_returnRange);
}

QColor RiskReturnMapWidget::getPointColor(const StrategyPoint& point)
{
    if (point.isCurrentStrategy) {
        // Stratégie courante - bleu vif
        return QColor(0, 120, 215);
    }
    else if (point.isReferencePoint) {
        // Point de référence - gris
        return QColor(160, 160, 160);
    }
    else {
        // Autres stratégies - utiliser la couleur définie
        return point.color;
    }
}

void RiskReturnMapWidget::showContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Menu contextuel"), this);
    
    QAction* exportAction = new QAction(tr("Exporter l'image..."), this);
    connect(exportAction, &QAction::triggered, this, &RiskReturnMapWidget::exportChartImage);
    contextMenu.addAction(exportAction);
    
    contextMenu.exec(m_view->mapToGlobal(pos));
}

void RiskReturnMapWidget::exportChartImage()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("Exporter l'image"), "", tr("Images (*.png *.jpg *.bmp)"));
        
    if (filePath.isEmpty()) {
        return;
    }
    
    // Assurer que le fichier a une extension
    if (!filePath.contains(".")) {
        filePath += ".png";
    }
    
    // Créer une image de la scène
    QRectF rect = m_scene->itemsBoundingRect();
    QImage image(rect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(), rect);
    painter.end();
    
    // Sauvegarder l'image
    if (image.save(filePath)) {
        QMessageBox::information(this, tr("Exportation réussie"), 
            tr("L'image a été enregistrée sous : %1").arg(filePath));
    } else {
        QMessageBox::warning(this, tr("Erreur d'exportation"), 
            tr("Impossible d'enregistrer l'image."));
    }
}