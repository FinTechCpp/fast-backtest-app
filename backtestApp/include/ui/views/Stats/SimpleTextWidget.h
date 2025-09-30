#pragma once

#include "ui/views/Stats/TitledWidget.h"

class SimpleTextWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit SimpleTextWidget(const QString& title = QString(), QWidget *parent = nullptr);

    // Getters
    QString statText() const { return m_statText; }
    QColor textColor() const { return m_textColor; }
    // QColor backgroundColor() const { return m_backgroundColor; }
    
    // Setters
    void setStatText(const QString &text);
    void setStatColors(const QColor &textColor);
    void setFontSize(int fontSize);
    void setUseBoldFont(bool useBold);
    
    // Configuration supplémentaire
    void setSuffix(const QString &suffix); // Pour ajouter un % ou autre
    
    // Ajout des méthodes pour l'offset de texte
    void setTextOffset(int x, int y);
    void setTextOffsetX(int x);
    void setTextOffsetY(int y);
    int textOffsetX() const { return m_textOffsetX; }
    int textOffsetY() const { return m_textOffsetY; }
    

    // Taille recommandée pour le widget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    static QString formatWithThousandsSeparator(double value, int precision = 2);

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

private:
    QString m_statText;          // Texte affiché au centre
    QColor m_textColor;          // Couleur du texte
    QColor m_backgroundColor;    // Couleur de fond
    QString m_suffix;            // Suffixe (%, x, etc.)
    bool m_useBoldFont;          // Utiliser une police en gras
    int m_fontSize;              // Taille de la police
    int m_textOffsetX;           // Décalage horizontal du texte
    int m_textOffsetY;           // Décalage vertical du texte
};