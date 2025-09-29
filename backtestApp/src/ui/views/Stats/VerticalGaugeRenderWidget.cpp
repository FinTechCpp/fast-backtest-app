#include "ui/views/Stats/VerticalGaugeRenderWidget.h"
#include <algorithm>
#include <numeric>
#include <QDebug>
#include <QGroupBox>
#include <QFontMetrics>


VerticalGaugeRenderWidget::VerticalGaugeRenderWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent),
      m_tpAvg(0.0),
      m_tpMax(0.0),
      m_tpMedian(0.0),
      m_slAvg(0.0),
      m_slMin(0.0),
      m_slMedian(0.0),
      m_tpAvgPrc(0.0),
      m_tpMaxPrc(0.0),
      m_tpMedianPrc(0.0),
      m_slAvgPrc(0.0),
      m_slMinPrc(0.0),
      m_slMedianPrc(0.0)
{
    // Initialiser les couleurs
    m_tpAvgColor = QColor(0, 150, 0);       // Vert foncé
    m_tpMaxColor = QColor(100, 255, 100);   // Vert clair
    m_slAvgColor = QColor(150, 0, 0);       // Rouge foncé
    m_slMinColor = QColor(255, 100, 100);   // Rouge clair
    m_lineColor = QColor(0, 0, 0);          // Noir
    m_textColor = QColor(40, 40, 40);       // Gris foncé
    m_medianTpColor = QColor(0, 100, 0);    // Vert foncé
    m_medianSlColor = QColor(100, 0, 0);    // Rouge foncé
    
    // Définir la taille et la politique de taille - plus large pour inclure la légende
    setMinimumSize(220, 300);  // Augmenté de 40 à 150 pour avoir de l'espace pour la légende
    setMaximumWidth(240);      // Augmenté de 60 à 180
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void VerticalGaugeRenderWidget::updateContent(const be::Stats& stats)
{
    if (stats.trades.empty()) {
        clear();
        return;
    }
    
    // Collecter les PL des trades en fonction de leur raison de fermeture
    std::vector<double> tpPLs;
    std::vector<double> tpPLsPrc;
    std::vector<double> slPLs;
    std::vector<double> slPLsPrc;

    const be::TradeData* bestTrade = nullptr;
    const be::TradeData* worstTrade = nullptr;
    
    for (const auto& trade : stats.trades) {
        // Trouver le meilleur et le pire trade
        if (!bestTrade || trade.pl > bestTrade->pl) {
            bestTrade = &trade;
        }
        if (!worstTrade || trade.pl < worstTrade->pl) {
            worstTrade = &trade;
        }
        
        // Collecter les données par type de trade
        if (trade.closeReason == be::CloseReason::TakeProfit) {
            tpPLs.push_back(trade.pl);
            tpPLsPrc.push_back(trade.plPercent);
        } else if (trade.closeReason == be::CloseReason::StopLoss) {
            slPLs.push_back(trade.pl);
            slPLsPrc.push_back(trade.plPercent);
        }
    }
    
    // Stocker le nombre de trades
    size_t tpCount = tpPLs.size();
    size_t slCount = slPLs.size();

    // Calcul des statistiques pour les trades gagnants (TP)
    if (!tpPLs.empty()) {
        m_tpAvg = std::accumulate(tpPLs.begin(), tpPLs.end(), 0.0) / double(tpCount);
        m_tpAvgPrc = std::accumulate(tpPLsPrc.begin(), tpPLsPrc.end(), 0.0) / double(tpPLsPrc.size());
        m_tpMax = *std::max_element(tpPLs.begin(), tpPLs.end());
        m_tpMaxPrc = *std::max_element(tpPLsPrc.begin(), tpPLsPrc.end());

        // Calculer la médiane des trades TP
        std::vector<double> sortedTpPLs = tpPLs;
        std::vector<double> sortedTpPLsPrc = tpPLsPrc;
        std::sort(sortedTpPLs.begin(), sortedTpPLs.end());
        std::sort(sortedTpPLsPrc.begin(), sortedTpPLsPrc.end());

        if (sortedTpPLs.size() % 2 == 0) {
            m_tpMedian = (sortedTpPLs[sortedTpPLs.size() / 2 - 1] + 
                         sortedTpPLs[sortedTpPLs.size() / 2]) / 2.0;
            m_tpMedianPrc = (sortedTpPLsPrc[sortedTpPLsPrc.size() / 2 - 1] + 
                            sortedTpPLsPrc[sortedTpPLsPrc.size() / 2]) / 2.0;
        } else {
            m_tpMedian = sortedTpPLs[sortedTpPLs.size() / 2];
            m_tpMedianPrc = sortedTpPLsPrc[sortedTpPLsPrc.size() / 2];
        }
    } else {
        m_tpAvg = 0.0;
        m_tpAvgPrc = 0.0;
        m_tpMax = 0.0;
        m_tpMaxPrc = 0.0;
        m_tpMedian = 0.0;
        m_tpMedianPrc = 0.0;
    }
    
    // Calcul des statistiques pour les trades perdants (SL)
    if (!slPLs.empty()) {
        m_slAvg = std::accumulate(slPLs.begin(), slPLs.end(), 0.0) / slPLs.size();
        m_slAvgPrc = std::accumulate(slPLsPrc.begin(), slPLsPrc.end(), 0.0) / slPLsPrc.size();
        m_slMin = *std::min_element(slPLs.begin(), slPLs.end());
        m_slMinPrc = *std::min_element(slPLsPrc.begin(), slPLsPrc.end());

        // Calculer la médiane des trades SL
        std::vector<double> sortedSlPLs = slPLs;
        std::vector<double> sortedSlPLsPrc = slPLsPrc;
        std::sort(sortedSlPLs.begin(), sortedSlPLs.end());
        std::sort(sortedSlPLsPrc.begin(), sortedSlPLsPrc.end());

        if (sortedSlPLs.size() % 2 == 0) {
            m_slMedian = (sortedSlPLs[sortedSlPLs.size() / 2 - 1] + 
                         sortedSlPLs[sortedSlPLs.size() / 2]) / 2.0;
            m_slMedianPrc = (sortedSlPLsPrc[sortedSlPLsPrc.size() / 2 - 1] + 
                            sortedSlPLsPrc[sortedSlPLsPrc.size() / 2]) / 2.0;
        } else {
            m_slMedian = sortedSlPLs[sortedSlPLs.size() / 2];
            m_slMedianPrc = sortedSlPLsPrc[sortedSlPLsPrc.size() / 2];
        }
    } else {
        m_slAvg = 0.0;
        m_slAvgPrc = 0.0;
        m_slMin = 0.0;
        m_slMinPrc = 0.0;
        m_slMedian = 0.0;
        m_slMedianPrc = 0.0;
    }
    
    update();
}

void VerticalGaugeRenderWidget::clear()
{
    m_tpAvg = 0.0;
    m_tpMax = 0.0;
    m_tpMedian = 0.0;
    m_slAvg = 0.0;
    m_slMin = 0.0;
    m_slMedian = 0.0;
    
    m_tpAvgPrc = 0.0;
    m_tpMaxPrc = 0.0;
    m_tpMedianPrc = 0.0;
    m_slAvgPrc = 0.0;
    m_slMinPrc = 0.0;
    m_slMedianPrc = 0.0;
    
    update();
}

int VerticalGaugeRenderWidget::valueToY(double value, double minValue, double maxValue, int height)
{
    // Convertir une valeur en position Y
    double range = maxValue - minValue;
    if (range <= 0) return height / 2;  
    
    double normalizedValue = (value - minValue) / range;
    return height - static_cast<int>(normalizedValue * height);
}

void VerticalGaugeRenderWidget::paintContent(QPainter& painter, const QRect& contentRect)
{    
    painter.setRenderHint(QPainter::Antialiasing, false);

    int w = contentRect.width();
    int h = contentRect.height();
    int gaugeWidth = 45;        // Largeur fixe pour la jauge
    int legendSpace = 60;   // Espace pour les légendes à gauche
    int gaugeX = contentRect.left() + legendSpace + 5;  // Position X de la jauge (décalée pour avoir de l'espace à gauche)
    int padding = contentRect.top() + 10;           // Marge en haut et en bas
    int legendPadding = 10;     // Espace entre la jauge et le début de la légende
    
    // Calculer la plage symétrique autour de zéro
    double maxAbsValue = std::max(std::abs(m_tpMax), std::abs(m_slMin));
    double minValue = -maxAbsValue;
    double maxValue = maxAbsValue;
    
    // Tracer le fond de la jauge
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRect(gaugeX, padding, gaugeWidth, h - 2 * padding);
    
    // Position Y du zéro (milieu de la jauge)
    // int zeroY = padding + (h - 2 * padding) / 2;
    int zeroY = contentRect.top() + (h / 2);
    
    // Calculer les positions Y des valeurs
    int tpMaxY = valueToY(m_tpMax, minValue, maxValue, h - 2 * padding) + padding;
    int tpAvgY = valueToY(m_tpAvg, minValue, maxValue, h - 2 * padding) + padding;
    int tpMedianY = valueToY(m_tpMedian, minValue, maxValue, h - 2 * padding) + padding;
    int slAvgY = valueToY(m_slAvg, minValue, maxValue, h - 2 * padding) + padding;
    int slMinY = valueToY(m_slMin, minValue, maxValue, h - 2 * padding) + padding;
    int slMedianY = valueToY(m_slMedian, minValue, maxValue, h - 2 * padding) + padding;
    
    // Partie supérieure de la jauge (TP)
    if (m_tpAvg > 0) {
        // Partie moyenne (vert foncé)
        QRect tpAvgRect(gaugeX, tpAvgY, gaugeWidth, zeroY - tpAvgY);
        painter.setPen(m_tpAvgColor);
        painter.setBrush(m_tpAvgColor);
        painter.drawRect(tpAvgRect);
        
        // Partie maximum (vert clair)
        if (m_tpMax > m_tpAvg) {
            QRect tpMaxRect(gaugeX, tpMaxY, gaugeWidth, tpAvgY - tpMaxY);
            painter.setBrush(m_tpMaxColor);
            painter.setPen(m_tpAvgColor);
            painter.drawRect(tpMaxRect);
        }
    }
    
    // Partie inférieure de la jauge (SL)
    if (m_slAvg < 0) {
        // Partie moyenne (rouge foncé)
        QRect slAvgRect(gaugeX, zeroY, gaugeWidth, slAvgY - zeroY);
        painter.setBrush(m_slAvgColor);
        painter.setPen(m_slAvgColor);
        painter.drawRect(slAvgRect);
        
        // Partie minimum (rouge clair)
        if (m_slMin < m_slAvg) {
            QRect slMinRect(gaugeX, slAvgY, gaugeWidth, slMinY - slAvgY);
            painter.setBrush(m_slMinColor);
            painter.setPen(m_slAvgColor);
            painter.drawRect(slMinRect);
        }
    }
    
    // Ligne horizontale pour la médiane TP (pointillés verts)
    if (m_tpMedian > 0) {
        painter.setPen(QPen(m_medianTpColor, 1, Qt::DashLine));
        painter.drawLine(gaugeX, tpMedianY, gaugeX + gaugeWidth, tpMedianY);
    }
    
    // Ligne horizontale pour la médiane SL (pointillés rouges)
    if (m_slMedian < 0) {
        painter.setPen(QPen(m_medianSlColor, 1, Qt::DashLine));
        painter.drawLine(gaugeX, slMedianY, gaugeX + gaugeWidth, slMedianY);
    }
    
    // Configuration de la police pour les étiquettes
    painter.setPen(m_textColor);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont valueFont = painter.font();
    valueFont.setPointSize(13); // Réduire légèrement la taille de police pour les deux séries d'étiquettes
    painter.setFont(valueFont);
    
    // Définir les positions X pour les légendes
    int leftLegendX = 5;                         // Légendes de gauche (pourcentages)
    int rightLegendX = gaugeX + gaugeWidth + legendPadding; // Légendes de droite (valeurs absolues)
    int textWidth = 55;                          // Largeur des zones de texte
    
    // Hauteur standard d'une étiquette
    const int labelHeight = 20;
    
    // --- AJUSTEMENT DES POSITIONS POUR ÉVITER LES CHEVAUCHEMENTS ---
    
    // 1. Positions initiales des étiquettes
    int tpMaxLabelY = tpMaxY - labelHeight/2;
    int tpAvgLabelY = tpAvgY - labelHeight/2;
    int slAvgLabelY = slAvgY - labelHeight/2;
    int slMinLabelY = slMinY - labelHeight/2;
    
    // 2. Vérifier et corriger le chevauchement entre max TP et avg TP
    if (m_tpMax > 0 && m_tpAvg > 0 && std::abs(tpMaxLabelY - tpAvgLabelY) < labelHeight) {
        int overlap = labelHeight - std::abs(tpMaxLabelY - tpAvgLabelY);
        int adjustment = overlap / 2;
        
        tpMaxLabelY -= adjustment; // Déplacer le max vers le haut
        tpAvgLabelY += adjustment; // Déplacer la moyenne vers le bas
    }
    
    // 3. Vérifier et corriger le chevauchement entre avg SL et min SL
    if (m_slMin < 0 && m_slAvg < 0 && std::abs(slMinLabelY - slAvgLabelY) < labelHeight) {
        int overlap = labelHeight - std::abs(slMinLabelY - slAvgLabelY);
        int adjustment = overlap / 2;
        
        slMinLabelY += adjustment; // Déplacer le min vers le bas
        slAvgLabelY -= adjustment; // Déplacer la moyenne vers le haut
    }
    
    // --- DESSIN DES ÉTIQUETTES POURCENTAGE (GAUCHE) ---
    
    // Étiquettes pourcentage pour la partie TP (trades gagnants)
    if (m_tpMax > 0) {
        QRect maxPrcRect(leftLegendX, tpMaxLabelY, textWidth, labelHeight);
        painter.setPen(m_tpMaxColor.darker(150));
        painter.drawText(maxPrcRect, Qt::AlignRight | Qt::AlignVCenter, 
                      QString("%1%").arg(m_tpMaxPrc, 0, 'f', 2));
    }
    
    if (m_tpAvg > 0) {
        QRect avgPrcRect(leftLegendX, tpAvgLabelY, textWidth, labelHeight);
        painter.setPen(m_tpAvgColor.darker(120));
        painter.drawText(avgPrcRect, Qt::AlignRight | Qt::AlignVCenter, 
                      QString("%1%").arg(m_tpAvgPrc, 0, 'f', 2));
    }
    
    // Étiquettes pourcentage pour la partie SL (trades perdants)
    if (m_slAvg < 0) {
        QRect avgPrcRect(leftLegendX, slAvgLabelY, textWidth, labelHeight);
        painter.setPen(m_slAvgColor.darker(120));
        painter.drawText(avgPrcRect, Qt::AlignRight | Qt::AlignVCenter, 
                      QString("%1%").arg(m_slAvgPrc, 0, 'f', 2));
    }
    
    if (m_slMin < 0) {
        QRect minPrcRect(leftLegendX, slMinLabelY, textWidth, labelHeight);
        painter.setPen(m_slMinColor.darker(150));
        painter.drawText(minPrcRect, Qt::AlignRight | Qt::AlignVCenter, 
                      QString("%1%").arg(m_slMinPrc, 0, 'f', 2));
    }
    
    // --- DESSIN DES ÉTIQUETTES VALEURS ABSOLUES (DROITE) ---
    
    // Étiquettes pour la partie TP (trades gagnants)
    if (m_tpMax > 0) {
        QRect maxRect(rightLegendX, tpMaxLabelY, textWidth, labelHeight);
        painter.setPen(m_tpMaxColor.darker(150));
        painter.drawText(maxRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("%1").arg(m_tpMax, 0, 'f', 1));
    }
    
    if (m_tpAvg > 0) {
        QRect avgRect(rightLegendX, tpAvgLabelY, textWidth, labelHeight);
        painter.setPen(m_tpAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("%1").arg(m_tpAvg, 0, 'f', 1));
    }
    
    // Étiquettes pour la partie SL (trades perdants)
    if (m_slAvg < 0) {
        QRect avgRect(rightLegendX, slAvgLabelY, textWidth, labelHeight);
        painter.setPen(m_slAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("%1").arg(m_slAvg, 0, 'f', 1));
    }
    
    if (m_slMin < 0) {
        QRect minRect(rightLegendX, slMinLabelY, textWidth, labelHeight);
        painter.setPen(m_slMinColor.darker(150));
        painter.drawText(minRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("%1").arg(m_slMin, 0, 'f', 1));
    }
}