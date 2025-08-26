#pragma once

#include <QPainter>
#include <QColor>
#include <QDateTime>

#include "date.hpp"
#include "ui/views/Stats/StatsBaseWidget.h"


class TimelineWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget* parent = nullptr);

    void updateContent(const be::Stats& stats) override;
    void clear() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    QString m_startDate;
    QString m_endDate;
    QString m_duration;
    double m_exposureTimePct = 0.0;
    
    // Couleurs
    QColor m_timelineColor = QColor(200, 200, 200);        // Gris clair pour la timeline
    QColor m_exposureColor = QColor(52, 152, 219, 180);    // Bleu semi-transparent pour l'exposition
    QColor m_textColor = QColor(70, 70, 70);               // Gris foncé pour le texte
    
    // Dimensions
    int m_timelineHeight = 12;                             // Hauteur de la barre de timeline
    int m_timelineRadius = 6;                              // Rayon des extrémités arrondies
};