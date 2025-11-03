#include "ui/views/Stats/TitledWidget.h"
#include <QPainter>
#include <QFontMetrics>

TitledWidget::TitledWidget(const QString& title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_titleColor(Qt::black)
    , m_titleBackgroundColor(Qt::transparent) // Transparent
    , m_titleVisible(!title.isEmpty())
    , m_backgroundColor(Qt::transparent) // Transparent
    , m_titleCompanionWidget(nullptr)
    , m_backgroundIconSize(64, 64) // Default background icon size
    , m_backgroundIconAlignment(Qt::AlignCenter) // Centered by default
    , m_backgroundIconOpacity(0.15) // Slight default opacity
    , m_fontWeight(QFont::DemiBold)
    , m_fontSize(0)
{
    // setMinimumSize(100, 100);
    // setMinimumSize(140, 140);
}

void TitledWidget::setTitle(const QString& title)
{
    if (m_title != title) {
        m_title = title;
        m_titleVisible = !title.isEmpty();
        update();
    }
}

void TitledWidget::setTitleColor(const QColor& color)
{
    if (m_titleColor != color) {
        m_titleColor = color;
        update();
    }
}

void TitledWidget::setTitleBackgroundColor(const QColor& color)
{
    if (m_titleBackgroundColor != color) {
        m_titleBackgroundColor = color;
        update();
    }
}

void TitledWidget::setTitleVisible(bool visible)
{
    if (m_titleVisible != visible) {
        m_titleVisible = visible;
        update();
    }
}

void TitledWidget::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
    }
}

void TitledWidget::setFontWeight(QFont::Weight weight)
{
    if (m_fontWeight != weight) {
        m_fontWeight = weight;
        update();
    }
}

void TitledWidget::setFontSize(int fontSize)
{
    if (m_fontSize != fontSize) {
        m_fontSize = fontSize;
        update();  // Trigger a repaint
    }
}

void TitledWidget::setTitleCompanionWidget(QWidget* widget)
{
    // If a widget already exists, remove it
    if (m_titleCompanionWidget) {
        m_titleCompanionWidget->setParent(nullptr);
        m_titleCompanionWidget->deleteLater();
    }
    
    m_titleCompanionWidget = widget;
    
    if (m_titleCompanionWidget) {
        m_titleCompanionWidget->setParent(this);
        m_titleCompanionWidget->show();
    }
    
    update();
}

void TitledWidget::setTitleCompanionWidgetVisible(bool visible)
{
    if (m_titleCompanionWidget) {
        m_titleCompanionWidget->setVisible(visible);
    }
}

void TitledWidget::setBackgroundIcon(const QIcon& icon)
{
    m_backgroundIcon = icon;
    update();
}

void TitledWidget::setBackgroundIconSize(const QSize& size)
{
    m_backgroundIconSize = size;
    update();
}

void TitledWidget::setBackgroundIconAlignment(Qt::Alignment alignment)
{
    m_backgroundIconAlignment = alignment;
    update();
}

void TitledWidget::setBackgroundIconOpacity(qreal opacity)
{
    m_backgroundIconOpacity = qBound(0.0, opacity, 1.0);
    update();
}

QRect TitledWidget::contentRect() const
{
    QRect rect = this->rect();
    
    if (m_titleVisible && !m_title.isEmpty()) {
        // Reserve space for the title at the top
        QFontMetrics fm(font());
        int titleHeight = fm.height() + 4; // Text height + margin
        rect.setTop(rect.top() + titleHeight);
    }
    
    return rect;
}

void TitledWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    // Draw the background (without antialiasing)
    painter.fillRect(rect(), m_backgroundColor);

    // Draw the background icon if it exists
    if (!m_backgroundIcon.isNull()) {
        QRect contentArea = contentRect();
        
        // Calculate the icon position based on alignment
        QRect iconRect;
        
        // Horizontal calculation
        if (m_backgroundIconAlignment & Qt::AlignLeft)
            iconRect.setLeft(contentArea.left() + 10);
        else if (m_backgroundIconAlignment & Qt::AlignRight)
            iconRect.setLeft(contentArea.right() - m_backgroundIconSize.width() - 10);
        else // AlignHCenter or default
            iconRect.setLeft(contentArea.left() + (contentArea.width() - m_backgroundIconSize.width()) / 2);
        
        // Vertical calculation
        if (m_backgroundIconAlignment & Qt::AlignTop)
            iconRect.setTop(contentArea.top() + 10);
        else if (m_backgroundIconAlignment & Qt::AlignBottom)
            iconRect.setTop(contentArea.bottom() - m_backgroundIconSize.height() - 10);
        else // AlignVCenter or default
            iconRect.setTop(contentArea.top() + (contentArea.height() - m_backgroundIconSize.height()) / 2);
        
        iconRect.setSize(m_backgroundIconSize);
        
        // Draw the icon with the configured opacity
        painter.setOpacity(m_backgroundIconOpacity);
        m_backgroundIcon.paint(&painter, iconRect);
        painter.setOpacity(1.0);
    }

    // Draw the title if it is visible
    if (m_titleVisible && !m_title.isEmpty()) {
        QFont titleFont = font();
        titleFont.setPointSize(11); // Small size for the title
        painter.setFont(titleFont);

        QFontMetrics fm(titleFont);
        int textWidth = fm.horizontalAdvance(m_title) + 10; // Text width + margin
        int textHeight = fm.height() + 4; // Text height + margin
        
        // Adjust the width of the title area if a companion widget is present
        int titleBoxWidth = textWidth;
        if (m_titleCompanionWidget && m_titleCompanionWidget->isVisible()) {
            titleBoxWidth += m_titleCompanionWidget->width() + 15;  // Widget width + margin
            m_titleCompanionWidget->setGeometry(
                textWidth + 1,  // Position X: after the title with a margin
                1,              // Position Y: vertically centered in the title bar
                m_titleCompanionWidget->sizeHint().width(),
                textHeight      // Slightly reduced height for aesthetics
            );
        }


        // Draw the title background
        QRect titleRect(0, 0, textWidth, textHeight);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(160, 160, 160), 1)); // Use the title color for the border
        painter.drawRect(titleRect);

        // Re-enable antialiasing for text
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawText(QRect(0, 0, textWidth, textHeight), Qt::AlignCenter, m_title);
    }

    // Draw the widget border
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(160, 160, 160), 1)); // Border color and thickness
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1)); // Adjust so border is inside

    // Call the virtual method for specific content
    painter.setRenderHint(QPainter::Antialiasing, true);
    paintContent(painter, contentRect());
}