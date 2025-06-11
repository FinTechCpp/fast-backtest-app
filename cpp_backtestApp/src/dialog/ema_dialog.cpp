#include "dialog/ema_dialog.h"


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
    
    headerLayout->addWidget(enabledLabel);
    headerLayout->addWidget(periodLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(colorLabel);
    
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
    
    // Initialiser les 5 EMA fixés
    m_currentEMAs.clear();
    
    // Couleurs prédéfinies pour les 5 EMA
    static const int colors[] = {
        0x0000FF,  // Bleu
        0xFF0000,  // Rouge
        0x00AA00,  // Vert
        0xAA00AA,  // Violet
        0xFF8800,  // Orange
    };
    
    // Créer 5 EMA fixes
    for (int i = 0; i < 5; i++) {
        ChartWidget::EMAInstance ema;
        ema.id = -1 - i;  // ID temporaire négatif
        ema.period = 20;  // Période par défaut
        ema.visible = (i == 0);  // Seul le premier est activé par défaut
        ema.color = colors[i];
        m_currentEMAs.push_back(ema);
    }
    
    // Mettre à jour avec les EMA existants
    for (const auto& existingEma : emaInstances) {
        // Trouver un emplacement libre (index < 5)
        for (size_t i = 0; i < m_currentEMAs.size() && i < 5; i++) {
            if (!m_currentEMAs[i].visible) {
                // Utiliser cet emplacement pour l'EMA existant
                m_currentEMAs[i].period = existingEma.period;
                m_currentEMAs[i].visible = existingEma.visible;
                m_currentEMAs[i].color = existingEma.color;
                break;
            }
        }
    }
    
    // Créer les lignes pour les 5 EMA
    refreshEMAList();
    
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
    periodSpinBox->setEnabled(ema.visible); // Désactiver si l'EMA n'est pas visible
    connect(periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this, row](int value) {
        this->onPeriodChanged(row, value);
    });
    
    // Connecter le checkbox à l'état d'activation du spinbox
    connect(enabledCheckBox, &QCheckBox::toggled, periodSpinBox, &QSpinBox::setEnabled);
    
    // Bouton de sélection de couleur
    QPushButton* colorButton = new QPushButton();
    updateColorButtonStyle(colorButton, ema.color);
    colorButton->setFixedWidth(80);
    colorButton->setEnabled(ema.visible); // Désactiver si l'EMA n'est pas visible
    connect(colorButton, &QPushButton::clicked, [this, row]() {
        this->onColorButtonClicked(row);
    });
    
    // Connecter le checkbox à l'état d'activation du bouton de couleur
    connect(enabledCheckBox, &QCheckBox::toggled, colorButton, &QPushButton::setEnabled);
    
    // Ajouter les widgets au layout
    layout->addWidget(enabledCheckBox);
    layout->addWidget(periodSpinBox);
    layout->addStretch();
    layout->addWidget(colorButton);
    
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
    
    // Recréer les 5 lignes fixes
    m_rowToEMAId.clear();
    
    for (int row = 0; row < 5; ++row) {
        // Assurer qu'il y a 5 EMA dans m_currentEMAs
        if (row >= static_cast<int>(m_currentEMAs.size())) {
            ChartWidget::EMAInstance ema;
            ema.id = -1 - row;
            ema.period = 20;
            ema.visible = false;
            ema.color = 0x0000FF;
            m_currentEMAs.push_back(ema);
        }
        
        QWidget* rowWidget = createEMARow(m_currentEMAs[row], row);
        m_emaListLayout->addWidget(rowWidget);
        m_rowToEMAId[row] = m_currentEMAs[row].id;
    }
}

void EMADialog::onEnabledStateChanged(int row, bool enabled)
{
    if (row < 0 || row >= static_cast<int>(m_currentEMAs.size())) return;
    
    // Mettre à jour l'état de visibilité de l'EMA
    m_currentEMAs[row].visible = enabled;
    
    // Mettre à jour les contrôles dans l'interface si nécessaire
    QWidget* rowWidget = m_emaListLayout->itemAt(row)->widget();
    if (rowWidget) {
        QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(rowWidget->layout());
        if (layout) {
            // Activer/désactiver les contrôles de période et couleur
            if (layout->count() > 1 && layout->itemAt(1)->widget()) {
                layout->itemAt(1)->widget()->setEnabled(enabled);
            }
            if (layout->count() > 3 && layout->itemAt(3)->widget()) {
                layout->itemAt(3)->widget()->setEnabled(enabled);
            }
        }
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
    int row = static_cast<int>(m_currentEMAs.size() - 1);
    QWidget* rowWidget = createEMARow(newEMA, row);
    m_emaListLayout->addWidget(rowWidget);
    m_rowToEMAId[row] = newEMA.id;
    
    m_nextRowId++;
}

void EMADialog::onApply()
{
    // 1. Supprimer tous les EMA existants
    for (const auto& ema : m_originalEMAs) {
        m_chartWidget->removeEMA(ema.id);
    }
    
    // 2. Ajouter uniquement les EMA activés de la liste courante
    for (const auto& ema : m_currentEMAs) {
        if (ema.visible) {
            int id = m_chartWidget->addEMA(ema.period);
            m_chartWidget->setEMAConfig(id, ema);
            // m_chartWidget->setEMAVisible(id, true);
            // m_chartWidget->setEMAColor(id, ema.color);
        }
    }
    
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
        m_chartWidget->setEMAConfig(id, ema);
        // m_chartWidget->setEMAVisible(id, ema.visible);
        // m_chartWidget->setEMAColor(id, ema.color);
    }
    
    // 4. Fermer la boîte de dialogue
    reject();
}