#include "ui/views/Stats/RatioGaugeWidget.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QStyleOption>
#include <QLinearGradient>
#include <QPen>
#include <QBrush>
#include <cmath>
#include <QDebug>
#include <QResizeEvent>

RatioGaugeWidget::RatioGaugeWidget(const QVector<GaugeZone>& zones, const QString& title, const QString& explanation, QWidget* parent)
    : QWidget(parent),
      m_value(0.0),
      m_minValue(0.0),
      m_maxValue(1.0),
      m_precision(2),
      m_addPercentageSign(false),
      m_gaugeHeight(50),
      m_hasValue(false),
      m_fillDirection(FillDirection::LeftToRight),
      m_referenceValue(NAN),
      m_showReference(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(50);
    setupUI();

    m_zones = zones;
    m_metricTitle = title;
    m_baseExplanation = explanation;
    
    // Mettre à jour le nom de la métrique affiché
    m_metricNameLabel->setText(title + ": ");
    
    // Déterminer les valeurs min/max
    m_minValue = m_zones.first().minValue;
    m_maxValue = m_zones.last().maxValue;
    
    updateGauge();
    updateExplanation();
}

void RatioGaugeWidget::setupUI()
{
    // Layout principal
    m_mainLayout = new QGridLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(8);
    
    // Étiquette pour le nom de la métrique
    m_metricNameLabel = new QLabel("Ratio");
    QFont nameFont = m_metricNameLabel->font();
    nameFont.setPointSize(14);
    m_metricNameLabel->setFont(nameFont);
    m_metricNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_metricNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Étiquette pour la valeur actuelle
    m_valueLabel = new QLabel("N/A");
    QFont valueFont = m_valueLabel->font();
    valueFont.setPointSize(14);
    valueFont.setBold(true);
    m_valueLabel->setFont(valueFont);
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Widget personnalisé pour la jauge
    m_gaugeWidget = new FillGaugeRenderWidget(this);
    m_gaugeWidget->setFixedHeight(m_gaugeHeight);
    m_gaugeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Ajout dans le layout grille
    m_mainLayout->addWidget(m_metricNameLabel, 0, 0);
    m_mainLayout->addWidget(m_valueLabel,     0, 1);
    m_mainLayout->addWidget(m_gaugeWidget,    0, 2);

    // Proportions précises: 10% / 5% / 85%
    m_mainLayout->setColumnStretch(0, 12);
    m_mainLayout->setColumnStretch(1, 8);
    m_mainLayout->setColumnStretch(2, 68);
}

void RatioGaugeWidget::setValue(double value)
{
    m_value = value;
    m_hasValue = true;
    updateGauge();
    updateExplanation();
}

double RatioGaugeWidget::value() const
{
    return m_value;
}

void RatioGaugeWidget::setValueFormat(unsigned int precision, bool addPercentageSign)
{
    m_precision = precision;
    m_addPercentageSign = addPercentageSign;
    
    updateGauge();
}

void RatioGaugeWidget::setReferenceValue(double referenceValue)
{
    m_referenceValue = referenceValue;
    m_showReference = !std::isnan(referenceValue);
    updateGauge();
}

void RatioGaugeWidget::showReference(bool show)
{
    m_showReference = show;
    updateGauge();
}

void RatioGaugeWidget::setFillDirection(FillDirection direction)
{
    m_fillDirection = direction;
    updateGauge();
}

void RatioGaugeWidget::clear()
{
    m_value = 0.0;
    m_hasValue = false;
    
    updateGauge();
    updateExplanation();
}

void RatioGaugeWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // Mettre à jour les largeurs minimales des colonnes
    if (m_mainLayout) {
        m_mainLayout->setColumnMinimumWidth(0, width() * 0.10);
        m_mainLayout->setColumnMinimumWidth(1, width() * 0.05);
    }
    
    // Mise à jour de la jauge
    updateGauge();
}

void RatioGaugeWidget::paintEvent(QPaintEvent* event)
{
    // Dessiner le fond du widget
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    
    QWidget::paintEvent(event);
}

void RatioGaugeWidget::updateExplanation()
{
    // Trouver la zone actuelle pour ajouter une explication spécifique
    QString zoneExplanation = "";
    
    for (const auto& zone : m_zones) {
        if (m_value >= zone.minValue && m_value <= zone.maxValue) {
            zoneExplanation = QString("<br><b>Interprétation :</b> %1").arg(zone.description);
            break;
        }
    }

    QString tooltipText = m_baseExplanation + zoneExplanation;

    // Appliquer le tooltip aux widgets principaux
    this->setToolTip(tooltipText);
    m_gaugeWidget->setToolTip(tooltipText);
    m_metricNameLabel->setToolTip(tooltipText);
    m_valueLabel->setToolTip(tooltipText);
}

// Surcharge de la fonction de mise à jour de la jauge pour utiliser le widget spécialisé
void RatioGaugeWidget::updateGauge()
{    
    // Configurer le widget de rendu
    m_gaugeWidget->setZones(m_zones);
    m_gaugeWidget->setRange(m_minValue, m_maxValue);
    m_gaugeWidget->setFillDirection(m_fillDirection);
    m_gaugeWidget->setReferenceValue(m_referenceValue);
    m_gaugeWidget->showReference(m_showReference);

    // Ne pas afficher le curseur s'il n'y a pas de valeur définie
    if (m_hasValue) {
        m_gaugeWidget->setValue(m_value);
        // Mettre à jour uniquement la valeur, pas le nom de la métrique
        m_valueLabel->setText(QString::number(m_value, 'f', m_precision) + (m_addPercentageSign ? "%" : ""));
        
        // Trouver la zone actuelle
        QString currentZoneDesc = "Unknown";
        QColor valueColor = QColor(0, 0, 0);
        bool inRange = false;
        
        for (const auto& zone : m_zones) {
            if (m_value >= zone.minValue && m_value <= zone.maxValue ||
                (m_value >= zone.maxValue && m_value <= zone.minValue)) {
                currentZoneDesc = zone.description;
                valueColor = zone.labelColor;
                inRange = true;
                break;
            }
        }

        if (!inRange && m_value < m_minValue) {
            currentZoneDesc = m_zones.first().description;
            valueColor = m_zones.first().labelColor;
        } else if (!inRange && m_value > m_maxValue) {
            currentZoneDesc = m_zones.last().description;
            valueColor = m_zones.last().labelColor;
        }

        m_valueLabel->setStyleSheet(QString("color: rgb(%1, %2, %3);")
                .arg(valueColor.red())
                .arg(valueColor.green())
                .arg(valueColor.blue()));
    } else {
        // Réinitialiser l'affichage
        // m_gaugeWidget->setShowCursor(false);
        m_valueLabel->setText("N/A");
    }

    updateExplanation();
}