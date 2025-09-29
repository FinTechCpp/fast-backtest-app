#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>

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

protected:
    void paintEvent(QPaintEvent* event) override;
    
    // Méthode pour les classes dérivées qui doivent dessiner leur propre contenu
    virtual void paintContent(QPainter& painter, const QRect& contentRect) = 0;
    
    // Calcule la zone disponible pour le contenu après avoir dessiné le titre
    QRect contentRect() const;

private:
    QString m_title;
    QColor m_titleColor;
    QColor m_titleBackgroundColor;
    QColor m_backgroundColor;
    bool m_titleVisible;
};