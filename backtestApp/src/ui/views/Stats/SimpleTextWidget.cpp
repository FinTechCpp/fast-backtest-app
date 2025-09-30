#include "ui/views/Stats/SimpleTextWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <QPaintEvent>

SimpleTextWidget::SimpleTextWidget(const QString& title, QWidget *parent)
    : TitledWidget(title, parent)
    , m_statText("--")
    , m_textColor(0, 0, 0)       // Noir par défaut
    , m_backgroundColor(Qt::transparent)  // Transparent par défaut
    , m_suffix("")
    , m_textOffsetX(0)
    , m_textOffsetY(0)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMinimumSize(100, 60);
}

QString SimpleTextWidget::formatWithThousandsSeparator(double value, int precision) {
    QString numStr = QString::number(value, 'f', precision);
    
    // Séparer la partie entière et décimale
    QStringList parts = numStr.split(".");
    QString intPart = parts[0];
    QString fracPart = parts.size() > 1 ? parts[1] : "";
    
    // Ajouter les séparateurs de milliers à la partie entière
    QString result;
    int count = 0;
    for (int i = intPart.length() - 1; i >= 0; i--) {
        if (count == 3 && i != 0) {
            result.prepend(' ');
            count = 0;
        }
        result.prepend(intPart[i]);
        count++;
    }
    
    // Rajouter la partie décimale si elle existe
    if (!fracPart.isEmpty()) {
        result.append("." + fracPart);
    }
    
    return result;
}

void SimpleTextWidget::setStatText(const QString &text)
{
    if (m_statText != text) {
        m_statText = text;
        update();  // Déclencher un repaint
    }
}

void SimpleTextWidget::setStatColors(const QColor &textColor)
{
    if (m_textColor != textColor) {
        m_textColor = textColor;
        update();  // Déclencher un repaint
    }
}

void SimpleTextWidget::setSuffix(const QString &suffix)
{
    if (m_suffix != suffix) {
        m_suffix = suffix;
        update();
    }
}

void SimpleTextWidget::setTextOffset(int x, int y)
{
    if (m_textOffsetX != x || m_textOffsetY != y) {
        m_textOffsetX = x;
        m_textOffsetY = y;
        update();  // Déclencher un repaint
    }
}

void SimpleTextWidget::setTextOffsetX(int x)
{
    if (m_textOffsetX != x) {
        m_textOffsetX = x;
        update();  // Déclencher un repaint
    }
}

void SimpleTextWidget::setTextOffsetY(int y)
{
    if (m_textOffsetY != y) {
        m_textOffsetY = y;
        update();  // Déclencher un repaint
    }
}

QSize SimpleTextWidget::sizeHint() const
{
    return QSize(120, 60);
}

QSize SimpleTextWidget::minimumSizeHint() const
{
    return QSize(60, 40);
}

void SimpleTextWidget::paintContent(QPainter& painter, const QRect& contentRect)
{    
    // Dessiner le rectangle de fond
    QRect rect = contentRect.adjusted(2, 2, -2, -2); // Marge de 2 pixels
    
    // Définir la police pour le texte
    QFont font = painter.font();
    font.setWeight(m_fontWeight);
    
    // Ajuster la taille de police en fonction de la taille du widget
    if (m_fontSize > 0) {
        font.setPointSize(m_fontSize);
    } else {
        int fontSize = qMin(width(), height()) / 4;
        font.setPointSize(fontSize);
    }
    painter.setFont(font);

    QRect textRect = rect.adjusted(m_textOffsetX, m_textOffsetY, m_textOffsetX, m_textOffsetY);

    
    // Afficher le texte au centre
    painter.setPen(m_textColor);
    QString displayText = m_statText + m_suffix;
    painter.drawText(textRect, Qt::AlignCenter, displayText);
}