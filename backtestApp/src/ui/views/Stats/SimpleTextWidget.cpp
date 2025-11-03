#include "ui/views/Stats/SimpleTextWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <QPaintEvent>

SimpleTextWidget::SimpleTextWidget(const QString& title, QWidget *parent)
    : TitledWidget(title, parent)
    , m_statText("--")
    , m_textColor(0, 0, 0)       // Black by default
    , m_backgroundColor(Qt::transparent)  // Transparent by default
    , m_suffix("")
    , m_textOffsetX(0)
    , m_textOffsetY(0)
    , m_statTextBis("")
    , m_suffixBis("")
    , m_showBis(false)
    , m_checkBoxString("Show secondary text")
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMinimumSize(100, 60);

    m_fontSize = 15;

    // QCheckBox to switch between m_statText and m_statTextBis
    m_checkBox = new QCheckBox(m_checkBoxString);
    m_checkBox->setChecked(m_showBis);

    // Connect the QCheckBox toggled signal to a lambda slot
    connect(m_checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_showBis = checked;
        update();  // Redraw the widget
    });

    // Add the QCheckBox as a companion widget in the title
    setTitleCompanionWidget(m_checkBox);
    setTitleCompanionWidgetVisible(false);
}

QString SimpleTextWidget::formatWithThousandsSeparator(double value, int precision) {
    QString numStr = QString::number(value, 'f', precision);
    
    // Separate integer and fractional parts
    QStringList parts = numStr.split(".");
    QString intPart = parts[0];
    QString fracPart = parts.size() > 1 ? parts[1] : "";
    
    // Add thousand separators to the integer part
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
    
    // Reappend the fractional part if it exists
    if (!fracPart.isEmpty()) {
        result.append("." + fracPart);
    }
    
    return result;
}

void SimpleTextWidget::setStatText(const QString &text)
{
    if (m_statText != text) {
        m_statText = text;
        update();  // Trigger a repaint
    }
}

void SimpleTextWidget::setCheckBoxBisString(const QString &text)
{
    if (m_checkBoxString != text) {
        m_checkBoxString = text;
        m_checkBox->setText(text);
        setTitleCompanionWidgetVisible(true);
        update();  // Trigger a repaint
    }
}

void SimpleTextWidget::setStatTextBis(const QString &text) {
    if (m_statTextBis != text) {
        m_statTextBis = text;
        update();  // Trigger a repaint
    }
}

void SimpleTextWidget::setStatColors(const QColor &textColor)
{
    if (m_textColor != textColor) {
        m_textColor = textColor;
        update();  // Trigger a repaint
    }
}

void SimpleTextWidget::setSuffix(const QString &suffix)
{
    if (m_suffix != suffix) {
        m_suffix = suffix;
        update();
    }
}

void SimpleTextWidget::setSuffixBis(const QString &suffix)
{
    if (m_suffixBis != suffix) {
        m_suffixBis = suffix;
        update();
    }
}

void SimpleTextWidget::setTextOffset(int x, int y)
{
    if (m_textOffsetX != x || m_textOffsetY != y) {
        m_textOffsetX = x;
        m_textOffsetY = y;
        update();  // Trigger a repaint
    }
}

void SimpleTextWidget::setTextOffsetX(int x)
{
    if (m_textOffsetX != x) {
        m_textOffsetX = x;
        update();  // Trigger a repaint
    }
}

void SimpleTextWidget::setTextOffsetY(int y)
{
    if (m_textOffsetY != y) {
        m_textOffsetY = y;
        update();  // Trigger a repaint
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
    // Draw the background rectangle
    QRect rect = contentRect.adjusted(2, 2, -2, -2); // 2-pixel margin
    
    // Set font for the text
    QFont font = painter.font();
    font.setWeight(m_fontWeight);
    
    // Adjust font size according to widget size
    if (m_fontSize > 0) {
        font.setPointSize(m_fontSize);
    } else {
        int fontSize = qMin(width(), height()) / 4;
        font.setPointSize(fontSize);
    }
    painter.setFont(font);

    QRect textRect = rect.adjusted(m_textOffsetX, m_textOffsetY, m_textOffsetX, m_textOffsetY);

    
    // Display the text centered
    painter.setPen(m_textColor);
    QString displayText;

    if (m_showBis && !m_statTextBis.isEmpty()) {
        displayText = m_statTextBis + m_suffixBis;
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, displayText);
    } else {
        displayText = m_statText + m_suffix;
        painter.drawText(textRect, Qt::AlignCenter, displayText);
    }
}