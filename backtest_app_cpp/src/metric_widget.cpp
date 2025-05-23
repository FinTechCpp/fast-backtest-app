#include "metric_widget.h"
#include <QFont>
#include <QDebug>

MetricWidget::MetricWidget(const QString& title, const QString& value,
                           const QString& delta, const QString& delta_color,
                           QWidget* parent)
    : QFrame(parent)
{
    // Configuration du cadre
    setFrameShape(QFrame::StyledPanel);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    // Création du layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    
    // Titre
    m_titleLabel = new QLabel(title, this);
    QFont titleFont;
    titleFont.setPointSize(10);
    m_titleLabel->setFont(titleFont);
    
    // Valeur
    m_valueLabel = new QLabel(value, this);
    QFont valueFont;
    valueFont.setPointSize(12);
    valueFont.setBold(true);
    m_valueLabel->setFont(valueFont);
    
    // Delta (variation)
    m_deltaLabel = new QLabel(delta, this);
    QFont deltaFont;
    deltaFont.setPointSize(10);
    m_deltaLabel->setFont(deltaFont);
    
    // Définir la couleur du delta
    if (delta_color == "normal") {
        m_deltaLabel->setStyleSheet("color: green");
    } else if (delta_color == "inverse") {
        m_deltaLabel->setStyleSheet("color: red");
    }
    
    // Ajout des widgets au layout
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_valueLabel);
    if (!delta.isEmpty()) {
        layout->addWidget(m_deltaLabel);
    } else {
        m_deltaLabel->setVisible(false);
    }
}

void MetricWidget::updateValues(const QString& value, const QString& delta,
                                const QString& delta_color)
{
    // Mettre à jour la valeur
    m_valueLabel->setText(value);
    
    // Mettre à jour le delta s'il est fourni
    if (!delta.isEmpty()) {
        m_deltaLabel->setText(delta);
        m_deltaLabel->setVisible(true);
        
        // Mettre à jour la couleur du delta
        if (delta_color == "normal") {
            m_deltaLabel->setStyleSheet("color: green");
        } else if (delta_color == "inverse") {
            m_deltaLabel->setStyleSheet("color: red");
        }
    } else {
        m_deltaLabel->setVisible(false);
    }
}