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
    
    // Update displayed metric name
    m_metricNameLabel->setText(title + ": ");
    
    // Determine min/max values
    m_minValue = m_zones.first().minValue;
    m_maxValue = m_zones.last().maxValue;
    
    updateGauge();
    updateExplanation();
}

void RatioGaugeWidget::setupUI()
{
    // Main layout
    m_mainLayout = new QGridLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(8);
    
    // Label for metric name
    m_metricNameLabel = new QLabel("Ratio");
    QFont nameFont = m_metricNameLabel->font();
    nameFont.setPointSize(14);
    m_metricNameLabel->setFont(nameFont);
    m_metricNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_metricNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Label for current value
    m_valueLabel = new QLabel("N/A");
    QFont valueFont = m_valueLabel->font();
    valueFont.setPointSize(14);
    valueFont.setBold(true);
    m_valueLabel->setFont(valueFont);
    m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Custom widget for the gauge
    m_gaugeWidget = new FillGaugeRenderWidget(this);
    m_gaugeWidget->setFixedHeight(m_gaugeHeight);
    m_gaugeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Add to grid layout
    m_mainLayout->addWidget(m_metricNameLabel, 0, 0);
    m_mainLayout->addWidget(m_valueLabel,     0, 1);
    m_mainLayout->addWidget(m_gaugeWidget,    0, 2);

    // Exact proportions: 10% / 5% / 85%
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
    
    // Update column minimum widths
    if (m_mainLayout) {
        m_mainLayout->setColumnMinimumWidth(0, width() * 0.10);
        m_mainLayout->setColumnMinimumWidth(1, width() * 0.05);
    }
    
    // Update the gauge
    updateGauge();
}

void RatioGaugeWidget::paintEvent(QPaintEvent* event)
{
    // Draw the widget background
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    
    QWidget::paintEvent(event);
}

void RatioGaugeWidget::updateExplanation()
{
    // Find the current zone to add a specific explanation
    QString zoneExplanation = "";
    
    for (const auto& zone : m_zones) {
        if (m_value >= zone.minValue && m_value <= zone.maxValue) {
            zoneExplanation = QString("<br><b>Interpretation:</b> %1").arg(zone.description);
            break;
        }
    }

    QString tooltipText = m_baseExplanation + zoneExplanation;

    // Apply the tooltip to main widgets
    this->setToolTip(tooltipText);
    m_gaugeWidget->setToolTip(tooltipText);
    m_metricNameLabel->setToolTip(tooltipText);
    m_valueLabel->setToolTip(tooltipText);
}

// Override gauge update function to use the specialized widget
void RatioGaugeWidget::updateGauge()
{    
    // Configure the render widget
    m_gaugeWidget->setZones(m_zones);
    m_gaugeWidget->setRange(m_minValue, m_maxValue);
    m_gaugeWidget->setFillDirection(m_fillDirection);
    m_gaugeWidget->setReferenceValue(m_referenceValue);
    m_gaugeWidget->showReference(m_showReference);

    // Do not show cursor if no value defined
    if (m_hasValue) {
        m_gaugeWidget->setValue(m_value);
        // Update only the value, not the metric name
        m_valueLabel->setText(QString::number(m_value, 'f', m_precision) + (m_addPercentageSign ? "%" : ""));
        
        // Find the current zone
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
        // Reset display
        // m_gaugeWidget->setShowCursor(false);
        m_valueLabel->setText("N/A");
    }

    updateExplanation();
}