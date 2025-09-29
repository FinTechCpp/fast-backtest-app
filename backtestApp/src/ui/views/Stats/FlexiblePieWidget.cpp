#include "ui/views/Stats/FlexiblePieWidget.h"
#include <numeric>
#include <QDebug>

FlexiblePieWidget::FlexiblePieWidget(const QString& title, QWidget *parent)
    : TitledWidget(title, parent)
    , m_startAngle(90 * 16)   // Par défaut, commence en haut (90°)
    , m_angleSpan(360)         // Par défaut, cercle complet
    , m_innerRadiusRatio(0.65) // Ratio du rayon intérieur
    , m_centerText("--")
    , m_textSuffix("")
    , m_textColor(0, 0, 0)    // Noir par défaut
    , m_fontSizeRatio(1.0)
{
    // Définir un fond transparent
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMinimumSize(140, 140);
}

QSize FlexiblePieWidget::sizeHint() const
{
    // Pour un demi-cercle, on suggère un format plus large
    return QSize(140, 140);
}

QSize FlexiblePieWidget::minimumSizeHint() const
{
    return QSize(140, 140);
}

void FlexiblePieWidget::setStartAngle(int degrees)
{
    int newAngle = (degrees % 360) * 16; // Convertir en 1/16e de degrés pour QPainter
    if (m_startAngle != newAngle) {
        m_startAngle = newAngle;
        update();
    }
}

void FlexiblePieWidget::setAngleSpan(int degrees)
{
    int newSpan = qBound(1, degrees, 360);
    if (m_angleSpan != newSpan) {
        m_angleSpan = newSpan;
        update();
    }
}

void FlexiblePieWidget::setCenterText(const QString &text)
{
    if (m_centerText != text) {
        m_centerText = text;
        update();
    }
}

void FlexiblePieWidget::setCenterTextColor(const QColor &color)
{
    if (m_textColor != color) {
        m_textColor = color;
        update();
    }
}

void FlexiblePieWidget::setCenterTextSuffix(const QString &suffix)
{
    if (m_textSuffix != suffix) {
        m_textSuffix = suffix;
        update();
    }
}

void FlexiblePieWidget::setCenterTextFontSizeRatio(double ratio)
{
    if (ratio > 0 && m_fontSizeRatio != ratio) {
        m_fontSizeRatio = ratio;
        update();
    }
}

void FlexiblePieWidget::setInnerCircleRadius(double ratio)
{
    ratio = qBound(0.1, ratio, 0.9);
    if (m_innerRadiusRatio != ratio) {
        m_innerRadiusRatio = ratio;
        update();
    }
}

void FlexiblePieWidget::addSegment(double proportion, const QColor &color)
{
    if (proportion <= 0) return;
    
    m_segments.push_back(PieSegment(proportion, color));
    update();
}

void FlexiblePieWidget::setSegments(const std::vector<PieSegment>& segments)
{
    if (segments.empty()) return;
    
    m_segments = segments;
    update();
}

void FlexiblePieWidget::clearSegments()
{
    m_segments.clear();
    update();
}

void FlexiblePieWidget::paintContent(QPainter& painter, const QRect& contentRect)
{    
    // Calculer le rayon et le centre du cercle
    int size = qMin(contentRect.width(), contentRect.height());
    int radius = size / 2 - 5; // Marge de 5 pixels
    
    // Adapter la géométrie pour les formes partielles
    QRectF outerRect = QRectF(
        contentRect.left() + contentRect.width()/2 - radius, 
        contentRect.top() + contentRect.height()/2 - radius, 
        radius * 2, radius * 2
    );

    // Dessiner les segments si disponibles
    if (!m_segments.empty()) {
        int currentAngle = m_startAngle;
        constexpr int offsetAngle = 24; // Espacement entre les segments en 1/16e de degrés
        
        // Parcourir chaque segment
        for (const auto& segment : m_segments) {
            if (segment.proportion <= 0)
                continue;

            // Calculer l'angle pour ce segment proportionnellement à l'angle max
            int sweepAngle = -static_cast<int>(segment.proportion * m_angleSpan * 16 - offsetAngle);
            
            painter.setPen(Qt::NoPen);
            painter.setBrush(segment.color);
            painter.drawPie(outerRect, currentAngle, sweepAngle);
            
            // Mettre à jour l'angle pour le segment suivant
            currentAngle += sweepAngle - offsetAngle;
        }
    } else {
        // Aucun segment, dessiner un cercle gris
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(200, 200, 200));
        if (m_angleSpan >= 360) {
            painter.drawEllipse(outerRect);
        } else {
            painter.drawPie(outerRect, m_startAngle, -m_angleSpan * 16);
        }
    }
    
    // Dessiner le cercle intérieur
    int innerRadius = radius * m_innerRadiusRatio;
    QRectF innerRect = QRectF(
        contentRect.left() + contentRect.width()/2 - innerRadius,
        contentRect.top() + contentRect.height()/2 - innerRadius, 
        innerRadius * 2, innerRadius * 2
    );
        
    painter.setBrush(palette().color(QPalette::Window));
    painter.drawEllipse(innerRect);
    
    // Définir la police pour le texte central
    QFont font = painter.font();
    // font.setBold(true);
    
    // Ajuster la taille de police
    int fontSize = radius / 4 * m_fontSizeRatio;
    font.setPointSize(fontSize);
    painter.setFont(font);
    
    // Afficher le texte au centre
    painter.setPen(m_textColor);
    QString displayText = m_centerText + m_textSuffix;
    painter.drawText(innerRect, Qt::AlignCenter, displayText);
}