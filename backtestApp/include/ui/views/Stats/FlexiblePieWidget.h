#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <vector>

// Structure pour définir un segment
struct PieSegment {
    double proportion;  // Proportion relative (sera normalisée)
    QColor color;       // Couleur du segment
    
    PieSegment(double p, const QColor& c) : proportion(p), color(c) {}
};

class FlexiblePieWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlexiblePieWidget(QWidget *parent = nullptr);
    
    // Configuration de base
    void setStartAngle(int degrees); // 0° = est, 90° = haut, 180° = ouest
    void setAngleSpan(int degrees);   // 180° = demi-cercle, 360° = cercle complet
    void setCenterText(const QString &text);
    void setCenterTextColor(const QColor &color);
    void setCenterTextSuffix(const QString &suffix);
    void setCenterTextFontSizeRatio(double ratio); // >1 plus grand, <1 plus petit
    
    // Gestion des segments
    void addSegment(double proportion, const QColor &color);
    void setSegments(const std::vector<PieSegment>& segments);
    void clearSegments();
    const std::vector<PieSegment>& segments() const { return m_segments; }
    
    // Configuration visuelle
    void setInnerCircleRadius(double ratio); // 0.1-0.9, proportion du rayon extérieur
    
    // Taille recommandée
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Configuration géométrique
    int m_startAngle;    // Angle de départ en degrés (x16 pour QPainter)
    int m_angleSpan;      // Angle maximum à parcourir (x16 pour QPainter)
    double m_innerRadiusRatio; // Ratio du rayon intérieur
    
    // Texte central
    QString m_centerText;
    QString m_textSuffix;
    QColor m_textColor;
    double m_fontSizeRatio;
    
    // Segments
    std::vector<PieSegment> m_segments;
};