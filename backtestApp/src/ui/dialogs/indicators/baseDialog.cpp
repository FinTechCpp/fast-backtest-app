#include "ui/dialogs/indicators/baseDialog.h"

BaseDialog::BaseDialog(QWidget* parent, const QString& title)
    : QDialog(parent)
{
    // Dialog configuration
    setWindowTitle(title);
    setMinimumWidth(450);
    setModal(true);
    
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    
    // Parameters form
    m_formLayout = new QFormLayout();
    m_mainLayout->addLayout(m_formLayout);

    // Add the reset link at the top
    m_resetLink = new QLabel("<a href=\"#\">Reset to defaults</a>");
    m_resetLink->setAlignment(Qt::AlignRight);
    m_resetLink->setCursor(Qt::PointingHandCursor);
    connect(m_resetLink, &QLabel::linkActivated, this, &BaseDialog::onReset);
    m_mainLayout->addWidget(m_resetLink);
    
    // Cancel/Apply buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_mainLayout->addWidget(m_buttonBox);
    
    // Connect standard buttons
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &BaseDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &BaseDialog::onCancel);
}

BaseDialog::~BaseDialog()
{
}

void BaseDialog::updateColorButtonStyle(QPushButton* button, int color)
{
    int r, g, b;
    getRGBComponents(color, r, g, b);
    
    QString styleSheet = QString("background-color: rgb(%1, %2, %3); ")
                          .arg(r).arg(g).arg(b);
                          
    // Adjust text for readability on the background color
    int brightness = (r * 299 + g * 587 + b * 114) / 1000;
    if (brightness > 125) {
        styleSheet += "color: black;";
    } else {
        styleSheet += "color: white;";
    }
    
    button->setStyleSheet(styleSheet);
    button->setText(QString("#%1").arg(color, 6, 16, QChar('0')));
}

QColor BaseDialog::openColorDialog(int currentColor, const QString& title)
{
    int r, g, b;
    getRGBComponents(currentColor, r, g, b);
    
    QColor initialColor(r, g, b);
    return QColorDialog::getColor(initialColor, this, title);
}

int BaseDialog::colorFromRGB(int r, int g, int b)
{
    return (r << 16) | (g << 8) | b;
}

void BaseDialog::getRGBComponents(int color, int& r, int& g, int& b)
{
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;
}

void BaseDialog::onApply() {
    applyChanges();
    accept();
}

void BaseDialog::onCancel()
{
    cancelChanges();
    reject();
}

void BaseDialog::onReset()
{
    resetToDefaults();
}