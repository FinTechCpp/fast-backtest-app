#include "dialog/indicatorDialog.h"

IndicatorDialog::IndicatorDialog(QWidget* parent, ChartWidget* chartWidget, const QString& title)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
{
    // Configuration du dialogue
    setWindowTitle(title);
    setMinimumWidth(300);
    setModal(true);
    
    // Layout principal
    m_mainLayout = new QVBoxLayout(this);
    
    // Formulaire de paramètres
    m_formLayout = new QFormLayout();
    m_mainLayout->addLayout(m_formLayout);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_mainLayout->addWidget(m_buttonBox);
    
    // Connexion des boutons standard
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &IndicatorDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &IndicatorDialog::onCancel);
}

IndicatorDialog::~IndicatorDialog()
{
}

void IndicatorDialog::updateColorButtonStyle(QPushButton* button, int color)
{
    int r, g, b;
    getRGBComponents(color, r, g, b);
    
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

QColor IndicatorDialog::openColorDialog(int currentColor, const QString& title)
{
    int r, g, b;
    getRGBComponents(currentColor, r, g, b);
    
    QColor initialColor(r, g, b);
    return QColorDialog::getColor(initialColor, this, title);
}

int IndicatorDialog::colorFromRGB(int r, int g, int b)
{
    return (r << 16) | (g << 8) | b;
}

void IndicatorDialog::getRGBComponents(int color, int& r, int& g, int& b)
{
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;
}

void IndicatorDialog::onApply() {
    applyChanges();
    accept();
}

void IndicatorDialog::onCancel()
{
    cancelChanges();
    reject();
}