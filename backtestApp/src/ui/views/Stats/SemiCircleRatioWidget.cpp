#include "ui/views/Stats/SemiCircleRatioWidget.h"

SemiCircleRatioWidget::SemiCircleRatioWidget(QWidget *parent)
    : BaseRatioWidget(parent)
{
    // Demi-cercle a besoin d'être plus large que haut
    setMinimumSize(120, 120);
}

QSize SemiCircleRatioWidget::sizeHint() const
{
    return QSize(120, 120);
}

QSize SemiCircleRatioWidget::minimumSizeHint() const
{
    return QSize(120, 120);
}

void SemiCircleRatioWidget::drawGauge(QPainter &painter, const QRectF &outerRect, 
                                     const QRectF &innerRect, int radius)
{
    // Pour un demi-cercle de l'ouest à l'est, on ajuste le rectangle
    // pour dessiner un cercle complet dont on ne verra que la moitié
    QRectF adjustedRect = outerRect;
    // adjustedRect.setHeight(outerRect.height() * 2);
    // adjustedRect.moveTop(outerRect.top() - outerRect.height() / 2);
    
    // Dessiner le fond du demi-cercle (partie négative)
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_negativeColor);
    painter.drawPie(adjustedRect, 0 * 16, 180 * 16);  // 0° à 180°
    
    // Dessiner la partie positive (en proportion)
    if (m_proportion > 0) {
        painter.setBrush(m_positiveColor);
        // Calculer l'angle de la partie positive (en 1/16 de degrés)
        int sweepAngle = static_cast<int>(-m_proportion * 180 * 16);
        painter.drawPie(adjustedRect, 180 * 16, sweepAngle);
    }
}