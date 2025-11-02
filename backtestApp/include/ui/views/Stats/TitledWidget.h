#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QIcon>

class TitledWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitledWidget(const QString& title = QString(), QWidget *parent = nullptr);
    
    // Accessors for the title
    QString title() const { return m_title; }
    void setTitle(const QString& title);
    
    // Title customization
    void setTitleColor(const QColor& color);
    void setTitleBackgroundColor(const QColor& color);
    void setTitleVisible(bool visible);
    void setBackgroundColor(const QColor& color);

    void setFontWeight(QFont::Weight weight); // QFont::Thin to QFont::Black
    QFont::Weight fontWeight() const { return m_fontWeight; }

    void setFontSize(int fontSize);
    int getFontSize() const { return m_fontSize; }
    
    // Companion widget next to the title
    void setTitleCompanionWidget(QWidget* widget);
    void setTitleCompanionWidgetVisible(bool visible);
    QWidget* titleCompanionWidget() const { return m_titleCompanionWidget; }


    // Adding methods for the background icon
    void setBackgroundIcon(const QIcon& icon);
    QIcon backgroundIcon() const { return m_backgroundIcon; }
    
    // Customization options for the icon
    void setBackgroundIconSize(const QSize& size);
    QSize backgroundIconSize() const { return m_backgroundIconSize; }
    void setBackgroundIconAlignment(Qt::Alignment alignment);
    Qt::Alignment backgroundIconAlignment() const { return m_backgroundIconAlignment; }
    void setBackgroundIconOpacity(qreal opacity);
    qreal backgroundIconOpacity() const { return m_backgroundIconOpacity; }


protected:
    void paintEvent(QPaintEvent* event) override;
    
    // Method for derived classes that need to draw their own content
    virtual void paintContent(QPainter& painter, const QRect& contentRect) = 0;
    
    // Calculates the area available for content after drawing the title
    QRect contentRect() const;
    
    QFont::Weight m_fontWeight;  // Font weight
    int m_fontSize;              // Font size
private:
    QString m_title;
    QColor m_titleColor;
    QColor m_titleBackgroundColor;
    QColor m_backgroundColor;
    bool m_titleVisible;
    
    // Companion widget that will be displayed next to the title
    QWidget* m_titleCompanionWidget;

    // Background icon and its properties
    QIcon m_backgroundIcon;
    QSize m_backgroundIconSize;
    Qt::Alignment m_backgroundIconAlignment;
    qreal m_backgroundIconOpacity;
};