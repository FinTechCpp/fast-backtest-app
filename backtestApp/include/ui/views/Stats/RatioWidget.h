#pragma once

#include "ui/views/Stats/BaseRatioWidget.h"

class RatioWidget : public BaseRatioWidget
{
    Q_OBJECT

public:
    explicit RatioWidget(QWidget *parent = nullptr);
    
    // Anciens noms de méthodes pour compatibilité
    void setGreenProportion(double proportion) { setProportion(proportion); }
    double greenProportion() const { return proportion(); }
    QColor greenColor() const { return positiveColor(); }
    QColor redColor() const { return negativeColor(); }
    
    // Implémentation des méthodes abstraites
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    // Implémentation du dessin spécifique au cercle complet
    void drawGauge(QPainter &painter, const QRectF &outerRect, 
                  const QRectF &innerRect, int radius) override;
};