#pragma once

#include "ui/views/Stats/TitledWidget.h"
#include <QCheckBox>

class SimpleTextWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit SimpleTextWidget(const QString& title = QString(), QWidget *parent = nullptr);

    // Getters
    QString statText() const { return m_statText; }
    QString statTextBis() const { return m_statTextBis; }
    bool showStatTextBis() const { return m_showBis; }
    QColor textColor() const { return m_textColor; }
    // QColor backgroundColor() const { return m_backgroundColor; }
    
    // Setters
    void setStatText(const QString &text);
    void setCheckBoxBisString(const QString &text);
    void setStatTextBis(const QString &text);
    void setStatColors(const QColor &textColor);
    
    // Additional configuration
    void setSuffix(const QString &suffix); // To add a % or other suffix
    void setSuffixBis(const QString &suffix); // For the secondary text
    
    // Adding methods for text offset
    void setTextOffset(int x, int y);
    void setTextOffsetX(int x);
    void setTextOffsetY(int y);
    int textOffsetX() const { return m_textOffsetX; }
    int textOffsetY() const { return m_textOffsetY; }
    

    // Recommended size for the widget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    static QString formatWithThousandsSeparator(double value, int precision = 2);

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

private:
    QString m_checkBoxString;    // Checkbox text for the secondary text
    QCheckBox* m_checkBox;       // Checkbox to show/hide the secondary text

    QString m_statText;          // Text displayed in the center
    QString m_statTextBis;       // Secondary text (optional)

    QColor m_textColor;          // Text color
    QColor m_backgroundColor;    // Background color

    QString m_suffix;            // Suffix (%, x, etc.)
    QString m_suffixBis;         // Secondary suffix (optional)
    
    int m_textOffsetX;           // Horizontal text offset
    int m_textOffsetY;           // Vertical text offset

    bool m_showBis;              // Indicates if the secondary text should be displayed
};