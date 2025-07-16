#include "ui/dialogs/pivotPointsDialog.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDebug>
#include <QColorDialog>
#include <QPainter>
#include <QPen>

PivotPointsDialog::PivotPointsDialog(QWidget* parent, ChartWidget* chartWidget, const PivotPointsInstance& pivotPoints)
    : IndicatorDialog<PivotPointsInstance>(parent, "Pivot Points", chartWidget, pivotPoints)
{
    // setMinimumWidth(500);
    setupUI();
    connectSignals();
}

PivotPointsDialog::~PivotPointsDialog()
{
}

void PivotPointsDialog::setupUI()
{
    // Configuration générale
    QGroupBox* generalGroupBox = new QGroupBox("General Settings");
    QFormLayout* generalLayout = new QFormLayout(generalGroupBox);
    
    // Combo box pour le type de période
    m_periodTypeComboBox = new QComboBox();
    m_periodTypeComboBox->addItem("4H", static_cast<int>(PivotPointsInstance::PeriodType::FourHour));
    m_periodTypeComboBox->addItem("Daily", static_cast<int>(PivotPointsInstance::PeriodType::Daily));
    m_periodTypeComboBox->addItem("Weekly", static_cast<int>(PivotPointsInstance::PeriodType::Weekly));
    m_periodTypeComboBox->addItem("Monthly", static_cast<int>(PivotPointsInstance::PeriodType::Monthly));
    
    // Sélectionner la période actuelle
    m_periodTypeComboBox->setCurrentIndex(static_cast<int>(m_currentIndicator.periodType));
    generalLayout->addRow("Period Type:", m_periodTypeComboBox);

    m_calculationMethodComboBox = new QComboBox();
    m_calculationMethodComboBox->addItem("High, Low, Close (Standard)", 
                                        static_cast<int>(PivotPointsInstance::CalculationMethod::HLC));
    m_calculationMethodComboBox->addItem("Open, High, Low, Close", 
                                        static_cast<int>(PivotPointsInstance::CalculationMethod::OHLC));
    m_calculationMethodComboBox->addItem("High, Low, Open", 
                                        static_cast<int>(PivotPointsInstance::CalculationMethod::HL0));

    // Sélectionner la méthode de calcul actuelle
    m_calculationMethodComboBox->setCurrentIndex(static_cast<int>(m_currentIndicator.calculationMethod));
    generalLayout->addRow("Calculation Method:", m_calculationMethodComboBox);
    

    // Checkbox pour l'affichage des niveaux milieux
    m_showMidLevelsCheckBox = new QCheckBox();
    m_showMidLevelsCheckBox->setChecked(false); 
    generalLayout->addRow("Show Mid Levels:", m_showMidLevelsCheckBox);
    
    // Checkbox pour l'affichage des étiquettes
    m_showLabelsCheckBox = new QCheckBox();
    m_showLabelsCheckBox->setChecked(m_currentIndicator.showLabels);
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
        case PivotPointsInstance::PeriodType::FourHour: periodSuffix = "4H"; break;
        case PivotPointsInstance::PeriodType::Daily: periodSuffix = "J"; break;
        case PivotPointsInstance::PeriodType::Weekly: periodSuffix = "S"; break;
        case PivotPointsInstance::PeriodType::Monthly: periodSuffix = "M"; break;
    }
    
    // Ajouter les niveaux dans l'ordre du plus élevé au plus bas
    int row = 1;
    
    // Résistances et leurs niveaux milieux
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::R3, "R3:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_R2R3, "mR3:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::R2, "R2:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_R1R2, "mR2:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::R1, "R1:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_PR1, "mR1:");

    // Pivot central
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::Pivot, "Piv:");

    // Supports et leurs niveaux milieux
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_PS1, "mS1:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::S1, "S1:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_S1S2, "mS2:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::S2, "S2:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_S2S3, "mS3:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::S3, "S3:");

    // Rendre le groupe des niveaux déroulable
    levelsGroupBox->setLayout(levelsLayout);
    m_formLayout->addWidget(levelsGroupBox);
    
    // Mise à jour des contrôles selon l'état actuel
    updateLevelControlsState();
}

void PivotPointsDialog::setupLevelControls(QGridLayout* layout, int row, PivotPointsInstance::LevelType levelType, const QString& labelText)
{
    int levelTypeInt = static_cast<int>(levelType);
    auto& style = m_currentIndicator.levelStyles[levelType];
    
    // Labels
    // layout->addWidget(new QLabel(labelText), row, 0);
    
    // Visibility checkbox
    QCheckBox* visibilityCheckBox = new QCheckBox(labelText);
    visibilityCheckBox->setChecked(style.visible);
    layout->addWidget(visibilityCheckBox, row, 0);
    
    // Color button
    QPushButton* colorButton = new QPushButton();
    updateColorButtonStyle(colorButton, style.color);
    layout->addWidget(colorButton, row, 1);
    
    // Thickness spinbox
    QSpinBox* thicknessSpinBox = new QSpinBox();
    thicknessSpinBox->setRange(1, 5);
    thicknessSpinBox->setValue(style.thickness > 0 ? style.thickness : 1);
    layout->addWidget(thicknessSpinBox, row, 2);
    
    // Line style combobox
    QComboBox* lineStyleComboBox = createLineStyleComboBox();
    int styleIndex = 0;
    switch (style.lineStyle) {
        case PivotPointsInstance::LineStyle::Solid: styleIndex = 0; break;
        case PivotPointsInstance::LineStyle::Dash: styleIndex = 1; break;
        case PivotPointsInstance::LineStyle::Dot: styleIndex = 2; break;
        case PivotPointsInstance::LineStyle::DotDash: styleIndex = 3; break;
        case PivotPointsInstance::LineStyle::AltDash: styleIndex = 4; break;
        default: styleIndex = 0; break;
    }
    lineStyleComboBox->setCurrentIndex(styleIndex);
    layout->addWidget(lineStyleComboBox, row, 3);
    
    // Stocker les contrôles pour les utiliser plus tard
    m_levelControls[levelTypeInt] = {
        visibilityCheckBox,
        colorButton,
        thicknessSpinBox,
        lineStyleComboBox
    };
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
    std::vector<PivotPointsInstance::LevelType> midLevels = {
        PivotPointsInstance::LevelType::M_PR1,
        PivotPointsInstance::LevelType::M_R1R2,
        PivotPointsInstance::LevelType::M_R2R3,
        PivotPointsInstance::LevelType::M_PS1,
        PivotPointsInstance::LevelType::M_S1S2,
        PivotPointsInstance::LevelType::M_S2S3
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
    
    // Déterminer si des niveaux milieux sont visibles
    bool anyMidVisible = false;
    std::vector<PivotPointsInstance::LevelType> midLevels = {
        PivotPointsInstance::LevelType::M_PR1,
        PivotPointsInstance::LevelType::M_R1R2,
        PivotPointsInstance::LevelType::M_R2R3,
        PivotPointsInstance::LevelType::M_PS1,
        PivotPointsInstance::LevelType::M_S1S2,
        PivotPointsInstance::LevelType::M_S2S3
    };
    for (auto level : midLevels) {
        if (m_currentIndicator.levelStyles[level].visible) {
            anyMidVisible = true;
            break;
        }
    }
    m_showMidLevelsCheckBox->setChecked(anyMidVisible);
    
    // Mettre à jour les contrôles pour chaque niveau
    for (auto& [levelType, controls] : m_levelControls) {
        PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
        auto& style = m_currentIndicator.levelStyles[type];
        
        controls.visibilityCheckBox->setChecked(style.visible);
        updateColorButtonStyle(controls.colorButton, style.color);
        controls.thicknessSpinBox->setValue(style.thickness);
        
        int styleIndex = 0;
        switch (style.lineStyle) {
            case PivotPointsInstance::LineStyle::Solid: styleIndex = 0; break;
            case PivotPointsInstance::LineStyle::Dash: styleIndex = 1; break;
            case PivotPointsInstance::LineStyle::Dot: styleIndex = 2; break;
            case PivotPointsInstance::LineStyle::DotDash: styleIndex = 3; break;
            case PivotPointsInstance::LineStyle::AltDash: styleIndex = 4; break;
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
    }
}

void PivotPointsDialog::onPeriodTypeChanged(int index)
{
    m_currentIndicator.periodType = static_cast<PivotPointsInstance::PeriodType>(index);
    applyChanges();
}

void PivotPointsDialog::onCalculationMethodChanged(int index)
{
    m_currentIndicator.calculationMethod = static_cast<PivotPointsInstance::CalculationMethod>(index);
    applyChanges();
}

void PivotPointsDialog::onShowMidLevelsChanged(int state)
{
    // Liste des niveaux milieux et leurs dépendances
    struct MidLevelInfo {
        PivotPointsInstance::LevelType mid;
        PivotPointsInstance::LevelType left;
        PivotPointsInstance::LevelType right;
    };
    std::vector<MidLevelInfo> midLevels = {
        {PivotPointsInstance::LevelType::M_PR1, PivotPointsInstance::LevelType::Pivot, PivotPointsInstance::LevelType::R1},
        {PivotPointsInstance::LevelType::M_R1R2, PivotPointsInstance::LevelType::R1, PivotPointsInstance::LevelType::R2},
        {PivotPointsInstance::LevelType::M_R2R3, PivotPointsInstance::LevelType::R2, PivotPointsInstance::LevelType::R3},
        {PivotPointsInstance::LevelType::M_PS1, PivotPointsInstance::LevelType::Pivot, PivotPointsInstance::LevelType::S1},
        {PivotPointsInstance::LevelType::M_S1S2, PivotPointsInstance::LevelType::S1, PivotPointsInstance::LevelType::S2},
        {PivotPointsInstance::LevelType::M_S2S3, PivotPointsInstance::LevelType::S2, PivotPointsInstance::LevelType::S3}
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
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    m_currentIndicator.levelStyles[type].visible = checked;
    applyChanges();
}

void PivotPointsDialog::onLevelColorChanged(int levelType)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    QColor newColor = openColorDialog(m_currentIndicator.levelStyles[type].color, "Select Level Color");
    if (newColor.isValid()) {
        int colorValue = colorFromRGB(newColor.red(), newColor.green(), newColor.blue());
        m_currentIndicator.levelStyles[type].color = colorValue;
        updateColorButtonStyle(m_levelControls[levelType].colorButton, colorValue);
        applyChanges();
    }
}

void PivotPointsDialog::onLevelThicknessChanged(int levelType, int value)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    m_currentIndicator.levelStyles[type].thickness = value;
    applyChanges();
}

void PivotPointsDialog::onLevelLineStyleChanged(int levelType, int index)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    
    // Convertir l'index en style de ligne Qt
    PivotPointsInstance::LineStyle style = PivotPointsInstance::LineStyle::Solid;
    switch (index) {
        case 0: style = PivotPointsInstance::LineStyle::Solid; break;
        case 1: style = PivotPointsInstance::LineStyle::Dash; break;
        case 2: style = PivotPointsInstance::LineStyle::Dot; break;
        case 3: style = PivotPointsInstance::LineStyle::DotDash; break;
        case 4: style = PivotPointsInstance::LineStyle::AltDash; break;
        default: style = PivotPointsInstance::LineStyle::Solid; break;
    }
    
    m_currentIndicator.levelStyles[type].lineStyle = style;
    applyChanges();
}
