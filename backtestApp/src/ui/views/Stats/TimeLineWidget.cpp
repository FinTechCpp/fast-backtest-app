#include "ui/views/Stats/TimeLineWidget.h"

TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    // Configuration du widget
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void TimelineWidget::setData(const be::Date& start, const be::Date& end, const be::Duration& duration, double exposurePercent) {
    m_startDate = QString::fromStdString(start.toString());
    m_endDate = QString::fromStdString(end.toString());
    m_duration = QString::fromStdString(duration.toString());
    m_exposurePercent = std::min(100.0, std::max(0.0, exposurePercent));

    update();
}

void TimelineWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Récupérer les dimensions du widget
    const int width = this->width();
    const int height = this->height();
    
    // Définir les marges
    const int marginX = 20;
    const int marginY = 40; // Marge supérieure pour les dates
    
    // Définir les positions de la timeline
    const int timelineY = marginY;
    const int timelineWidth = width - 2 * marginX;
    
    // Dessiner la timeline de base (rectangle gris avec coins arrondis)
    QPen timelinePen(Qt::NoPen);
    QBrush timelineBrush(m_timelineColor);
    painter.setPen(timelinePen);
    painter.setBrush(timelineBrush);
    
    QRectF timelineRect(marginX, timelineY, timelineWidth, m_timelineHeight);
    painter.drawRoundedRect(timelineRect, m_timelineRadius, m_timelineRadius);
    
    // Dessiner la partie "exposure time" (partie bleue)
    if (m_exposurePercent > 0.0) {
        QBrush exposureBrush(m_exposureColor);
        painter.setBrush(exposureBrush);
        
        // Calculer la largeur en fonction du pourcentage
        int exposureWidth = static_cast<int>(timelineWidth * m_exposurePercent / 100.0);
        QRectF exposureRect(marginX, timelineY, exposureWidth, m_timelineHeight);
        painter.drawRoundedRect(exposureRect, m_timelineRadius, m_timelineRadius);
    }
    
    // Ajouter des marqueurs de début et fin de timeline
    const int markerSize = 10;
    QPen markerPen(m_textColor, 2);
    painter.setPen(markerPen);
    painter.setBrush(Qt::white);
    
    // Début de timeline (cercle)
    painter.drawEllipse(QPointF(marginX, timelineY + m_timelineHeight/2), markerSize/2, markerSize/2);
    
    // Fin de timeline (cercle)
    painter.drawEllipse(QPointF(marginX + timelineWidth, timelineY + m_timelineHeight/2), markerSize/2, markerSize/2);
    
    // Ajouter les dates de début et fin
    QFont dateFont = painter.font();
    dateFont.setBold(true);
    painter.setFont(dateFont);
    painter.setPen(m_textColor);
    
    QFontMetrics fm(dateFont);
    
    // Date de début (alignée à gauche)
    int startTextWidth = fm.horizontalAdvance(m_startDate);
    painter.drawText(QRectF(marginX - startTextWidth/2, timelineY - 25, startTextWidth*2, 20), 
                    Qt::AlignHCenter, m_startDate);
    
    // Date de fin (alignée à droite) - CORRIGÉ
    int endTextWidth = fm.horizontalAdvance(m_endDate);
    painter.drawText(QRectF(marginX + timelineWidth - endTextWidth, timelineY - 25, endTextWidth*2, 20), 
                    Qt::AlignLeft, m_endDate);
    
    // Ajouter la durée totale au centre sous la timeline - CORRIGÉ
    QFont durationFont = dateFont;
    durationFont.setPointSize(durationFont.pointSize() + 1);
    painter.setFont(durationFont);
    
    // Créer un label explicite pour la durée
    QString durationText = "Durée totale: " + m_duration;
    // Utiliser toute la largeur du widget pour permettre le centrage
    painter.drawText(QRectF(0, timelineY + m_timelineHeight + 15, width, 20),
                    Qt::AlignHCenter, durationText);
    
    // Ajouter le pourcentage d'exposition sous la durée - CORRIGÉ
    QFont exposureFont = painter.font();
    exposureFont.setPointSize(exposureFont.pointSize() - 1);
    painter.setFont(exposureFont);
    
    QString exposureText = QString("Exposition en marché: %1%").arg(QString::number(m_exposurePercent, 'f', 2));
    // Utiliser toute la largeur du widget pour permettre le centrage
    painter.drawText(QRectF(0, timelineY + m_timelineHeight + 40, width, 20),
                    Qt::AlignHCenter, exposureText);
}