#include "ui/dialogs/indicators/pivotPointsDialog.h"
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
    // Define groups with high-contrast colors
    m_syncGroups["R"] = {
        false,
        0xFF0000,  // Bright red for resistances
        {
            indicators::PivotPointsInstance::LevelType::R1,
            indicators::PivotPointsInstance::LevelType::R2,
            indicators::PivotPointsInstance::LevelType::R3
        }
    };
    
    m_syncGroups["S"] = {
        false,
        0x008000,  // Dark green for supports
        {
            indicators::PivotPointsInstance::LevelType::S1,
            indicators::PivotPointsInstance::LevelType::S2,
            indicators::PivotPointsInstance::LevelType::S3
        }
    };
    
    m_syncGroups["mR"] = {
        false,
        0xFFA500,  // Bright orange for mid resistance levels
        {
            indicators::PivotPointsInstance::LevelType::M_PR1,
            indicators::PivotPointsInstance::LevelType::M_R1R2,
            indicators::PivotPointsInstance::LevelType::M_R2R3
        }
    };
    
    m_syncGroups["mS"] = {
        false,
        0x0000FF,  // Bright blue for mid support levels
        {
            indicators::PivotPointsInstance::LevelType::M_PS1,
            indicators::PivotPointsInstance::LevelType::M_S1S2,
            indicators::PivotPointsInstance::LevelType::M_S2S3
        }
    };
}

std::string PivotPointsDialog::getLevelGroup(indicators::PivotPointsInstance::LevelType levelType) const
{
    for (const auto& [groupName, group] : m_syncGroups) {
        if (std::find(group.levelTypes.begin(), group.levelTypes.end(), levelType) != group.levelTypes.end()) {
            return groupName;
        }
    }
    return "";  // Level not belonging to any group
}

void PivotPointsDialog::updateSyncButtonsInGroup(const std::string& groupName)
{
    if (m_syncGroups.find(groupName) == m_syncGroups.end()) return;
    
    bool synchronized = m_syncGroups[groupName].synchronized;
    int groupColor = m_syncGroups[groupName].color;
    
    // Extract RGB components of the group color
    int r = (groupColor >> 16) & 0xFF;
    int g = (groupColor >> 8) & 0xFF;
    int b = groupColor & 0xFF;
    
    // Common style for all buttons in the group
    // Always circular shape, but different opacity depending on state
    QString buttonStyle;
    if (synchronized) {
        // Full color when synchronized (100% opacity)
        buttonStyle = QString("QPushButton { background-color: rgb(%1,%2,%3); border: 1px solid darkgray; border-radius: 12px; }")
                     .arg(r).arg(g).arg(b);
    } else {
        // Color with low opacity when not synchronized (20% opacity)
        buttonStyle = QString("QPushButton { background-color: rgba(%1,%2,%3,20%); border: 1px solid darkgray; border-radius: 12px; }")
                     .arg(r).arg(g).arg(b);
    }
    
    // Update all buttons in the group
    for (const auto& levelType : m_syncGroups[groupName].levelTypes) {
        PivotPointsDialog::LevelControls controls = m_levelControls[static_cast<size_t>(levelType)];
        controls.syncButton->blockSignals(true);
        controls.syncButton->setChecked(synchronized);
        controls.syncButton->setStyleSheet(buttonStyle);
        controls.syncButton->blockSignals(false);
    }
}

void PivotPointsDialog::syncGroupControls(const std::string& groupName, indicators::PivotPointsInstance::LevelType sourceLevelType)
{
    if (m_syncGroups.find(groupName) == m_syncGroups.end()) return;

    const auto& sourceStyle = m_currentIndicator.levelStyles[static_cast<size_t>(sourceLevelType)];
    const auto& controls = m_levelControls[static_cast<size_t>(sourceLevelType)];

    bool visible = controls.visibilityCheckBox->isChecked();
    int color = sourceStyle.color;
    int thickness = controls.thicknessSpinBox->value();
    int lineStyleIndex = controls.lineStyleComboBox->currentIndex();

    // Apply to all levels in the group except the source level
    for (indicators::PivotPointsInstance::LevelType levelType : m_syncGroups[groupName].levelTypes) {
        if (levelType == sourceLevelType)
            continue;

        PivotPointsDialog::LevelControls& controls = m_levelControls[static_cast<size_t>(levelType)];
        
        // Update the UI without triggering signals
        controls.visibilityCheckBox->blockSignals(true);
        controls.thicknessSpinBox->blockSignals(true);
        controls.lineStyleComboBox->blockSignals(true);

        controls.visibilityCheckBox->setChecked(visible);
        updateColorButtonStyle(controls.colorButton, color);
        controls.thicknessSpinBox->setValue(thickness);
        controls.lineStyleComboBox->setCurrentIndex(lineStyleIndex);
        
        // Update data
        m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].visible = visible;
        m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].color = color;
        m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].thickness = thickness;
        m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].lineStyle = static_cast<indicators::PivotPointsInstance::LineStyle>(lineStyleIndex);

        // Re-enable signals
        controls.visibilityCheckBox->blockSignals(false);
        controls.thicknessSpinBox->blockSignals(false);
        controls.lineStyleComboBox->blockSignals(false);
    }
}

void PivotPointsDialog::setupUI()
{
    // General settings
    QGroupBox* generalGroupBox = new QGroupBox("General Settings");
    QFormLayout* generalLayout = new QFormLayout(generalGroupBox);
    
    // Combo box for period type
    m_periodTypeComboBox = new QComboBox();
    m_periodTypeComboBox->addItem("4H", static_cast<int>(indicators::PivotPeriodType::FourHour));
    m_periodTypeComboBox->addItem("Daily", static_cast<int>(indicators::PivotPeriodType::Daily));
    m_periodTypeComboBox->addItem("Weekly", static_cast<int>(indicators::PivotPeriodType::Weekly));
    m_periodTypeComboBox->addItem("Monthly", static_cast<int>(indicators::PivotPeriodType::Monthly));
    
    // Select the current period
    generalLayout->addRow("Period Type:", m_periodTypeComboBox);

    m_calculationMethodComboBox = new QComboBox();
    m_calculationMethodComboBox->addItem("High, Low, Close (Standard)", 
                                        static_cast<int>(indicators::PivotCalculationMethod::HLC));
    m_calculationMethodComboBox->addItem("Open, High, Low, Close", 
                                        static_cast<int>(indicators::PivotCalculationMethod::OHLC));
    m_calculationMethodComboBox->addItem("High, Low, Open", 
                                        static_cast<int>(indicators::PivotCalculationMethod::HLO));

    // Select the current calculation method
    generalLayout->addRow("Calculation Method:", m_calculationMethodComboBox);
    

    // Checkbox for showing mid levels
    m_showMidLevelsCheckBox = new QCheckBox();
    m_showMidLevelsCheckBox->setChecked(false); 
    generalLayout->addRow("Show Mid Levels:", m_showMidLevelsCheckBox);
    
    // Checkbox for showing labels
    m_showLabelsCheckBox = new QCheckBox();
    generalLayout->addRow("Show Labels:", m_showLabelsCheckBox);
    
    // Add the general section to the main layout
    m_formLayout->addWidget(generalGroupBox);
    
    // Create a group for all price levels
    QGroupBox* levelsGroupBox = new QGroupBox("Price Levels");
    QGridLayout* levelsLayout = new QGridLayout(levelsGroupBox);
    
    // Headers for the levels grid
    levelsLayout->addWidget(new QLabel("Level"), 0, 0);
    levelsLayout->addWidget(new QLabel("Color"), 0, 1);
    levelsLayout->addWidget(new QLabel("Thickness"), 0, 2);
    levelsLayout->addWidget(new QLabel("Style"), 0, 3);

    // Get the current period suffix
    QString periodSuffix;
    switch (m_currentIndicator.periodType) {
        case indicators::PivotPeriodType::FourHour: periodSuffix = "4H"; break;
        case indicators::PivotPeriodType::Daily: periodSuffix = "D"; break;
        case indicators::PivotPeriodType::Weekly: periodSuffix = "W"; break;
        case indicators::PivotPeriodType::Monthly: periodSuffix = "M"; break;
    }
    
    // Add levels from highest to lowest
    int row = 1;
    
    // Resistances and their mid levels
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::R3, "R3:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_R2R3, "mR3:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::R2, "R2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_R1R2, "mR2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::R1, "R1:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_PR1, "mR1:");

    // Central pivot
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::Pivot, "Piv:");

    // Supports and their mid levels
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_PS1, "mS1:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::S1, "S1:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_S1S2, "mS2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::S2, "S2:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::M_S2S3, "mS3:");
    setupLevelControls(levelsLayout, row++, indicators::PivotPointsInstance::LevelType::S3, "S3:");

    // Make the levels group collapsible
    levelsGroupBox->setLayout(levelsLayout);
    m_formLayout->addWidget(levelsGroupBox);
    
    // Update controls according to current state
    updateLevelControlsState();
}

void PivotPointsDialog::setupLevelControls(QGridLayout* layout, int row, indicators::PivotPointsInstance::LevelType levelType, const QString& labelText)
{
    indicators::PivotPointsInstance::LevelStyle& style = m_currentIndicator.levelStyles[static_cast<size_t>(levelType)];
    
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
    
    // Synchronization button
    QPushButton* syncButton = new QPushButton();
    syncButton->setFixedSize(24, 24);
    syncButton->setCheckable(true);
    
    // Create a lighter sync icon
    QPixmap syncPixmap(20, 20);
    syncPixmap.fill(Qt::transparent);
    QPainter painter(&syncPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Slightly gray background to better see the icon
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(240, 240, 240, 80));
    painter.drawEllipse(1, 1, 18, 18);

    // Draw arrows with better separation
    painter.setPen(QPen(Qt::black, 1.5));

    // First circular arrow (clockwise, first quarter)
    painter.drawArc(3, 3, 14, 14, 0, 120 * 16);
    // Arrow tip for the clockwise arc
    painter.setBrush(Qt::black);
    QPolygonF arrow1;
    arrow1 << QPointF(10 + 7*cos(120*M_PI/180), 10 - 7*sin(120*M_PI/180))  // Tip
        << QPointF(10 + 5*cos(150*M_PI/180), 10 - 5*sin(150*M_PI/180))  // Left side
        << QPointF(10 + 5*cos(100*M_PI/180), 10 - 5*sin(100*M_PI/180)); // Right side
    painter.drawPolygon(arrow1);

    // Second circular arrow (counter-clockwise, first quarter)
    painter.setPen(QPen(Qt::black, 1.5));
    painter.drawArc(3, 3, 14, 14, 180 * 16, 120 * 16);
    // Arrow tip for the counter-clockwise arc
    painter.setBrush(Qt::black);
    QPolygonF arrow2;
    arrow2 << QPointF(10 + 7*cos(300*M_PI/180), 10 - 7*sin(300*M_PI/180))  // Tip
        << QPointF(10 + 5*cos(330*M_PI/180), 10 - 5*sin(330*M_PI/180))  // Left side
        << QPointF(10 + 5*cos(280*M_PI/180), 10 - 5*sin(280*M_PI/180)); // Right side
    painter.drawPolygon(arrow2);

    syncButton->setIcon(QIcon(syncPixmap));
    syncButton->setToolTip("Synchronize with other levels in the group");
    layout->addWidget(syncButton, row, 4);

    if (levelType == indicators::PivotPointsInstance::LevelType::Pivot) {
        // For the pivot point we don't display the sync button
        // so hide it
        syncButton->setVisible(false);
    }
    
    // Store the controls for later use
    m_levelControls[static_cast<size_t>(levelType)] = {
        visibilityCheckBox,
        colorButton,
        thicknessSpinBox,
        lineStyleComboBox,
        syncButton  // Add the sync button
    };
    
    // Apply the initial sync button style according to the group
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty()) {
        int groupColor = m_syncGroups[groupName].color;
        int r = (groupColor >> 16) & 0xFF;
        int g = (groupColor >> 8) & 0xFF;
        int b = groupColor & 0xFF;
        
        // Default color with low opacity (20%)
        QString buttonStyle = QString("QPushButton { background-color: rgba(%1,%2,%3,20%); border: 1px solid darkgray; border-radius: 12px; }")
                             .arg(r).arg(g).arg(b);
        syncButton->setStyleSheet(buttonStyle);
        
        // Tooltip styled with the group's color
        QString tooltipStyle = QString("QToolTip { color: black; background-color: rgb(%1,%2,%3); border: 1px solid black; }")
                              .arg(r).arg(g).arg(b);
        syncButton->setStyleSheet(syncButton->styleSheet() + tooltipStyle);
    }
}

QComboBox* PivotPointsDialog::createLineStyleComboBox()
{
    QComboBox* comboBox = new QComboBox();
    
    // Icon dimensions
    const int width = 80;
    const int height = 20;
    
    // Create an icon for each line style
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
    
    // Add the items with their icons
    comboBox->addItem(QIcon(solidPixmap), "Solid");
    comboBox->addItem(QIcon(dashPixmap), "Dash");
    comboBox->addItem(QIcon(dotPixmap), "Dot");
    comboBox->addItem(QIcon(dotDashPixmap), "DotDash");
    comboBox->addItem(QIcon(altDashPixmap), "AltDash");
    
    // Allow enough space to see the icons
    comboBox->setIconSize(QSize(width, height));
    
    return comboBox;
}

void PivotPointsDialog::updateLevelControlsState()
{
    // Disable the visibility checkbox for mid levels when the shortcut is used
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
        auto& controls = m_levelControls[static_cast<size_t>(levelType)];
        // If the shortcut is active, disable individual modification
        controls.visibilityCheckBox->setEnabled(!shortcutActive);
    }
}

void PivotPointsDialog::updateUIFromInstance()
{
    // Update general controls
    m_periodTypeComboBox->setCurrentIndex(static_cast<int>(m_currentIndicator.periodType));
    m_calculationMethodComboBox->setCurrentIndex(static_cast<int>(m_currentIndicator.calculationMethod));
    m_showLabelsCheckBox->setChecked(m_currentIndicator.showLabels);
    m_showMidLevelsCheckBox->setChecked(false);
    
    // Update controls for each level
    for (size_t levelType = 0; levelType < static_cast<size_t>(indicators::PivotPointsInstance::LevelType::Count); ++levelType) {
        auto& controls = m_levelControls[levelType];
        auto& style = m_currentIndicator.levelStyles[levelType];
        
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
    
    // Update controls state
    updateLevelControlsState();
}

void PivotPointsDialog::connectSignals()
{
    // Connect general controls
    connect(m_periodTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PivotPointsDialog::onPeriodTypeChanged);
    connect(m_calculationMethodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PivotPointsDialog::onCalculationMethodChanged);
    connect(m_showMidLevelsCheckBox, &QCheckBox::checkStateChanged, this, &PivotPointsDialog::onShowMidLevelsChanged);
    connect(m_showLabelsCheckBox, &QCheckBox::checkStateChanged, this, &PivotPointsDialog::onShowLabelsChanged);
    
    // Connect controls for each level
    for (size_t levelType = 0; levelType < static_cast<size_t>(indicators::PivotPointsInstance::LevelType::Count); ++levelType) {
        // Visibility
        auto& controls = m_levelControls[levelType];

        connect(controls.visibilityCheckBox, &QCheckBox::toggled, [this, controls, levelType](bool checked) {
            onLevelVisibilityChanged(static_cast<indicators::PivotPointsInstance::LevelType>(levelType), checked);

            // Update enabled state of other controls
            controls.colorButton->setEnabled(checked);
            controls.thicknessSpinBox->setEnabled(checked);
            controls.lineStyleComboBox->setEnabled(checked);
        });
        
        // Color
        connect(controls.colorButton, &QPushButton::clicked, [this, levelType]() {
            onLevelColorChanged(static_cast<indicators::PivotPointsInstance::LevelType>(levelType));
        });
        
        // Thickness
        connect(controls.thicknessSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, levelType](int value) {
            onLevelThicknessChanged(static_cast<indicators::PivotPointsInstance::LevelType>(levelType), value);
        });
        
        // Line style
        connect(controls.lineStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, levelType](int index) {
            onLevelLineStyleChanged(static_cast<indicators::PivotPointsInstance::LevelType>(levelType), index);
        });

        // Sync button
        connect(controls.syncButton, &QPushButton::toggled, [this, levelType](bool checked) {
            onSyncButtonToggled(static_cast<indicators::PivotPointsInstance::LevelType>(levelType), checked);
        });
    }
}

void PivotPointsDialog::onPeriodTypeChanged(size_t index)
{
    m_currentIndicator.periodType = static_cast<indicators::PivotPeriodType>(index);
    applyChanges();
}

void PivotPointsDialog::onCalculationMethodChanged(size_t index)
{
    m_currentIndicator.calculationMethod = static_cast<indicators::PivotCalculationMethod>(index);
    applyChanges();
}

void PivotPointsDialog::onShowMidLevelsChanged(int state)
{
    // List of mid levels and their dependencies
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

    for (const MidLevelInfo& info : midLevels) {
        bool leftVisible = m_currentIndicator.levelStyles[static_cast<size_t>(info.left)].visible;
        bool rightVisible = m_currentIndicator.levelStyles[static_cast<size_t>(info.right)].visible;
        bool midShouldBeVisible = showMid && leftVisible && rightVisible;

        m_currentIndicator.levelStyles[static_cast<size_t>(info.mid)].visible = midShouldBeVisible;

        // Update checkbox state in the UI
        auto& controls = m_levelControls[static_cast<int>(info.mid)];
        controls.visibilityCheckBox->setChecked(midShouldBeVisible);
    }
    applyChanges();
    updateLevelControlsState();
}

void PivotPointsDialog::onShowLabelsChanged(int state)
{
    m_currentIndicator.showLabels = (state == Qt::Checked);
    applyChanges();
}

void PivotPointsDialog::onLevelVisibilityChanged(indicators::PivotPointsInstance::LevelType levelType, bool checked)
{
    m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].visible = checked;

    // Synchronize if necessary
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
        syncGroupControls(groupName, levelType);
    }

    applyChanges();
}

void PivotPointsDialog::onLevelColorChanged(indicators::PivotPointsInstance::LevelType levelType)
{
    QColor newColor = openColorDialog(m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].color, "Select Level Color");
    if (newColor.isValid()) {
        int colorValue = colorFromRGB(newColor.red(), newColor.green(), newColor.blue());
        m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].color = colorValue;
        updateColorButtonStyle(m_levelControls[static_cast<size_t>(levelType)].colorButton, colorValue);

        // Synchronize if necessary
        std::string groupName = getLevelGroup(levelType);
        if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
            syncGroupControls(groupName, levelType);
        }

        applyChanges();
    }
}

void PivotPointsDialog::onLevelThicknessChanged(indicators::PivotPointsInstance::LevelType levelType, int value)
{
    m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].thickness = value;

    // Synchronize if necessary
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
        syncGroupControls(groupName, levelType);
    }

    applyChanges();
}

void PivotPointsDialog::onLevelLineStyleChanged(indicators::PivotPointsInstance::LevelType levelType, int index)
{    
    // Convert index to line style
    indicators::PivotPointsInstance::LineStyle style = indicators::PivotPointsInstance::LineStyle::Solid;
    switch (index) {
        case 0: style = indicators::PivotPointsInstance::LineStyle::Solid; break;
        case 1: style = indicators::PivotPointsInstance::LineStyle::Dash; break;
        case 2: style = indicators::PivotPointsInstance::LineStyle::Dot; break;
        case 3: style = indicators::PivotPointsInstance::LineStyle::DotDash; break;
        case 4: style = indicators::PivotPointsInstance::LineStyle::AltDash; break;
        default: style = indicators::PivotPointsInstance::LineStyle::Solid; break;
    }

    m_currentIndicator.levelStyles[static_cast<size_t>(levelType)].lineStyle = style;

    // Synchronize if necessary
    std::string groupName = getLevelGroup(levelType);
    if (!groupName.empty() && m_syncGroups[groupName].synchronized) {
        syncGroupControls(groupName, levelType);
    }
    
    applyChanges();
}

void PivotPointsDialog::onSyncButtonToggled(indicators::PivotPointsInstance::LevelType levelType, bool checked)
{
    std::string groupName = getLevelGroup(levelType);
    if (groupName.empty()) return;
    
    // Update group's state
    m_syncGroups[groupName].synchronized = checked;
    
    // Update appearance of all buttons in the group
    updateSyncButtonsInGroup(groupName);
    
    // If synchronization enabled, synchronize controls
    if (checked) {
        syncGroupControls(groupName, levelType);
    }

    applyChanges();  // Apply changes immediately
}