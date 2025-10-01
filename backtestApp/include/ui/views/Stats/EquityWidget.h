#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include "ui/views/Stats/TitledWidget.h"


// TODO : il faut donner au widget les date relatives aux points X pour afficher les labels d'axe X correctement (dates et plus indices des points)
// IL FAUT AJOUTER DANS LES stats le calcule de l'equity curve sous forme de pnl en POURCENT et afficher l'equity curve en pourcent
class EquityWidget : public TitledWidget
{
    Q_OBJECT
public:
    explicit EquityWidget(const QString& title = QString(), QWidget *parent = nullptr);


    // Replace points (will be sorted by X and filtered for consecutive duplicates in Y)
    void setPoints(const QVector<QPointF>& pts);
    void setPoints(const std::vector<double>& pts);
    QVector<QPointF> points() const { return m_points; }


protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;


private:
    QVector<QPointF> m_points; // sorted, filtered


    // world bounds
    double m_xmin, m_xmax, m_ymin, m_ymax;
    void updateBounds();


    // mouse crosshair
    bool m_showCrosshair;
    QPoint m_mousePos; // in widget pixel coordinates


    // drawing helpers
    void drawGrid(QPainter &painter);
    void drawAxes(QPainter &painter);
    QPointF mapToWidget(const QPointF &pt) const;
    QPointF mapToWorld(const QPointF &pixel) const;


    // layout margins to leave space for axis labels
    const int m_leftMargin = 10;      // Réduit car plus de labels à gauche
    const int m_bottomMargin = 30;
    const int m_rightMargin = 80;     // Augmenté pour les labels Y à droite
    const int m_topMargin = 10;

    QRect m_contentRect;
    int m_margin = 5;
};
