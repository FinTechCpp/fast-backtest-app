#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>

enum class MetricStatus {
    Good,   // Valeur positive/bonne (vert)
    Neutral, // Valeur neutre (noir)
    Bad,    // Valeur négative/mauvaise (rouge)
    NA      // Valeur non disponible/non applicable (gris)
};

class MetricWidget : public QWidget
{
    Q_OBJECT

public:
    MetricWidget(const QString& label, const QString& value = "N/A", QWidget* parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(5);
        
        m_labelWidget = new QLabel(label);
        m_valueWidget = new QLabel(value);
        
        m_labelWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_valueWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        // Police en gras pour la valeur
        QFont valueFont = m_valueWidget->font();
        valueFont.setBold(true);
        m_valueWidget->setFont(valueFont);
        
        layout->addWidget(m_labelWidget, 1);
        layout->addWidget(m_valueWidget, 1);
        
        setLayout(layout);
    }
    
    void updateValues(const QString& value, MetricStatus status = MetricStatus::Neutral) {
        m_valueWidget->setText(value);
        
        // Appliquer le style CSS en fonction du statut
        QString styleSheet;
        
        switch (status) {
            case MetricStatus::Good:
                styleSheet = "QLabel { color: #2ecc71; }"; // Vert
                break;
            case MetricStatus::Bad:
                styleSheet = "QLabel { color: #e74c3c; }"; // Rouge
                break;
            case MetricStatus::NA:
                styleSheet = "QLabel { color: #7f8c8d; font-style: italic; }"; // Gris
                break;
            case MetricStatus::Neutral:
            default:
                styleSheet = "QLabel { color: black; }"; // Noir par défaut
                break;
        }
        
        m_valueWidget->setStyleSheet(styleSheet);
    }
    
    void setTooltip(const QString& tooltip) {
        setToolTip(tooltip);
    }

private:
    QLabel* m_labelWidget;
    QLabel* m_valueWidget;
};