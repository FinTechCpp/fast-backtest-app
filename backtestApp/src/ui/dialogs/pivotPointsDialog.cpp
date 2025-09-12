#include "ui/dialogs/pivotPointsDialog.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDebug>
#include <QColorDialog>
#include <QPainter>
#include <QPen>

PivotPointsDialog::PivotPointsDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::PivotPointsInstance& pivotPoints)
    : IndicatorDialog<indicators::PivotPointsInstance>(parent, "Pivot Points", chartWidget, pivotPoints)
{
    initSyncGroups();
    initialize();
}

PivotPointsDialog::~PivotPointsDialog()
{
}

void PivotPointsDialog::initSyncGroups()
{
    // Définir les groupes avec des couleurs à contraste élevé
    m_syncGroups["R"] = {
        false,
        0xFF0000,  // Rouge vif pour résistances
        {
            static_cast<int>(indicators::PivotPointsInstance::LevelType::R1),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::R2),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::R3)
        }
    };
    
    m_syncGroups["S"] = {
        false,
        0x008000,  // Vert foncé pour supports
        {
            static_cast<int>(indicators::PivotPointsInstance::LevelType::S1),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::S2),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::S3)
        }
    };
    
    m_syncGroups["mR"] = {
        false,
        0xFFA500,  // Orange vif pour niveaux milieux résistances
        {
            static_cast<int>(indicators::PivotPointsInstance::LevelType::M_PR1),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::M_R1R2),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::M_R2R3)
        }
    };
    
    m_syncGroups["mS"] = {
        false,
        0x0000FF,  // Bleu vif pour niveaux milieux supports
        {
            static_cast<int>(indicators::PivotPointsInstance::LevelType::M_PS1),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::M_S1S2),
            static_cast<int>(indicators::PivotPointsInstance::LevelType::M_S2S3)
        }
    };
}

std::string PivotPointsDialog::getLevelGroup(int levelType) const
{
    for (const auto& [groupName, group] : m_syncGroups) {
        if (std::find(group.levelTypes.begin(), group.levelTypes.end(), levelType) != group.levelTypes.end()) {
            return groupName;
        }
    }
    return "";  // Niveau n'appartenant à aucun groupe
}

void PivotPointsDialog::updateSyncButtonsInGroup(const std::string& groupName)
{
    if (m_syncGroups.find(groupName) == m_syncGroups.end()) return;
    
    bool synchronized = m_syncGroups[groupName].synchronized;
    int groupColor = m_syncGroups[groupName].color;
    
    // Extraire les composantes RGB de la couleur du groupe
    int r = (groupColor >> 16) & 0xFF;
    int g = (groupColor >> 8) & 0xFF;
    int b = groupColor & 0xFF;
    
    // Style commun pour tous les boutons du groupe
    // Toujours forme circulaire, mais opacité différente selon l'état
    QString buttonStyle;
    if (synchronized) {
        // Couleur pleine quand synchronisé (100% opacité)
        buttonStyle = QString("QPushButton { background-color: rgb(%1,%2,%3); border: 1px solid darkgray; border-radius: 12px; }")
                     .arg(r).arg(g).arg(b);
    } else {
        // Couleur avec faible opacité quand non synchronisé (20% opacité)
        buttonStyle = QString("QPushButton { background-color: rgba(%1,%2,%3,20%); border: 1px solid darkgray; border-radius: 12px; }")
                     .arg(r).arg(g).arg(b);
    }
    
    // Mettre à jour tous les boutons du groupe
    for (int levelType : m_syncGroups[groupName].levelTypes) {
        auto it = m_levelControls.find(levelType);
        if (it != m_levelControls.end()) {
            it->second.syncButton->blockSignals(true);
            it->second.syncButton->setChecked(synchronized);
            it->second.syncButton->setStyleSheet(buttonStyle);
            it->second.syncButton->blockSignals(false);
        }
    }
}

void PivotPointsDialog::syncGroupControls(const std::string& groupName, int sourceLevelType)
{
    if (m_syncGroups.find(groupName) == m_syncGroups.end()) return;
    
    // Obtenir les valeurs de référence du niveau source
    auto sourceIt = m_levelControls.find(sourceLevelType);
    if (sourceIt == m_levelControls.end()) return;
    
    indicators::PivotPointsInstance::LevelType sourceType = static_cast<indicators::PivotPointsInstance::LevelType>(sourceLevelType);
    const auto& sourceStyle = m_currentIndicator.levelStyles[sourceType];
    
    bool visible = sourceIt->second.visibilityCheckBox->isChecked();
    int color = sourceStyle.color;
    int thickness = sourceIt->second.thicknessSpinBox->value();
    int lineStyleIndex = sourceIt->second.lineStyleComboBox->currentIndex();
    
    // Appliquer à tous les niveaux du groupe sauf le niveau source
    for (int levelType : m_syncGroups[groupName].levelTypes) {
        if (levelType != sourceLevelType) {
            auto it = m_levelControls.find(levelType);
            if (it == m_levelControls.end()) continue;
            
            // Mettre à jour l'UI sans déclencher de signaux
            it->second.visibilityCheckBox->blockSignals(true);
            it->second.thicknessSpinBox->blockSignals(true);
            it->second.lineStyleComboBox->blockSignals(true);
            
            it->second.visibilityCheckBox->setChecked(visible);
            updateColorButtonStyle(it->second.colorButton, color);
            it->second.thicknessSpinBox->setValue(thickness);
            it->second.lineStyleComboBox->setCurrentIndex(lineStyleIndex);
            
            // Mettre à jour les données
            indicators::PivotPointsInstance::LevelType type = static_cast<indicators::PivotPointsInstance::LevelType>(levelType);
            m_currentIndicator.levelStyles[type].visible = visible;
            m_currentIndicator.levelStyles[type].color = color;
            m_currentIndicator.levelStyles[type].thickness = thickness;
            m_currentIndicator.levelStyles[type].lineStyle = static_cast<indicators::PivotPointsInstance::LineStyle>(lineStyleIndex);
            
            // Réactiver les signaux
            it->second.visibilityCheckBox->blockSignals(false);
            it->second.thicknessSpinBox->blockSignals(false);
            it->second.lineStyleComboBox->blockSignals(false);
        }
    }
}

void PivotPointsDialog::setupUI()
{
    // Configuration générale
    QGroupBox* generalGroupBox = new QGroupBox("General Settings");
    QFormLayout* generalLayout = new QFormLayout(generalGroupBox);
    
    // Combo box pour le type de période
    m_periodTypeComboBox = new QComboBox();
    m_periodTypeComboBox->addItem("4H", static_cast<int>(indicators::PivotPointsInstance::PeriodType::FourHour));
    m_periodTypeComboBox->addItem("Daily", static_cast<int>(indicators::PivotPointsInstance::PeriodType::Daily));
    m_periodTypeComboBox->addItem("Weekly", static_cast<int>(indicators::PivotPointsInstance::PeriodType::Weekly));
    m_periodTypeComboBox->addItem("Monthly", static_cast<int>(indicators::PivotPointsInstance::PeriodType::Monthly));
    
    // Sélectionner la période actuelle
    generalLayout->addRow("Period Type:", m_periodTypeComboBox);

    m_calculationMethodComboBox = new QComboBox();
    m_calculationMethodComboBox->addItem("High, Low, Close (Standard)", 
                                        static_cast<int>(indicators::PivotPointsInstance::CalculationMethod::HLC));
    m_calculationMethodComboBox->addItem("Open, High, Low, Close", 
                                        static_cast<int>(indicators::PivotPointsInstance::CalculationMethod::OHLC));
    m_calculationMethodComboBox->addItem("High, Low, Open", 
                                        static_cast<int>(indicators::PivotPointsInstance::CalculationMethod::HL0));

    // Sélectionner la méthode de calcul actuelle
    generalLayout->addRow("Calculation Method:", m_calculationMethodComboBox);
    

    // Checkbox pour l'affichage des niveaux milieux
    m_showMidLevelsCheckBox = new QCheckBox();
    m_showMidLevelsCheckBox->setChecked(false); 
    generalLayout->addRow("Show Mid Levels:", m_showMidLevelsCheckBox);
    
    // Checkbox pour l'affichage des étiquettes
    m_showLabelsCheckBox = new QCheckBox();
    generalLayout->addRow("Show Labels:", m_showLabelsCheckBox);
    
    // Ajouter la section générale au layout principal
    m_formLayout->addWidget(generalGroupBox);
    
    // Créer un groupe pour tous les niveaux de prix
    QGroupBox* levelsGroupBox = new QGroupBox("Price Levels");
    QGridLayout* levelsLayout = new QGridLayout(levelsGroupBox);
    
    // En-têtes pour la grille des niveaux
    levelsLayout->addWidget(new QLabel("Level"), 0, 0);
    levelsLayout->addWidget(new QLabel("Color"), 0, 1);
    levelsLayout->addWidget(new QLabel("Thickness"), 0, 2);
    levelsLayout->addWidget(new QLabel("Style"), 0, 3);

    // Obtenir le suffixe de période actuel
    QString periodSuffix;
    switch (m_currentIndicator.periodType) {
        case indicators::PivotPointsInstance::PeriodType::FourHour: periodSuffix = "4H"; break;
        case indicators::PivotPointsInstance::PeriodType::Daily: periodSuffix = "J"; break;
        case indicators::PivotPointsInstance::PeriodType::Weekly: periodSuffix = "S"; break;
        case indicators::PivotPointsInstance::PeriodType::Monthly: periodSuffix = "M"; break;
    }
    
    // Ajouter les niveaux dans l'ordre du plus élevé au plus bas
    int row = 1;
    
    // Résistances et leurs niveaux milieux
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::R3, "R3:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_R2R3, "mR3:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::R2, "R2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_R1R2, "mR2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::R1, "R1:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_PR1, "mR1:");

    // Pivot central
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::Pivot, "Piv:");

    // Supports et leurs niveaux milieux
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_PS1, "mS1:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::S1, "S1:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_S1S2, "mS2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::S2, "S2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_S2S3, "mS3:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::S3, "S3:");

    // Rendre le groupe des niveaux déroulable
    levelsGroupBox->setLayout(levelsLayout);
    m_formLayout->addWidget(levelsGroupBox);
    
    // Mise à jour des contrôles selon l'état actuel
    updateLevelControlsState();
}

void PivotPointsDialog::setupLevelControls(QGridLayout* layout, int row, indicators::PivotPointsInstance::LevelType levelType, const QString& labelText)
{
    int levelTypeInt = static_cast<int>(levelType);
    auto& style = m_currentIndicator.levelStyles[levelType];
    
    // Visibility checkbox
    QCheckBox* visibilityCheckBox = new QCheckBox(labelText);
    layout->addWidget(visibilityCheckBox, row, 0);
    
    // Color button
    QPushButton* colorButton = new QPushButton();
    layout->addWidget(colorButton, row, 1);
    
    // Thickness spinbox
    QSpinBox* thicknessSpinBox = new QSpinBox();
    thicknessSpinBox->setRange(1, 5);
    layout->addWidget(thicknessSpinBox, row, 2);
    
    // Line style combobox
    QComboBox* lineStyleComboBox = createLineStyleComboBox();
    layout->addWidget(lineStyleComboBox, row, 3);
    
    // Bouton de synchronisation
    QPushButton* syncButton = new QPushButton();
    syncButton->setFixedSize(24, 24);
    syncButton->setCheckable(true);
    
    // Créer une icône plus claire de synchronisation
    QPixmap syncPixmap(20, 20);
    syncPixmap.fill(Qt::transparent);
    QPainter painter(&syncPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Fond légèrement grisé pour mieux voir l'icône
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(240, 240, 240, 80));
    painter.drawEllipse(1, 1, 18, 18);

    // Dessiner les flèches avec une meilleure séparation
    painter.setPen(QPen(Qt::black, 1.5));

    // Première flèche circulaire (sens horaire, premier quart de cercle)
    painter.drawArc(3, 3, 14, 14, 0, 120 * 16);
    // Pointe de flèche pour l'arc horaire
    painter.setBrush(Qt::black);
    QPolygonF arrow1;
    arrow1 << QPointF(10 + 7*cos(120*M_PI/180), 10 - 7*sin(120*M_PI/180))  // Pointe
        << QPointF(10 + 5*cos(150*M_PI/180), 10 - 5*sin(150*M_PI/180))  // Côté gauche
        << QPointF(10 + 5*cos(100*M_PI/180), 10 - 5*sin(100*M_PI/180)); // Côté droit
    painter.drawPolygon(arrow1);

    // Seconde flèche circulaire (sens anti-horaire, premier quart de cercle)
    painter.setPen(QPen(Qt::black, 1.5));
    painter.drawArc(3, 3, 14, 14, 180 * 16, 120 * 16);
    // Pointe de flèche pour l'arc anti-horaire
    painter.setBrush(Qt::black);
    QPolygonF arrow2;
    arrow2 << QPointF(10 + 7*cos(300*M_PI/180), 10 - 7*sin(300*M_PI/180))  // Pointe
        << QPointF(10 + 5*cos(330*M_PI/180), 10 - 5*sin(330*M_PI/180))  // Côté gauche
        << QPointF(10 + 5*cos(280*M_PI/180), 10 - 5*sin(280*M_PI/180)); // Côté droit
    painter.drawPolygon(arrow2);

    syncButton->setIcon(QIcon(syncPixmap));
    syncButton->setToolTip("Synchroniser avec les autres niveaux du groupe");
    layout->addWidget(syncButton, row, 4);

    if (levelType == indicators::PivotPointsInstance::LevelType::Pivot) {
        // pour le point pivot on n'affiche pas le bouton de synchronisation
        // il faut donc le cacher
        syncButton->setVisible(false);
    }
    
    // Stocker les contrôles pour les utiliser plus tard
    m_levelControls[levelTypeInt] = {
        visibilityCheckBox,
        colorButton,
        thicknessSpinBox,
        lineStyleComboBox,
        syncButton  // Ajouter le bouton sync
    };
    
    // Appliquer le style initial du bouton sync selon le groupe
    std::string groupName = getLevelGroup(levelTypeInt);
    if (!groupName.empty()) {
        int groupColor = m_syncGroups[groupName].color;
        int r = (groupColor >> 16) & 0xFF;
        int g = (groupColor >> 8) & 0xFF;
        int b = groupColor & 0xFF;
        
        // Couleur avec faible opacité par défaut (20%)
        QString buttonStyle = QString("QPushButton { background-color: rgba(%1,%2,%3,20%); border: 1px solid darkgray; border-radius: 12px; }")
                             .arg(r).arg(g).arg(b);
        syncButton->setStyleSheet(buttonStyle);
        
        // Tooltip avec la couleur du groupe
        QString tooltipStyle = QString("QToolTip { color: black; background-color: rgb(%1,%2,%3); border: 1px solid black; }")
                              .arg(r).arg(g).arg(b);
        syncButton->setStyleSheet(syncButton->styleSheet() + tooltipStyle);
    }
}

QComboBox* PivotPointsDialog::createLineStyleComboBox()
{
    QComboBox* comboBox = new QComboBox();
    
    // Dimensions de l'icône
    const int width = 80;
    const int height = 20;
    
    // Créer une icône pour chaque style de ligne
    // 1. Solid
    QPixmap solidPixmap(width, height);
    solidPixmap.fill(Qt::transparent);
    QPainter solidPainter(&solidPixmap);
    QPen solidPen(Qt::black, 2, Qt::SolidLine);
    solidPainter.setPen(solidPen);
    solidPainter.drawLine(5, height/2, width-5, height/2);
    
    // 2. Dash
    QPixmap dashPixmap(width, height);
    dashPixmap.fill(Qt::transparent);
    QPainter dashPainter(&dashPixmap);
    QPen dashPen(Qt::black, 2, Qt::DashLine);
    dashPainter.setPen(dashPen);
    dashPainter.drawLine(5, height/2, width-5, height/2);
    
    // 3. Dot
    QPixmap dotPixmap(width, height);
    dotPixmap.fill(Qt::transparent);
    QPainter dotPainter(&dotPixmap);
    QPen dotPen(Qt::black, 2, Qt::DotLine);
    dotPainter.setPen(dotPen);
    dotPainter.drawLine(5, height/2, width-5, height/2);
    
    // 4. DotDash
    QPixmap dotDashPixmap(width, height);
    dotDashPixmap.fill(Qt::transparent);
    QPainter dotDashPainter(&dotDashPixmap);
    QPen dotDashPen(Qt::black, 2, Qt::DashDotLine);
    dotDashPainter.setPen(dotDashPen);
    dotDashPainter.drawLine(5, height/2, width-5, height/2);
    
    // 5. AltDash
    QPixmap altDashPixmap(width, height);
    altDashPixmap.fill(Qt::transparent);
    QPainter altDashPainter(&altDashPixmap);
    QPen altDashPen(Qt::black, 2, Qt::DashDotDotLine);
    altDashPainter.setPen(altDashPen);
    altDashPainter.drawLine(5, height/2, width-5, height/2);
    
    // Ajouter les items avec leurs icônes
    comboBox->addItem(QIcon(solidPixmap), "Solid");
    comboBox->addItem(QIcon(dashPixmap), "Dash");
    comboBox->addItem(QIcon(dotPixmap), "Dot");
    comboBox->addItem(QIcon(dotDashPixmap), "DotDash");
    comboBox->addItem(QIcon(altDashPixmap), "AltDash");
    
    // Permettre suffisamment d'espace pour voir les icônes
    comboBox->setIconSize(QSize(width, height));
    
    return comboBox;
}

void PivotPointsDialog::updateLevelControlsState()
{
    // Désactiver le checkbox de visibilité pour les milieux si on utilise le raccourci
    bool shortcutActive = m_showMidLevelsCheckBox->isChecked();
    std::vector<indicators::PivotPointsInstance::LevelType> midLevels = {
        indicators::PivotPointsInstance::LevelType::M_PR1,
        indicators::PivotPointsInstance::LevelType::M_R1R2,
        indicators::PivotPointsInstance::LevelType::M_R2R3,
        indicators::PivotPointsInstance::LevelType::M_PS1,
        indicators::PivotPointsInstance::LevelType::M_S1S2,
        indicators::PivotPointsInstance::LevelType::M_S2S3
    };
    for (auto levelType : midLevels) {
        int levelTypeInt = static_cast<int>(levelType);
        auto it = m_levelControls.find(levelTypeInt);
        if (it != m_levelControls.end()) {
            // Si le raccourci est actif, on désactive la modification individuelle
            it->second.visibilityCheckBox->setEnabled(!shortcutActive);
        }
    }
}

void PivotPointsDialog::updateUIFromInstance()
{
    // Mettre à jour les contrôles généraux
    m_periodTypeComboBox->setCurrentIndex(static_cast<int>(m_currentIndicator.periodType));
    m_calculationMethodComboBox->setCurrentIndex(static_cast<int>(m_currentIndicator.calculationMethod));
    m_showLabelsCheckBox->setChecked(m_currentIndicator.showLabels);
    m_showMidLevelsCheckBox->setChecked(false);
    
    // Mettre à jour les contrôles pour chaque niveau
    for (auto& [levelType, controls] : m_levelControls) {
        indicators::PivotPointsInstance::LevelType type = static_cast<indicators::PivotPointsInstance::LevelType>(levelType);
        auto& style = m_currentIndicator.levelStyles[type];
        
        controls.visibilityCheckBox->setChecked(style.visible);
        updateColorButtonStyle(controls.colorButton, style.color);
        controls.thicknessSpinBox->setValue(style.thickness > 0 ? style.thickness : 1);
        
        int styleIndex = 0;
        switch (style.lineStyle) {
            case indicators::PivotPointsInstance::LineStyle::Solid: styleIndex = 0; break;
            case indicators::PivotPointsInstance::LineStyle::Dash: styleIndex = 1; break;
            case indicators::PivotPointsInstance::LineStyle::Dot: styleIndex = 2; break;
            case indicators::PivotPointsInstance::LineStyle::DotDash: styleIndex = 3; break;
            case indicators::PivotPointsInstance::LineStyle::AltDash: styleIndex = 4; break;
            default: styleIndex = 0; break;
        }
        controls.lineStyleComboBox->setCurrentIndex(styleIndex);
    }
    
    // Mettre à jour l'état des contrôles
    updateLevelControlsState();
}

void PivotPointsDialog::connectSignals()
{
    // Connecter les contrôles généraux
    connect(m_periodTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PivotPointsDialog::onPeriodTypeChanged);
    connect(m_calculationMethodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PivotPointsDialog::onCalculationMethodChanged);
    connect(m_showMidLevelsCheckBox, &QCheckBox::checkStateChanged, this, &PivotPointsDialog::onShowMidLevelsChanged);
    connect(m_showLabelsCheckBox, &QCheckBox::checkStateChanged, this, &PivotPointsDialog::onShowLabelsChanged);
    
    // Connecter les contrôles pour chaque niveau
    for (auto& [levelType, controls] : m_levelControls) {
        // Visibilité
        connect(controls.visibilityCheckBox, &QCheckBox::toggled, [this, levelType](bool checked) {
            onLevelVisibilityChanged(levelType, checked);
            
            // Mettre à jour l'état d'activation des autres contrôles
            auto it = m_levelControls.find(levelType);
            if (it != m_levelControls.end()) {
                it->second.colorButton->setEnabled(checked);
                it->second.thicknessSpinBox->setEnabled(checked);
                it->second.lineStyleComboBox->setEnabled(checked);
            }
        });
        
        // Couleur
        connect(controls.colorButton, &QPushButton::clicked, [this, levelType]() {
            onLevelColorChanged(levelType);
        });
        
        // Épaisseur
        connect(controls.thicknessSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, levelType](int value) {
            onLevelThicknessChanged(levelType, value);
        });
        
        // Style de ligne
        connect(controls.lineStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, levelType](int index) {
            onLevelLineStyleChanged(levelType, index);
        });

        // Bouton de synchronisation
        connect(controls.syncButton, &QPushButton::toggled, [this, levelType](bool checked) {
            onSyncButtonToggled(levelType, checked);
        });
    }
}

void PivotPointsDialog::onPeriodTypeChanged(int index)
{
    m_currentIndicator.periodType = static_cast<indicators::PivotPointsInstance::PeriodType>(index);
    applyChanges();
}

void PivotPointsDialog::onCalculationMethodChanged(int index)
{
    m_currentIndicator.calculationMethod = static_cast<indicators::PivotPointsInstance::CalculationMethod>(index);
    applyChanges();
}

void PivotPointsDialog::onShowMidLevelsChanged(int state)
{
    // Liste des niveaux milieux et leurs dépendances
    struct MidLevelInfo {
        indicators::PivotPointsInstance::LevelType mid;
        indicators::PivotPointsInstance::LevelType left;
        indicators::PivotPointsInstance::LevelType right;
    };
    std::vector<MidLevelInfo> midLevels = {
        {indicators::PivotPointsInstance::LevelType::M_PR1, indicators::PivotPointsInstance::LevelType::Pivot, indicators::PivotPointsInstance::LevelType::R1},
        {indicators::PivotPointsInstance::LevelType::M_R1R2, indicators::PivotPointsInstance::LevelType::R1, indicators::PivotPointsInstance::LevelType::R2},
        {indicators::PivotPointsInstance::LevelType::M_R2R3, indicators::PivotPointsInstance::LevelType::R2, indicators::PivotPointsInstance::LevelType::R3},
        {indicators::PivotPointsInstance::LevelType::M_PS1, indicators::PivotPointsInstance::LevelType::Pivot, indicators::PivotPointsInstance::LevelType::S1},
        {indicators::PivotPointsInstance::LevelType::M_S1S2, indicators::PivotPointsInstance::LevelType::S1, indicators::PivotPointsInstance::LevelType::S2},
        {indicators::PivotPointsInstance::LevelType::M_S2S3, indicators::PivotPointsInstance::LevelType::S2, indicators::PivotPointsInstance::LevelType::S3}
    };

    bool showMid = (state == Qt::Checked);

    for (const auto& info : midLevels) {
        bool leftVisible = m_currentIndicator.levelStyles[info.left].visible;
        bool rightVisible = m_currentIndicator.levelStyles[info.right].visible;
        bool midShouldBeVisible = showMid && leftVisible && rightVisible;

        m_currentIndicator.levelStyles[info.mid].visible = midShouldBeVisible;

        // Met à jour l'état du checkbox dans l'UI
        int midTypeInt = static_cast<int>(info.mid);
        auto it = m_levelControls.find(midTypeInt);
        if (it != m_levelControls.end()) {
            it->second.visibilityCheckBox->setChecked(midShouldBeVisible);
        }
    }
    applyChanges();
    updateLevelControlsState();
}

void PivotPointsDialog::onShowLabelsChanged(int state)
{
    m_currentIndicator.showLabels = (state == Qt::Checked);
    applyChanges();
}

void PivotPointsDialog::onLevelVisibilityChanged(int levelType, bool checked)
{
    indicators::PivotPointsInstance::LevelType type = static_cast<indicators::PivotPointsInstance::LevelType>(levelType);
    m_currentIndicator.levelStyles[type].visible = checked;

    // Synchroniser si nécessaire
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
        syncGroupControls(groupName, levelType);
    }

    applyChanges();
}

void PivotPointsDialog::onLevelColorChanged(int levelType)
{
    indicators::PivotPointsInstance::LevelType type = static_cast<indicators::PivotPointsInstance::LevelType>(levelType);
    QColor newColor = openColorDialog(m_currentIndicator.levelStyles[type].color, "Select Level Color");
    if (newColor.isValid()) {
        int colorValue = colorFromRGB(newColor.red(), newColor.green(), newColor.blue());
        m_currentIndicator.levelStyles[type].color = colorValue;
        updateColorButtonStyle(m_levelControls[levelType].colorButton, colorValue);

        // Synchroniser si nécessaire
        std::string groupName = getLevelGroup(levelType);
        if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
            syncGroupControls(groupName, levelType);
        }

        applyChanges();
    }
}

void PivotPointsDialog::onLevelThicknessChanged(int levelType, int value)
{
    indicators::PivotPointsInstance::LevelType type = static_cast<indicators::PivotPointsInstance::LevelType>(levelType);
    m_currentIndicator.levelStyles[type].thickness = value;

    // Synchroniser si nécessaire
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
        syncGroupControls(groupName, levelType);
    }

    applyChanges();
}

void PivotPointsDialog::onLevelLineStyleChanged(int levelType, int index)
{
    indicators::PivotPointsInstance::LevelType type = static_cast<indicators::PivotPointsInstance::LevelType>(levelType);
    
    // Convertir l'index en style de ligne Qt
    indicators::PivotPointsInstance::LineStyle style = indicators::PivotPointsInstance::LineStyle::Solid;
    switch (index) {
        case 0: style = indicators::PivotPointsInstance::LineStyle::Solid; break;
        case 1: style = indicators::PivotPointsInstance::LineStyle::Dash; break;
        case 2: style = indicators::PivotPointsInstance::LineStyle::Dot; break;
        case 3: style = indicators::PivotPointsInstance::LineStyle::DotDash; break;
        case 4: style = indicators::PivotPointsInstance::LineStyle::AltDash; break;
        default: style = indicators::PivotPointsInstance::LineStyle::Solid; break;
    }

    m_currentIndicator.levelStyles[type].lineStyle = style;
    
    // Synchroniser si nécessaire
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
        syncGroupControls(groupName, levelType);
    }
    
    applyChanges();
}

void PivotPointsDialog::onSyncButtonToggled(int levelType, bool checked)
{
    std::string groupName = getLevelGroup(levelType);
    if (groupName.empty()) return;
    
    // Mettre à jour l'état du groupe
    m_syncGroups[groupName].synchronized = checked;
    
    // Mettre à jour l'apparence de tous les boutons du groupe
    updateSyncButtonsInGroup(groupName);
    
    // Si synchronisation activée, synchroniser les contrôles
    if (checked) {
        syncGroupControls(groupName, levelType);
    }

    applyChanges();  // Appliquer les changements immédiatement
}