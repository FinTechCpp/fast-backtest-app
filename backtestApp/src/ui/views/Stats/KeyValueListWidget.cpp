#include "ui/views/Stats/KeyValueListWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>

KeyValueListWidget::KeyValueListWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent)
    , m_keyAlignment(Qt::AlignLeft | Qt::AlignVCenter)
    , m_valueAlignment(Qt::AlignLeft | Qt::AlignVCenter)
    , m_spacing(5)
    , m_keyTextWidth(0)
    , m_checkBoxString("Secondary text")
    , m_showBis(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_fontSize = 12;


    m_checkbox = new QCheckBox(m_checkBoxString);
    m_checkbox->setChecked(m_showBis);

    connect(m_checkbox, &QCheckBox::toggled, this, [this](bool checked) {
        m_showBis = checked;
        update();  // Redraw the widget
    });

    setTitleCompanionWidget(m_checkbox);
    setTitleCompanionWidgetVisible(false);
}

void KeyValueListWidget::addItem(const QString& key, const QString& value, const QColor& color, const QString& valueBis)
{
    m_items.append(KeyValueItem(key, value, color, valueBis));
    
    // Enable checkbox if at least one item has an alternative value
    if (!valueBis.isEmpty()) {
        setTitleCompanionWidgetVisible(true);
    }
    
    update();
}

void KeyValueListWidget::addItems(const QList<KeyValueItem>& items)
{
    m_items.append(items);
    update();
}

void KeyValueListWidget::clear()
{
    m_items.clear();
    update();
}

bool KeyValueListWidget::updateValue(const QString& key, const QString& newValue)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].value = newValue;
            update();
            return true;
        }
    }
    return false;
}

bool KeyValueListWidget::updateValueBis(const QString& key, const QString& newValueBis)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].valueBis = newValueBis;
            
            // Enable checkbox if at least one item has an alternative value
            if (!newValueBis.isEmpty()) {
                setTitleCompanionWidgetVisible(true);
            }
            
            update();
            return true;
        }
    }
    return false;
}

bool KeyValueListWidget::updateValues(const QString& key, const QString& newValue, const QString& newValueBis)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].value = newValue;
            m_items[i].valueBis = newValueBis;
            
            // Enable checkbox if at least one item has an alternative value
            if (!newValueBis.isEmpty()) {
                setTitleCompanionWidgetVisible(true);
            }
            
            update();
            return true;
        }
    }
    return false;
}

bool KeyValueListWidget::updateColor(const QString& key, const QColor& newColor)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].textColor = newColor;
            update();
            return true;
        }
    }
    return false;
}

void KeyValueListWidget::setCheckBoxBisString(const QString &text)
{
    if (m_checkBoxString != text) {
        m_checkBoxString = text;
        m_checkbox->setText(text);
        setTitleCompanionWidgetVisible(true);
        update();
    }
}

void KeyValueListWidget::setKeyAlignment(Qt::Alignment alignment)
{
    if (m_keyAlignment != alignment) {
        m_keyAlignment = alignment;
        update();
    }
}

void KeyValueListWidget::setValueAlignment(Qt::Alignment alignment)
{
    if (m_valueAlignment != alignment) {
        m_valueAlignment = alignment;
        update();
    }
}

void KeyValueListWidget::setSpacing(int spacing)
{
    if (m_spacing != spacing && spacing >= 0) {
        m_spacing = spacing;
        update();
    }
}

void KeyValueListWidget::setKeyTextWidth(int width)
{
    if (m_keyTextWidth != width && width >= 0) {
        m_keyTextWidth = width;
        update();
    }
}

QSize KeyValueListWidget::sizeHint() const
{
    if (m_items.isEmpty()) {
        return QSize(250, 150); // Default size when empty
    }
    
    // Recompute size based on content
    QFont font;
    font.setPointSize(m_fontSize);
    font.setWeight(m_fontWeight);

    QFontMetrics fm(font);
    
    int maxKeyWidth = calculateMaxKeyWidth(fm);
    int separatorWidth = fm.horizontalAdvance(": ");
    
    int maxValueWidth = 0;
    for (const auto& item : m_items) {
        // Consider both values for size calculation
        int valueWidth = fm.horizontalAdvance(item.value);
        if (!item.valueBis.isEmpty()) {
            valueWidth = std::max(valueWidth, fm.horizontalAdvance(item.valueBis));
        }
        maxValueWidth = std::max(maxValueWidth, valueWidth);
    }
    
    int contentWidth = maxKeyWidth + separatorWidth + maxValueWidth;
    int contentHeight = m_items.count() * fm.height() + (m_items.count() - 1) * m_spacing;
    
    // Add margins around the content
    contentWidth += 40;  // 20px on each side
    contentHeight += 40; // 20px top and bottom
    
    return QSize(contentWidth, contentHeight);
}

QSize KeyValueListWidget::minimumSizeHint() const
{
    QSize hint = sizeHint();
    return QSize(hint.width() / 2, hint.height() / 2);
}

int KeyValueListWidget::calculateMaxKeyWidth(const QFontMetrics& fm) const
{
    // If a fixed width is specified, use it
    if (m_keyTextWidth > 0) {
        return m_keyTextWidth;
    }
    
    int maxWidth = 0;
    for (const auto& item : m_items) {
        int width = fm.horizontalAdvance(item.key);
        maxWidth = std::max(maxWidth, width);
    }
    return maxWidth;
}

void KeyValueListWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    // Nothing to draw if the list is empty
    if (m_items.isEmpty()) {
        return;
    }
    
    // Configure font with custom size
    QFont font = painter.font();
    font.setPointSize(m_fontSize);
    font.setWeight(m_fontWeight);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    int lineHeight = fm.height();
    int maxKeyWidth = calculateMaxKeyWidth(fm);
    
    // Separator width ": "
    int separatorWidth = fm.horizontalAdvance(": ");
    
    // Calculate total width of the content block
    int contentWidth = maxKeyWidth + separatorWidth;
    
    // Find the widest value to determine total width
    int maxValueWidth = 0;
    for (const auto& item : m_items) {
        // Check which value to display according to m_showBis
        QString displayValue = (m_showBis && !item.valueBis.isEmpty()) ? item.valueBis : item.value;
        maxValueWidth = std::max(maxValueWidth, fm.horizontalAdvance(displayValue));
    }
    contentWidth += maxValueWidth;
    
    // Calculate total height of the content block
    int contentHeight = m_items.count() * lineHeight + (m_items.count() - 1) * m_spacing;
    
    // Calculate starting positions for centering with minimum safe margins
    int marginX = 10; // Minimum horizontal margin
    int marginY = 10; // Minimum vertical margin
    
    // Center content but ensure it does not exceed margins
    int startX = qMax(marginX, (contentRect.left() + contentRect.width() - contentWidth) / 2);
    int startY = qMax(marginY, (contentRect.top() + contentRect.height() - contentHeight) / 2);
    
    // Current y position
    int y = startY + lineHeight/2; // Start with some spacing for the first line
    
    // Draw each line
    for (const auto& item : m_items) {
        // Text color for this line
        painter.setPen(item.textColor);
        
        // Determine which value to display
        QString displayValue = (m_showBis && !item.valueBis.isEmpty()) ? item.valueBis : item.value;
        
        // Rectangle for the key, ensured to be within bounds
        QRect keyRect(startX, y - lineHeight/2, maxKeyWidth, lineHeight);
        
        // Rectangle for the separator
        QRect separatorRect(keyRect.right(), y - lineHeight/2, separatorWidth, lineHeight);
        
        // Rectangle for the value
        QRect valueRect(separatorRect.right(), y - lineHeight/2, 
                       maxValueWidth, lineHeight);
        
        // Draw the key right-aligned for better alignment with the separator
        painter.drawText(keyRect, Qt::AlignRight | Qt::AlignVCenter, item.key);
        
        // Draw the separator
        painter.drawText(separatorRect, Qt::AlignCenter, ": ");
        
        // Draw the value (primary or alternative based on m_showBis)
        painter.drawText(valueRect, m_valueAlignment, displayValue);
        
        // Move to the next line
        y += lineHeight + m_spacing;
    }
}