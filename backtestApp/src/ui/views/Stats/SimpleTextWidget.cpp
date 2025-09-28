#include "ui/views/Stats/SimpleTextWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <QPaintEvent>

SimpleTextWidget::SimpleTextWidget(QWidget *parent)
    : QWidget(parent)
    , m_statText("N/A")
    , m_textColor(0, 0, 0)       // Noir par défaut
    , m_backgroundColor(240, 240, 240)  // Gris clair par défaut
    , m_suffix("")
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

void SimpleTextWidget::setColors(const QColor &textColor, const QColor &backgroundColor)
{
    if (m_textColor != textColor || m_backgroundColor != backgroundColor) {
        m_textColor = textColor;
        m_backgroundColor = backgroundColor;
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

QSize SimpleTextWidget::sizeHint() const
{
    return QSize(120, 60);
}

QSize SimpleTextWidget::minimumSizeHint() const
{
    return QSize(60, 40);
}

void SimpleTextWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Dessiner le rectangle de fond
    QRect rect = this->rect().adjusted(2, 2, -2, -2); // Marge de 2 pixels
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_backgroundColor);
    painter.drawRoundedRect(rect, 5, 5); // Coins arrondis
    
    // Définir la police pour le texte
    QFont font = painter.font();
    font.setBold(true);
    
    // Ajuster la taille de police en fonction de la taille du widget
    int fontSize = qMin(width(), height()) / 4;
    font.setPointSize(fontSize);
    painter.setFont(font);
    
    // Afficher le texte au centre
    painter.setPen(m_textColor);
    QString displayText = m_statText + m_suffix;
    painter.drawText(rect, Qt::AlignCenter, displayText);
}