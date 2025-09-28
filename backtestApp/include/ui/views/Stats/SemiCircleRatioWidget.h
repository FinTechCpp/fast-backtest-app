#pragma once

#include "ui/views/Stats/BaseRatioWidget.h"

class SemiCircleRatioWidget : public BaseRatioWidget
{
    Q_OBJECT

public:
    explicit SemiCircleRatioWidget(QWidget *parent = nullptr);
    
    // Implémentation des méthodes abstraites
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    // Implémentation du dessin spécifique au demi-cercle
    void drawGauge(QPainter &painter, const QRectF &outerRect, 
                  const QRectF &innerRect, int radius) override;
};