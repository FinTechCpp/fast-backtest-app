#pragma once

#include "ui/views/Stats/TitledWidget.h"
#include <QList>
#include <QCheckBox>

// Structure to store information for each row
struct KeyValueItem {
    QString key;        // The left text (key)
    QString value;      // The primary value on the right
    QString valueBis;   // The alternative value (optional)
    QColor textColor;   // Text color
    
    KeyValueItem(const QString& k = QString(), 
                const QString& v = QString(), 
                const QColor& c = Qt::black,
                const QString& vBis = QString())
        : key(k), value(v), valueBis(vBis), textColor(c) {}
};

class KeyValueListWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit KeyValueListWidget(const QString& title = QString(), QWidget* parent = nullptr);
    
    // Add an entry to the list
    void addItem(const QString& key, const QString& value, const QColor& color = Qt::black, const QString& valueBis = QString());
    
    // Add multiple entries at once
    void addItems(const QList<KeyValueItem>& items);
    
    // Remove all entries
    void clear();
    
    // Update an existing entry (primary value)
    bool updateValue(const QString& key, const QString& newValue);
    
    // Update the alternative value
    bool updateValueBis(const QString& key, const QString& newValueBis);
    
    // Update both values at the same time
    bool updateValues(const QString& key, const QString& newValue, const QString& newValueBis);
    
    // Update the color of an entry
    bool updateColor(const QString& key, const QColor& newColor);
    
    // Access to elements
    QList<KeyValueItem> items() const { return m_items; }
    int count() const { return m_items.count(); }
    
    // Visual configuration
    void setCheckBoxBisString(const QString &text);
    void setKeyAlignment(Qt::Alignment alignment);
    void setValueAlignment(Qt::Alignment alignment);
    void setSpacing(int spacing);
    void setKeyTextWidth(int width); // Force a specific width (0 = auto)
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

private:
    QString m_checkBoxString;
    QCheckBox* m_checkbox;
    bool m_showBis;

    QList<KeyValueItem> m_items;
    Qt::Alignment m_keyAlignment;
    Qt::Alignment m_valueAlignment;
    int m_spacing;
    int m_keyTextWidth;  // Fixed width for keys (0 = auto)
    
    // Calculate the maximum width of keys for alignment
    int calculateMaxKeyWidth(const QFontMetrics& fm) const;
};