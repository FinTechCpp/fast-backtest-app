#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>

class BaseRatioWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseRatioWidget(QWidget *parent = nullptr, 
                            double fontSizeRatio = 1.0,
                            double innerRadiusRatio = 0.7);
    
    // Getters communs
    QString statText() const { return m_statText; }
    double proportion() const { return m_proportion; }
    QColor positiveColor() const { return m_positiveColor; }
    QColor negativeColor() const { return m_negativeColor; }
    
    // Nouveaux getters
    double fontSizeRatio() const { return m_fontSizeRatio; }
    double innerRadiusRatio() const { return m_innerRadiusRatio; }
    
    // Setters communs
    void setStatText(const QString &text);
    void setProportion(double proportion); // Entre 0.0 et 1.0
    void setColors(const QColor &positiveColor, const QColor &negativeColor, const QColor &statTextColor = QColor(0, 0, 0));
    
    // Nouveaux setters
    void setFontSizeRatio(double ratio);
    void setInnerRadiusRatio(double ratio); // Entre 0.1 et 0.9
    
    // Configuration supplémentaire
    void setSuffix(const QString &suffix); // Pour ajouter un % ou autre
    
    // Taille recommandée pour le widget (à implémenter par les classes dérivées)
    virtual QSize sizeHint() const override = 0;
    virtual QSize minimumSizeHint() const override = 0;

protected:
    void paintEvent(QPaintEvent *event) override;
    
    // Méthode virtuelle pure à implémenter par les classes dérivées
    virtual void drawGauge(QPainter &painter, const QRectF &outerRect, 
                          const QRectF &innerRect, int radius) = 0;
    
    QString m_statText;        // Texte affiché au centre
    double m_proportion;       // Proportion positive (0.0-1.0)
    QColor m_statTextColor;    // Couleur du texte
    QColor m_positiveColor;    // Couleur pour la partie positive
    QColor m_negativeColor;    // Couleur pour la partie négative
    QString m_suffix;          // Suffixe (%, x, etc.)
    double m_fontSizeRatio;    // Ratio pour la taille de police
    double m_innerRadiusRatio; // Ratio du rayon intérieur par rapport à l'extérieur
};