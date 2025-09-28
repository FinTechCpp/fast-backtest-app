#include "ui/views/Stats/RatioWidget.h"

RatioWidget::RatioWidget(QWidget *parent)
    : BaseRatioWidget(parent)
{
    setMinimumSize(120, 120);
}

QSize RatioWidget::sizeHint() const
{
    return QSize(120, 120);
}

QSize RatioWidget::minimumSizeHint() const
{
    return QSize(120, 120);
}

void RatioWidget::drawGauge(QPainter &painter, const QRectF &outerRect, 
                           const QRectF &innerRect, int radius)
{
    // Dessiner le fond du cercle (partie négative)
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_negativeColor);
    painter.drawEllipse(outerRect);
    
    // Dessiner la partie positive (en proportion)
    if (m_proportion > 0) {
        painter.setBrush(m_positiveColor);
        painter.drawPie(outerRect, 90 * 16, -m_proportion * 360 * 16);
    }
}