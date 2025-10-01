#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>

class EquityWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EquityWidget(QWidget *parent = nullptr);


    // Replace points (will be sorted by X and filtered for consecutive duplicates in Y)
    void setPoints(const QVector<QPointF>& pts);
    void setPoints(const std::vector<double>& pts);
    QVector<QPointF> points() const { return m_points; }


protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;


private:
    QVector<QPointF> m_points; // sorted, filtered


    // world bounds
    double m_xmin, m_xmax, m_ymin, m_ymax;
    void updateBounds();
    QPointF mapToWidget(const QPointF &pt) const;
    QPointF mapToWorld(const QPointF &pixel) const;


    // mouse crosshair
    bool m_showCrosshair;
    QPoint m_mousePos; // in widget pixel coordinates


    // drawing helpers
    void drawGrid(QPainter &painter);
    void drawAxes(QPainter &painter);


    // layout margins to leave space for axis labels
    const int m_leftMargin = 80;
    const int m_bottomMargin = 30;
    const int m_rightMargin = 10;
    const int m_topMargin = 10;
};
