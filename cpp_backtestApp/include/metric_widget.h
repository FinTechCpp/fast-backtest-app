#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>

// Enum pour les statuts des métriques (pour la coloration)
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
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(10);
        
        m_labelWidget = new QLabel(label);
        m_valueWidget = new QLabel(value);
        
        m_labelWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_valueWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        // Augmenter la taille de police pour les deux labels
        QFont labelFont = m_labelWidget->font();
        labelFont.setPointSize(labelFont.pointSize() + 1); // Augmente de 1 point
        m_labelWidget->setFont(labelFont);
        
        // Police en gras et plus grande pour la valeur
        QFont valueFont = m_valueWidget->font();
        valueFont.setBold(true);
        valueFont.setPointSize(valueFont.pointSize() + 2); // Augmente de 2 points
        m_valueWidget->setFont(valueFont);
        
        layout->addWidget(m_labelWidget);
        layout->addStretch(1); // Ajoute un espace extensible entre le label et la valeur
        layout->addWidget(m_valueWidget);
        
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