#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLinearGradient>
#include "ui/views/Stats/TitledWidget.h"

#include "stats.hpp"

// On peut calculer le ratio gain perte et le mettre au dessus du widget pk pas

class VerticalGaugeRenderWidget : public TitledWidget {
    Q_OBJECT
    
public:
    explicit VerticalGaugeRenderWidget(const QString& title = QString(), QWidget* parent = nullptr);

    void updateContent(const be::Stats& stats);
    void clear();
    
protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    
private:
    // Conversion valeur -> position Y
    int valueToY(double value, double minValue, double maxValue, int height);
    
    // Valeurs à afficher
    double m_tpAvg;     // Moyenne des trades gagnants
    double m_tpAvgPrc;  // Moyenne des trades gagnants en %
    double m_tpMax;     // Maximum des trades gagnants
    double m_tpMaxPrc;  // Maximum des trades gagnants en %
    double m_tpMedian;  // Médiane des trades gagnants
    double m_tpMedianPrc;  // Médiane des trades gagnants en %

    double m_slAvg;     // Moyenne des trades perdants
    double m_slAvgPrc;  // Moyenne des trades perdants en %
    double m_slMin;     // Minimum des trades perdants
    double m_slMinPrc;  // Minimum des trades perdants en %
    double m_slMedian;  // Médiane des trades perdants
    double m_slMedianPrc;  // Médiane des trades perdants en %
    
    // Couleurs
    QColor m_tpAvgColor;    // Vert foncé pour la moyenne des TP
    QColor m_tpMaxColor;    // Vert clair pour le max des TP
    QColor m_slAvgColor;    // Rouge foncé pour la moyenne des SL
    QColor m_slMinColor;    // Rouge clair pour le min des SL
    QColor m_medianTpColor; // Couleur de la ligne de médiane TP
    QColor m_medianSlColor; // Couleur de la ligne de médiane SL
    QColor m_lineColor;     // Couleur des lignes
    QColor m_textColor;     // Couleur du texte
};