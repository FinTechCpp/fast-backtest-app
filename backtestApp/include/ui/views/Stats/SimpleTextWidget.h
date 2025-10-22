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
    
    // Configuration supplémentaire
    void setSuffix(const QString &suffix); // Pour ajouter un % ou autre
    void setSuffixBis(const QString &suffix); // Pour le texte secondaire
    
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
    QString m_checkBoxString;    // Texte de la checkbox pour le texte secondaire
    QCheckBox* m_checkBox;       // Checkbox pour afficher/masquer le texte secondaire

    QString m_statText;          // Texte affiché au centre
    QString m_statTextBis;       // Texte secondaire (optionnel)

    QColor m_textColor;          // Couleur du texte
    QColor m_backgroundColor;    // Couleur de fond

    QString m_suffix;            // Suffixe (%, x, etc.)
    QString m_suffixBis;         // Suffixe secondaire (optionnel)
    
    int m_textOffsetX;           // Décalage horizontal du texte
    int m_textOffsetY;           // Décalage vertical du texte

    bool m_showBis;          // Indique si le texte secondaire doit être affiché
};