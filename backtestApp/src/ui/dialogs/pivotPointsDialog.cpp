#include "ui/dialogs/pivotPointsDialog.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDebug>

PivotPointsDialog::PivotPointsDialog(QWidget* parent, ChartWidget* chartWidget, int pivotId, const PivotPointsInstance& pivotPoints)
    : BaseDialog(parent, chartWidget, "Configure Pivot Points")
    , m_pivotId(pivotId)
    , m_originalPivots(pivotPoints)
    , m_currentPivots(pivotPoints)
{
    // S'assurer que tous les niveaux sont définis avec des valeurs par défaut
    using LT = PivotPointsInstance::LevelType;
    if (m_currentPivots.levelStyles.empty()) {
        // Définir des styles par défaut pour le pivot central (rouge)
        PivotPointsInstance::LevelStyle pivotStyle;
        pivotStyle.color = 0xCC0000;  // Rouge
        pivotStyle.thickness = 2;
        pivotStyle.lineStyle = Qt::SolidLine;
        pivotStyle.visible = true;
        pivotStyle.labelFormat = "PP: %.2f";
        m_currentPivots.levelStyles[LT::Pivot] = pivotStyle;
        
        // Styles pour les résistances (vert)
        PivotPointsInstance::LevelStyle r1Style = pivotStyle;
        r1Style.color = 0x009900;  // Vert
        r1Style.thickness = 1;
        r1Style.labelFormat = "R1: %.2f";
        m_currentPivots.levelStyles[LT::R1] = r1Style;
        
        PivotPointsInstance::LevelStyle r2Style = r1Style;
        r2Style.lineStyle = Qt::DashLine;
        r2Style.labelFormat = "R2: %.2f";
        m_currentPivots.levelStyles[LT::R2] = r2Style;
        
        PivotPointsInstance::LevelStyle r3Style = r1Style;
        r3Style.lineStyle = Qt::DotLine;
        r3Style.visible = false;
        r3Style.labelFormat = "R3: %.2f";
        m_currentPivots.levelStyles[LT::R3] = r3Style;
        
        // Styles pour les supports (bleu)
        PivotPointsInstance::LevelStyle s1Style = pivotStyle;
        s1Style.color = 0x0066CC;  // Bleu
        s1Style.thickness = 1;
        s1Style.labelFormat = "S1: %.2f";
        m_currentPivots.levelStyles[LT::S1] = s1Style;
        
        PivotPointsInstance::LevelStyle s2Style = s1Style;
        s2Style.lineStyle = Qt::DashLine;
        s2Style.labelFormat = "S2: %.2f";
        m_currentPivots.levelStyles[LT::S2] = s2Style;
        
        PivotPointsInstance::LevelStyle s3Style = s1Style;
        s3Style.lineStyle = Qt::DotLine;
        s3Style.visible = false;
        s3Style.labelFormat = "S3: %.2f";
        m_currentPivots.levelStyles[LT::S3] = s3Style;
        
        // Styles pour les niveaux milieux (gris)
        PivotPointsInstance::LevelStyle midStyle;
        midStyle.color = 0x888888;  // Gris
        midStyle.thickness = 1;
        midStyle.lineStyle = Qt::DotLine;
        midStyle.visible = true;
        
        midStyle.labelFormat = "M(P-R1): %.2f";
        m_currentPivots.levelStyles[LT::M_PR1] = midStyle;
        
        midStyle.labelFormat = "M(R1-R2): %.2f";
        m_currentPivots.levelStyles[LT::M_R1R2] = midStyle;
        
        midStyle.labelFormat = "M(R2-R3): %.2f";
        m_currentPivots.levelStyles[LT::M_R2R3] = midStyle;
        
        midStyle.labelFormat = "M(P-S1): %.2f";
        m_currentPivots.levelStyles[LT::M_PS1] = midStyle;
        
        midStyle.labelFormat = "M(S1-S2): %.2f";
        m_currentPivots.levelStyles[LT::M_S1S2] = midStyle;
        
        midStyle.labelFormat = "M(S2-S3): %.2f";
        m_currentPivots.levelStyles[LT::M_S2S3] = midStyle;
    }
    
    // Configurer le type de période par défaut si non défini
    if (m_currentPivots.periodType == PivotPointsInstance::PeriodType::Daily) {
        m_currentPivots.showMidLevels = false;
        m_currentPivots.showLabels = true;
    }
    
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
    m_periodTypeComboBox->addItem("Daily", static_cast<int>(PivotPointsInstance::PeriodType::Daily));
    m_periodTypeComboBox->addItem("Weekly", static_cast<int>(PivotPointsInstance::PeriodType::Weekly));
    m_periodTypeComboBox->addItem("Monthly", static_cast<int>(PivotPointsInstance::PeriodType::Monthly));
    m_periodTypeComboBox->addItem("Quarterly", static_cast<int>(PivotPointsInstance::PeriodType::Quarterly));
    m_periodTypeComboBox->addItem("Yearly", static_cast<int>(PivotPointsInstance::PeriodType::Yearly));
    
    // Sélectionner la période actuelle
    m_periodTypeComboBox->setCurrentIndex(static_cast<int>(m_currentPivots.periodType));
    generalLayout->addRow("Period Type:", m_periodTypeComboBox);
    
    // Checkbox pour l'affichage des niveaux milieux
    m_showMidLevelsCheckBox = new QCheckBox();
    m_showMidLevelsCheckBox->setChecked(m_currentPivots.showMidLevels);
    generalLayout->addRow("Show Mid Levels:", m_showMidLevelsCheckBox);
    
    // Checkbox pour l'affichage des étiquettes
    m_showLabelsCheckBox = new QCheckBox();
    m_showLabelsCheckBox->setChecked(m_currentPivots.showLabels);
    generalLayout->addRow("Show Labels:", m_showLabelsCheckBox);
    
    // Ajouter la section générale au layout principal
    m_mainLayout->addWidget(generalGroupBox);
    
    // Créer un groupe pour tous les niveaux de prix
    QGroupBox* levelsGroupBox = new QGroupBox("Price Levels");
    QGridLayout* levelsLayout = new QGridLayout(levelsGroupBox);
    
    // En-têtes pour la grille des niveaux
    levelsLayout->addWidget(new QLabel("Level"), 0, 0);
    levelsLayout->addWidget(new QLabel("Visible"), 0, 1);
    levelsLayout->addWidget(new QLabel("Color"), 0, 2);
    levelsLayout->addWidget(new QLabel("Thickness"), 0, 3);
    levelsLayout->addWidget(new QLabel("Style"), 0, 4);
    
    // Ajouter les niveaux dans l'ordre du plus élevé au plus bas
    int row = 1;
    
    // Résistances et leurs niveaux milieux
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::R3, "Resistance 3 (R3):");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_R2R3, "Mid R2-R3:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::R2, "Resistance 2 (R2):");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_R1R2, "Mid R1-R2:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::R1, "Resistance 1 (R1):");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_PR1, "Mid PP-R1:");
    
    // Pivot central
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::Pivot, "Pivot Point (PP):");
    
    // Supports et leurs niveaux milieux
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_PS1, "Mid PP-S1:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::S1, "Support 1 (S1):");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_S1S2, "Mid S1-S2:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::S2, "Support 2 (S2):");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::M_S2S3, "Mid S2-S3:");
    setupLevelControls(levelsLayout, row++, PivotPointsInstance::LevelType::S3, "Support 3 (S3):");
    
    // Rendre le groupe des niveaux déroulable
    levelsGroupBox->setLayout(levelsLayout);
    m_mainLayout->addWidget(levelsGroupBox);
    
    // Mise à jour des contrôles selon l'état actuel
    updateLevelControlsState();
}

void PivotPointsDialog::setupLevelControls(QGridLayout* layout, int row, PivotPointsInstance::LevelType levelType, const QString& labelText)
{
    int levelTypeInt = static_cast<int>(levelType);
    auto& style = m_currentPivots.levelStyles[levelType];
    
    // Labels
    layout->addWidget(new QLabel(labelText), row, 0);
    
    // Visibility checkbox
    QCheckBox* visibilityCheckBox = new QCheckBox("Visible");
    visibilityCheckBox->setChecked(style.visible);
    layout->addWidget(visibilityCheckBox, row, 1);
    
    // Color button
    QPushButton* colorButton = new QPushButton();
    updateColorButtonStyle(colorButton, style.color);
    layout->addWidget(colorButton, row, 2);
    
    // Thickness spinbox
    QSpinBox* thicknessSpinBox = new QSpinBox();
    thicknessSpinBox->setRange(1, 5);
    thicknessSpinBox->setValue(style.thickness > 0 ? style.thickness : 1);
    layout->addWidget(thicknessSpinBox, row, 3);
    
    // Line style combobox
    QComboBox* lineStyleComboBox = createLineStyleComboBox();
    int styleIndex = 0;
    switch (style.lineStyle) {
        case Qt::SolidLine: styleIndex = 0; break;
        case Qt::DashLine: styleIndex = 1; break;
        case Qt::DotLine: styleIndex = 2; break;
        case Qt::DashDotLine: styleIndex = 3; break;
        default: styleIndex = 0; break;
    }
    lineStyleComboBox->setCurrentIndex(styleIndex);
    layout->addWidget(lineStyleComboBox, row, 4);
    
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
    comboBox->addItem("Solid");
    comboBox->addItem("Dash");
    comboBox->addItem("Dot");
    comboBox->addItem("DashDot");
    return comboBox;
}

void PivotPointsDialog::updateLevelControlsState()
{
    // Mettre à jour la visibilité des niveaux milieux selon l'option showMidLevels
    bool showMid = m_currentPivots.showMidLevels;
    
    // Liste des niveaux milieux
    std::vector<PivotPointsInstance::LevelType> midLevels = {
        PivotPointsInstance::LevelType::M_PR1,
        PivotPointsInstance::LevelType::M_R1R2,
        PivotPointsInstance::LevelType::M_R2R3,
        PivotPointsInstance::LevelType::M_PS1,
        PivotPointsInstance::LevelType::M_S1S2,
        PivotPointsInstance::LevelType::M_S2S3
    };
    
    // Activer/désactiver les contrôles pour les niveaux milieux
    for (auto levelType : midLevels) {
        int levelTypeInt = static_cast<int>(levelType);
        auto it = m_levelControls.find(levelTypeInt);
        if (it != m_levelControls.end()) {
            it->second.visibilityCheckBox->setEnabled(showMid);
            it->second.colorButton->setEnabled(showMid && it->second.visibilityCheckBox->isChecked());
            it->second.thicknessSpinBox->setEnabled(showMid && it->second.visibilityCheckBox->isChecked());
            it->second.lineStyleComboBox->setEnabled(showMid && it->second.visibilityCheckBox->isChecked());
        }
    }
}

void PivotPointsDialog::connectSignals()
{
    // Connecter les contrôles généraux
    connect(m_periodTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PivotPointsDialog::onPeriodTypeChanged);
    connect(m_showMidLevelsCheckBox, &QCheckBox::stateChanged, this, &PivotPointsDialog::onShowMidLevelsChanged);
    connect(m_showLabelsCheckBox, &QCheckBox::stateChanged, this, &PivotPointsDialog::onShowLabelsChanged);
    
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
    m_currentPivots.periodType = static_cast<PivotPointsInstance::PeriodType>(index);
    applyChanges();
}

void PivotPointsDialog::onShowMidLevelsChanged(int state)
{
    m_currentPivots.showMidLevels = (state == Qt::Checked);
    updateLevelControlsState();
    applyChanges();
}

void PivotPointsDialog::onShowLabelsChanged(int state)
{
    m_currentPivots.showLabels = (state == Qt::Checked);
    applyChanges();
}

void PivotPointsDialog::onLevelVisibilityChanged(int levelType, bool checked)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    m_currentPivots.levelStyles[type].visible = checked;
    applyChanges();
}

void PivotPointsDialog::onLevelColorChanged(int levelType)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    QColor newColor = openColorDialog(m_currentPivots.levelStyles[type].color, "Select Level Color");
    if (newColor.isValid()) {
        int colorValue = colorFromRGB(newColor.red(), newColor.green(), newColor.blue());
        m_currentPivots.levelStyles[type].color = colorValue;
        updateColorButtonStyle(m_levelControls[levelType].colorButton, colorValue);
        applyChanges();
    }
}

void PivotPointsDialog::onLevelThicknessChanged(int levelType, int value)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    m_currentPivots.levelStyles[type].thickness = value;
    applyChanges();
}

void PivotPointsDialog::onLevelLineStyleChanged(int levelType, int index)
{
    PivotPointsInstance::LevelType type = static_cast<PivotPointsInstance::LevelType>(levelType);
    
    // Convertir l'index en style de ligne Qt
    Qt::PenStyle style = Qt::SolidLine;
    switch (index) {
        case 0: style = Qt::SolidLine; break;
        case 1: style = Qt::DashLine; break;
        case 2: style = Qt::DotLine; break;
        case 3: style = Qt::DashDotLine; break;
        default: style = Qt::SolidLine; break;
    }
    
    m_currentPivots.levelStyles[type].lineStyle = style;
    applyChanges();
}

void PivotPointsDialog::applyChanges()
{
    // Appliquer les changements au graphique
    m_chartWidget->updateIndicator(m_currentPivots);
}

void PivotPointsDialog::cancelChanges()
{
    // Restaurer l'instance originale
    m_chartWidget->updateIndicator(m_originalPivots);
}