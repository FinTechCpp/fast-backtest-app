#pragma once

#include <QWidget>
#include <QString>
#include <QColor>

class SimpleTextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimpleTextWidget(QWidget *parent = nullptr);

    // Getters
    QString statText() const { return m_statText; }
    QColor textColor() const { return m_textColor; }
    QColor backgroundColor() const { return m_backgroundColor; }
    
    // Setters
    void setStatText(const QString &text);
    void setColors(const QColor &textColor, const QColor &backgroundColor);
    
    // Configuration supplémentaire
    void setSuffix(const QString &suffix); // Pour ajouter un % ou autre
    
    // Taille recommandée pour le widget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    static QString formatWithThousandsSeparator(double value, int precision = 2);

protected:
    void paintEvent(QPaintEvent *event) override;

private:

    QString m_statText;          // Texte affiché au centre
    QColor m_textColor;         // Couleur du texte
    QColor m_backgroundColor;   // Couleur de fond
    QString m_suffix;            // Suffixe (%, x, etc.)
};