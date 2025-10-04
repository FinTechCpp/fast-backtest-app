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
    
    // Accesseurs pour le titre
    QString title() const { return m_title; }
    void setTitle(const QString& title);
    
    // Personnalisation du titre
    void setTitleColor(const QColor& color);
    void setTitleBackgroundColor(const QColor& color);
    void setTitleVisible(bool visible);
    void setBackgroundColor(const QColor& color);

    void setFontWeight(QFont::Weight weight); // QFont::Thin à QFont::Black
    QFont::Weight fontWeight() const { return m_fontWeight; }

    void setFontSize(int fontSize);
    int getFontSize() const { return m_fontSize; }
    
    // Widget compagnon à côté du titre
    void setTitleCompanionWidget(QWidget* widget);
    void setTitleCompanionWidgetVisible(bool visible);
    QWidget* titleCompanionWidget() const { return m_titleCompanionWidget; }


    // Ajout des méthodes pour l'icône de fond
    void setBackgroundIcon(const QIcon& icon);
    QIcon backgroundIcon() const { return m_backgroundIcon; }
    
    // Options de personnalisation de l'icône
    void setBackgroundIconSize(const QSize& size);
    QSize backgroundIconSize() const { return m_backgroundIconSize; }
    void setBackgroundIconAlignment(Qt::Alignment alignment);
    Qt::Alignment backgroundIconAlignment() const { return m_backgroundIconAlignment; }
    void setBackgroundIconOpacity(qreal opacity);
    qreal backgroundIconOpacity() const { return m_backgroundIconOpacity; }


protected:
    void paintEvent(QPaintEvent* event) override;
    
    // Méthode pour les classes dérivées qui doivent dessiner leur propre contenu
    virtual void paintContent(QPainter& painter, const QRect& contentRect) = 0;
    
    // Calcule la zone disponible pour le contenu après avoir dessiné le titre
    QRect contentRect() const;
    
    QFont::Weight m_fontWeight;  // Poids de la police
    int m_fontSize;              // Taille de la police
private:
    QString m_title;
    QColor m_titleColor;
    QColor m_titleBackgroundColor;
    QColor m_backgroundColor;
    bool m_titleVisible;
    
    // Widget compagnon qui sera affiché à côté du titre
    QWidget* m_titleCompanionWidget;

    // Icône de fond et ses propriétés
    QIcon m_backgroundIcon;
    QSize m_backgroundIconSize;
    Qt::Alignment m_backgroundIconAlignment;
    qreal m_backgroundIconOpacity;
};