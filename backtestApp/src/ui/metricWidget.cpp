#include "ui/metricWidget.h"

MetricWidget::MetricWidget(const QString& label, const QString& value, QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(10);

    m_labelWidget = new QLabel(label);
    m_valueWidget = new QLabel(value);

    m_labelWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_valueWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QFont labelFont = m_labelWidget->font();
    labelFont.setPointSize(labelFont.pointSize() + 1);
    m_labelWidget->setFont(labelFont);

    QFont valueFont = m_valueWidget->font();
    valueFont.setBold(true);
    valueFont.setPointSize(valueFont.pointSize() + 2);
    m_valueWidget->setFont(valueFont);

    layout->addWidget(m_labelWidget);
    layout->addStretch(1);
    layout->addWidget(m_valueWidget);

    setLayout(layout);
}

void MetricWidget::updateValues(const QString& value, MetricStatus status) {
    m_valueWidget->setText(value);
    QString styleSheet;
    switch (status) {
        case MetricStatus::Good:
            styleSheet = "QLabel { color: #2ecc71; }";
            break;
        case MetricStatus::Bad:
            styleSheet = "QLabel { color: #e74c3c; }";
            break;
        case MetricStatus::NA:
            styleSheet = "QLabel { color: #7f8c8d; font-style: italic; }";
            break;
        case MetricStatus::Neutral:
        default:
            styleSheet = "QLabel { color: black; }";
            break;
    }
    m_valueWidget->setStyleSheet(styleSheet);
}

void MetricWidget::setTooltip(const QString& tooltip) {
    setToolTip(tooltip);
}