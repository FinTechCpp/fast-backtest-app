#include "ui/views/Stats/EquityWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>
#include <QPainterPath>

static constexpr double EPSILON_D = 1e-9;

EquityWidget::EquityWidget(const QString& title, QWidget *parent)
    : TitledWidget(title, parent),
      m_xmin(0), m_xmax(1), m_ymin(0), m_ymax(1),
      m_showCrosshair(false)
{
    setMinimumSize(160, 120);
    setMouseTracking(true); // necessary to receive mouseMoveEvent without pressing buttons
}

void EquityWidget::setPoints(const QVector<QPointF>& pts)
{
    if (pts.isEmpty()) {
        m_points.clear();
        updateBounds();
        update();
        return;
    }

    // Copier dans un std::vector pour le tri
    std::vector<QPointF> tmp;
    tmp.reserve(pts.size());
    for (const auto &p: pts) tmp.push_back(p);

    // Trier les points par X croissant
    std::sort(tmp.begin(), tmp.end(), [](const QPointF &a, const QPointF &b){
        return a.x() < b.x();
    });

    // Convertir le vecteur trié en QVector
    m_points.clear();
    m_points.reserve(tmp.size());
    for (const auto &p: tmp) m_points.push_back(p);

    updateBounds();
    update();
}

void EquityWidget::setPoints(const std::vector<be::Stats::EquityPoint>& equityCurve) {
    QVector<QPointF> newPoints;
    
    // Si le tableau contient uniquement des valeurs Y
    if (!equityCurve.empty()) {
        // Créer des points avec X = index et Y = valeur
        for (size_t i = 0; i < equityCurve.size(); ++i) {
            newPoints.append(QPointF(static_cast<double>(equityCurve[i].index), equityCurve[i].value));
        }
    }
    
    setPoints(newPoints);
}

void EquityWidget::updateBounds()
{
    if (m_points.isEmpty()) {
        m_xmin = 0; m_xmax = 1;
        m_ymin = 0; m_ymax = 1;
        return;
    }
    m_xmin = m_xmax = m_points[0].x();
    m_ymin = m_ymax = m_points[0].y();
    for (const auto &p : m_points) {
        m_xmin = qMin(m_xmin, p.x());
        m_xmax = qMax(m_xmax, p.x());
        m_ymin = qMin(m_ymin, p.y());
        m_ymax = qMax(m_ymax, p.y());
    }
}

void EquityWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    m_contentRect = contentRect.adjusted(m_margin, m_margin, -m_margin, -m_margin);
    
    painter.setRenderHint(QPainter::Antialiasing, true);

    // background
    painter.fillRect(m_contentRect, Qt::white);

    // draw grid
    drawGrid(painter);

    // draw axes (ticks + labels)
    drawAxes(painter);

    // draw polyline connecting points (no squares on points)
    if (!m_points.isEmpty()) {
        QPainterPath path;
        QPointF p0 = mapToWidget(m_points.front());
        path.moveTo(p0);
        for (int i = 1; i < m_points.size(); ++i) {
            QPointF w = mapToWidget(m_points[i]);
            path.lineTo(w);
        }

        QPen linePen(Qt::blue);
        linePen.setWidth(2);
        painter.setPen(linePen);
        painter.drawPath(path);
    }

    // crosshair
    if (m_showCrosshair) {
        QPen crossPen(Qt::black);
        crossPen.setStyle(Qt::DashLine);
        crossPen.setWidth(1);
        painter.setPen(crossPen);

        // Convertir la position de la souris en coordonnées relatives à drawRect
        QPoint relativeMousePos = m_mousePos - contentRect.topLeft() - QPoint(m_margin, m_margin);

        // vertical
        painter.drawLine(relativeMousePos.x(), m_contentRect.top(),
                         relativeMousePos.x(), m_contentRect.top() + m_contentRect.height() - m_bottomMargin);
        // horizontal
        painter.drawLine(m_leftMargin, relativeMousePos.y(),
                         m_contentRect.width() - m_rightMargin, relativeMousePos.y());

        // small info box with world coordinates
        QPointF world = mapToWorld(relativeMousePos);
        QString info = QString("(%1, %2)").arg(world.x(), 0, 'g', 6).arg(world.y(), 0, 'g', 6);
        QRect infoRect(relativeMousePos.x() + 10, relativeMousePos.y() - 20, 120, 18);
        painter.fillRect(infoRect, QColor(255,255,224,230));
        painter.setPen(Qt::black);
        painter.drawRect(infoRect);
        painter.drawText(infoRect.adjusted(4,0,-4,0), Qt::AlignVCenter | Qt::AlignLeft, info);
    }
}

QPointF EquityWidget::mapToWidget(const QPointF &pt) const
{
    double w = m_contentRect.width() - m_leftMargin - m_rightMargin;
    double h = m_contentRect.height() - m_topMargin - m_bottomMargin;
    double x = m_contentRect.left() + m_leftMargin + (pt.x() - m_xmin) / (m_xmax - m_xmin) * w;
    double y = m_contentRect.top() + m_topMargin + (1.0 - (pt.y() - m_ymin) / (m_ymax - m_ymin)) * h;
    return QPointF(x, y);
}

QPointF EquityWidget::mapToWorld(const QPointF &pixel) const
{
    double w = m_contentRect.width() - m_leftMargin - m_rightMargin;
    double h = m_contentRect.height() - m_topMargin - m_bottomMargin;
    double nx = (pixel.x() - m_contentRect.left() - m_leftMargin) / w;
    double ny = 1.0 - (pixel.y() - m_contentRect.top() - m_topMargin) / h;
    double wx = m_xmin + nx * (m_xmax - m_xmin);
    double wy = m_ymin + ny * (m_ymax - m_ymin);
    return QPointF(wx, wy);
}

void EquityWidget::drawGrid(QPainter &painter)
{
    // We draw grid lines aligned to "nice" world ticks so labels match the grid
    const int desiredLines = 8;
    double xrange = m_xmax - m_xmin;
    double yrange = m_ymax - m_ymin;
    if (xrange <= 0 || yrange <= 0) return;

    // compute a nice step
    auto niceStep = [](double range, int target){
        double raw = range / target;
        double expv = qPow(10.0, qFloor(qLn(raw)/qLn(10.0)));
        double f = raw / expv;
        double nicef;
        if (f < 1.5) nicef = 1.0;
        else if (f < 3.0) nicef = 2.0;
        else if (f < 7.0) nicef = 5.0;
        else nicef = 10.0;
        return nicef * expv;
    };

    double xstep = niceStep(xrange, desiredLines);
    double ystep = niceStep(yrange, desiredLines);

    QPen gridPen(QColor(220,220,220));
    gridPen.setWidth(1);
    painter.setPen(gridPen);

    // vertical lines
    double xstart = std::floor(m_xmin / xstep) * xstep;
    for (double x = xstart; x <= m_xmax; x += xstep) {
        QPointF p1 = mapToWidget(QPointF(x, m_ymin));
        painter.drawLine(QPointF(p1.x(), m_contentRect.top() + m_topMargin), 
                         QPointF(p1.x(), m_contentRect.top() + m_contentRect.height() - m_bottomMargin));
    }

    // horizontal lines - inchangé, toujours de gauche à droite
    double ystart = std::floor(m_ymin / ystep) * ystep;
    for (double y = ystart; y <= m_ymax; y += ystep) {
        QPointF p1 = mapToWidget(QPointF(m_xmin, y));
        painter.drawLine(QPointF(m_contentRect.left() + m_leftMargin, p1.y()), 
                         QPointF(m_contentRect.left() + m_contentRect.width() - m_rightMargin, p1.y()));
    }
}

QString formatValue(double value, bool useThousandsSeparator = true) {
    // Gérer les valeurs proches de zéro
    if (std::abs(value) < 0.01) {
        return "0";
    }

    // Trouver l'ordre de grandeur pour l'arrondi
    double absValue = std::abs(value);
    int digits = std::floor(std::log10(absValue));
    double factor;
    
    // Déterminer le facteur d'arrondi selon l'ordre de grandeur
    if (absValue >= 100000) {
        factor = std::pow(10, digits - 1);  // Arrondi aux 10000
    } else if (absValue >= 10000) {
        factor = 1000;  // Arrondi aux 1000
    } else if (absValue >= 1000) {
        factor = 100;   // Arrondi aux 100
    } else if (absValue >= 100) {
        factor = 10;    // Arrondi aux 10
    } else {
        factor = 1;     // Arrondi aux unités
    }
    
    // Arrondir à la précision déterminée
    double rounded = std::round(value / factor) * factor;
    
    // Formater avec QLocale pour les séparateurs de milliers
    if (useThousandsSeparator) {
        QLocale locale;
        return locale.toString(rounded, 'f', rounded < 10 ? 1 : 0);
    } else {
        return QString::number(rounded, 'f', rounded < 10 ? 1 : 0);
    }
}

void EquityWidget::drawAxes(QPainter &painter)
{
    QPen axisPen(Qt::black);
    axisPen.setWidth(1);
    painter.setPen(axisPen);

    // draw X axis at bottom (leave margin for labels)
    painter.drawLine(m_contentRect.left() + m_leftMargin, 
                     m_contentRect.top() + m_contentRect.height() - m_bottomMargin, 
                     m_contentRect.left() + m_contentRect.width() - m_rightMargin, 
                     m_contentRect.top() + m_contentRect.height() - m_bottomMargin);
    
    // draw Y axis at RIGHT (instead of left)
    painter.drawLine(m_contentRect.left() + m_contentRect.width() - m_rightMargin, 
                     m_contentRect.top() + m_topMargin, 
                     m_contentRect.left() + m_contentRect.width() - m_rightMargin, 
                     m_contentRect.top() + m_contentRect.height() - m_bottomMargin);

    // ticks and labels
    const int xTicks = 6;
    const int yTicks = 5;
    
    double xrange = m_xmax - m_xmin;
    double yrange = m_ymax - m_ymin;
    if (xrange <= 0 || yrange <= 0) return;

    double xstep = xrange / xTicks;
    double ystep = yrange / yTicks;

    QFontMetrics fm(font());

    // Axe X - inchangé
    for (int i = 0; i <= xTicks; ++i) {
        double xv = m_xmin + i * xstep;
        QPointF wp = mapToWidget(QPointF(xv, m_ymin));
        // tick
        painter.drawLine(QPointF(wp.x(), m_contentRect.top() + m_contentRect.height() - m_bottomMargin), 
                         QPointF(wp.x(), m_contentRect.top() + m_contentRect.height() - m_bottomMargin + 4));
        // label
        QString txt;
        if (std::abs(xv - std::round(xv)) < 0.001) {
            txt = QString::number(static_cast<int>(xv));
        } else {
            txt = QString::number(xv, 'f', 1);
        }
        
        int tw = fm.horizontalAdvance(txt);
        painter.drawText(QPointF(wp.x() - tw/2, m_contentRect.top() + m_contentRect.height() - 6), txt);
    }

    // Axe Y - MODIFIÉ pour afficher à droite
    for (int i = 0; i <= yTicks; ++i) {
        double yv = m_ymin + i * ystep;
        QPointF wp = mapToWidget(QPointF(m_xmax, yv));  // Utiliser m_xmax au lieu de m_xmin
        
        // tick - à droite maintenant
        painter.drawLine(QPointF(m_contentRect.left() + m_contentRect.width() - m_rightMargin, wp.y()), 
                         QPointF(m_contentRect.left() + m_contentRect.width() - m_rightMargin + 4, wp.y()));
        
        // label - aligné à gauche après le tick (à droite du graphique)
        QString txt = formatValue(yv);
        painter.drawText(QPointF(m_contentRect.left() + m_contentRect.width() - m_rightMargin + 8, 
                                wp.y() + fm.ascent()/2 - 2), txt);
    }
}

void EquityWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void EquityWidget::mouseMoveEvent(QMouseEvent *event)
{    
    // Enregistrer la position globale de la souris
    m_mousePos = event->pos();
    
    // Vérifier que la position est dans contentRect
    if (!m_contentRect.contains(m_mousePos)) {
        m_showCrosshair = false;
        update();
        return;
    }
    
    // Clamp dans la zone de tracé
    QPoint relativePos = m_mousePos;
    // Ajuster pour la marge
    // relativePos.rx() -= m_margin;
    // relativePos.ry() -= m_margin;

    if (relativePos.x() < m_contentRect.left() + m_leftMargin) 
        relativePos.setX(m_contentRect.left() + m_leftMargin);
    if (relativePos.x() > m_contentRect.top() + m_contentRect.width() - m_rightMargin)
        relativePos.setX(m_contentRect.top() + m_contentRect.width() - m_rightMargin);
    if (relativePos.y() < m_contentRect.top() + m_topMargin)
        relativePos.setY(m_contentRect.top() + m_topMargin);
    if (relativePos.y() > m_contentRect.top() + m_contentRect.height() - m_bottomMargin)
        relativePos.setY(m_contentRect.top() + m_contentRect.height() - m_bottomMargin);

    // Réajuster en tenant compte de la marge
    m_mousePos = m_contentRect.topLeft() + QPoint(relativePos.x() + m_margin, relativePos.y() + m_margin);
    m_showCrosshair = true;
    update();
}

void EquityWidget::leaveEvent(QEvent * /*event*/)
{
    m_showCrosshair = false;
    update();
}
