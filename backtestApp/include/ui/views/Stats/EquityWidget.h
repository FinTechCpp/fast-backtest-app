#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QCheckBox>
#include "ui/views/Stats/TitledWidget.h"
#include "stats.hpp"


// TODO : il faut donner au widget les date relatives aux points X pour afficher les labels d'axe X correctement (dates et plus indices des points)
// IL FAUT AJOUTER DANS LES stats le calcule de l'equity curve sous forme de pnl en POURCENT et afficher l'equity curve en pourcent
class EquityWidget : public TitledWidget
{
    Q_OBJECT
public:
    explicit EquityWidget(const QString& title = QString(), QWidget *parent = nullptr);


    // Replace points (will be sorted by X and filtered for consecutive duplicates in Y)
    void setPoints(const QVector<QPointF>& pts);
    void setPoints(const std::vector<be::Date>& dates, const std::vector<be::EquityPoint>& equityCurve);
    QVector<QPointF> points() const { return m_points; }

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;


private:
    struct DateLabel {
        double position;  // Position X sur l'axe
        QString text;     // Texte à afficher
        int importance;   // Niveau d'importance: 3=année, 2=mois, 1=jour
    };

    QCheckBox* m_checkBox;

    QVector<QPointF> m_points;
    QVector<QPointF> m_pointsPercent;
    std::vector<be::Date> m_dates;  // Dates correspondant aux points
    
    std::vector<DateLabel> generateDateLabels() const;
    void calculatePercentPoints(); // Calcule les points en pourcentage
    void invalidateCache();         // Invalide le cache quand les données changent


    void updateBounds();


    // mouse crosshair
    bool m_showCrosshair;
    QPoint m_mousePos; // in widget pixel coordinates

    // drawing helpers
    void drawGrid(QPainter &painter);
    void drawAxes(QPainter &painter);
    void drawFilledAreas(QPainter &painter);    // Dessine les zones colorées sous la courbe
    void drawEquityMarkers(QPainter &painter);  // Dessine les lignes initial/peak et highlight final
    QPointF mapToWidget(const QPointF &pt) const;
    QPointF mapToWorld(const QPointF &pixel) const;
    const QVector<QPointF>& getCachedWidgetPoints() const;  // Retourne les points en coordonnées widget (avec cache)
    
    // Méthodes helper pour les markers
    double getInitialEquity() const;
    double getPeakEquity() const;
    double getFinalEquity() const;

    QString formatValue(double value, bool useThousandsSeparator = true, bool isPercent = false, bool roundValue = true) const;


    double m_xmin, m_xmax, m_ymin, m_ymax;
    QRect m_contentRect;      // Zone totale du contenu (titre exclu)
    QRect m_plotRect;         // Zone du graphique uniquement (sans les marges pour axes)

    // Cache pour optimisation
    mutable bool m_cacheValid = false;
    mutable double m_cachedInitialEquity = 0.0;
    mutable double m_cachedPeakEquity = 0.0;
    mutable double m_cachedFinalEquity = 0.0;
    mutable std::vector<DateLabel> m_cachedDateLabels;
    mutable QVector<QPointF> m_cachedWidgetPoints;  // Points déjà convertis en coordonnées widget
    mutable bool m_widgetPointsValid = false;
    mutable QSize m_cachedPlotSize;  // Taille du plotRect pour détecter les changements

    // layout margins to leave space for axis labels
    const int m_leftMargin = 10;      // Réduit car plus de labels à gauche
    const int m_bottomMargin = 30;
    const int m_rightMargin = 80;     // Augmenté pour les labels Y à droite
    const int m_topMargin = 10;
    const int m_margin = 5;
};
