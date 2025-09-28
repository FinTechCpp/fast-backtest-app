#include "ui/views/Stats/BaseRatioWidget.h"
#include <QPainter>
#include <QDebug>

BaseRatioWidget::BaseRatioWidget(QWidget *parent, double fontSizeRatio, double innerRadiusRatio)
    : QWidget(parent)
    , m_statText("N/A")
    , m_proportion(0.5)
    , m_positiveColor(0, 190, 0)  // Vert
    , m_negativeColor(190, 0, 0)  // Rouge
    , m_statTextColor(QColor(0, 0, 0)) // Par défaut, noir
    , m_suffix("")
    , m_fontSizeRatio(fontSizeRatio)
    , m_innerRadiusRatio(qBound(0.1, innerRadiusRatio, 0.9))
{
    // Définir un fond transparent
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void BaseRatioWidget::setStatText(const QString &text)
{
    if (m_statText != text) {
        m_statText = text;
        update();  // Déclencher un repaint
    }
}

void BaseRatioWidget::setProportion(double proportion)
{
    // Limiter la proportion entre 0 et 1
    proportion = qBound(0.0, proportion, 1.0);
    
    if (m_proportion != proportion) {
        m_proportion = proportion;
        update();  // Déclencher un repaint
    }
}

void BaseRatioWidget::setColors(const QColor &positiveColor, const QColor &negativeColor, const QColor &statTextColor)
{
    if (m_positiveColor != positiveColor || m_negativeColor != negativeColor || m_statTextColor != statTextColor) {
        m_positiveColor = positiveColor;
        m_negativeColor = negativeColor;
        m_statTextColor = statTextColor;
        update();  // Déclencher un repaint
    }
}

void BaseRatioWidget::setFontSizeRatio(double ratio)
{
    if (m_fontSizeRatio != ratio && ratio > 0) {
        m_fontSizeRatio = ratio;
        update();
    }
}

void BaseRatioWidget::setInnerRadiusRatio(double ratio)
{
    // Limiter le ratio pour éviter des cercles trop grands ou trop petits
    ratio = qBound(0.1, ratio, 0.9);
    
    if (m_innerRadiusRatio != ratio) {
        m_innerRadiusRatio = ratio;
        update();
    }
}

void BaseRatioWidget::setSuffix(const QString &suffix)
{
    if (m_suffix != suffix) {
        m_suffix = suffix;
        update();
    }
}

void BaseRatioWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Calculer le rayon et le centre du cercle
    int size = qMin(width(), height());
    int outerRadius = size / 2 - 5; // Marge de 5 pixels
    QRectF outerRect(width()/2 - outerRadius, height()/2 - outerRadius, 
                    outerRadius * 2, outerRadius * 2);
    
    // Utiliser le ratio pour calculer le rayon intérieur
    int innerRadius = outerRadius * m_innerRadiusRatio;
    QRectF innerRect(width()/2 - innerRadius, height()/2 - innerRadius,
                    innerRadius * 2, innerRadius * 2);
    
    // Appeler la méthode spécifique à la classe dérivée pour dessiner la jauge
    drawGauge(painter, outerRect, innerRect, outerRadius);
    
    // Récupérer la couleur de fond du parent ou de l'application
    QColor backgroundColor;
    if (parentWidget()) {
        backgroundColor = parentWidget()->palette().color(QPalette::Window);
    } else {
        backgroundColor = palette().color(QPalette::Window);
    }
    
    // Dessiner le cercle intérieur avec la couleur de fond du parent
    painter.setBrush(backgroundColor);
    painter.drawEllipse(innerRect);
    
    // Définir la police pour le texte
    QFont font = painter.font();
    font.setBold(true);
    
    // Ajuster la taille de police en utilisant le ratio personnalisé
    int fontSize = static_cast<int>(outerRadius / 4 * m_fontSizeRatio);
    font.setPointSize(fontSize);
    painter.setFont(font);
    
    // Afficher la statistique au centre
    painter.setPen(m_statTextColor);
    QString displayText = m_statText + m_suffix;
    painter.drawText(innerRect, Qt::AlignCenter, displayText);
}