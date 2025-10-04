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

    // Ce checkbox peremt de switch entre affichage en valeur absolue et en pourcentage
    m_showPercentageCheckbox = new QCheckBox("Percentage", this);
    m_showPercentageCheckbox->setChecked(false); // Par défaut, afficher en valeur absolue

    // Connecter le signal toggled à une lambda qui appelle update()
    connect(m_showPercentageCheckbox, &QCheckBox::toggled, this, [this]() {
        this->update(); // Redessiner le widget lorsque l'état change
    });
    // Ajouter la checkbox au titre du widget

    setTitleCompanionWidget(m_showPercentageCheckbox);

    m_fontSize = 10;
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

    // Définir les dimensions et marges de base
    int gaugeWidth = 45;        // Largeur fixe pour la jauge
    int legendSpaceLeft = 70;   // Espace pour les légendes à gauche
    int legendSpaceRight = legendSpaceLeft;  // Espace pour les légendes à droite
    int textHeight = 20;        // Hauteur du texte max/min
    int padding = 10;           // Marge verticale réduite

    // Largeur totale nécessaire pour le widget
    int totalRequiredWidth = legendSpaceLeft + gaugeWidth + legendSpaceRight;
    
    // Calculer les offsets pour centrer le contenu horizontalement
    int offsetX = (contentRect.width() - totalRequiredWidth) / 2;
    
    // Hauteur effective pour la jauge (avec juste l'espace pour les textes, sans marge supplémentaire)
    int effectiveHeight = contentRect.height() - 2 * padding - 2 * textHeight;
    
    // Position de la jauge, centrée horizontalement
    int gaugeX = contentRect.left() + offsetX + legendSpaceLeft;
    int gaugeTop = contentRect.top() + padding + textHeight; // Début de la jauge après le texte max


    double displayTpAvg = m_tpAvg;
    double displayTpMax = m_tpMax;
    double displayTpMedian = m_tpMedian;

    double displaySlAvg = m_slAvg;
    double displaySlMin = m_slMin;
    double displaySlMedian = m_slMedian;

    QString suffix = "€";
    int precision = 1;

    if (m_showPercentageCheckbox->isChecked()) {
        displayTpAvg = m_tpAvgPrc;
        displayTpMax = m_tpMaxPrc;
        displayTpMedian = m_tpMedianPrc;

        displaySlAvg = m_slAvgPrc;
        displaySlMin = m_slMinPrc;
        displaySlMedian = m_slMedianPrc;

        suffix = "%";
        precision = 2;
    }


    
    // Récupérer les valeurs max et min pour l'échelle
    double posMax = displayTpMax;
    double negMin = displaySlMin;
    
    // Éviter les divisions par zéro
    if (posMax == 0) posMax = 0.1;
    if (negMin == 0) negMin = -0.1;
    
    // Calculer la plage totale
    double totalRange = posMax - negMin;
    
    // Calculer la position du zéro en proportion de la plage totale
    // Le zéro se situe à |negMin| / (|negMin| + posMax) depuis le haut
    double zeroRatio = std::abs(posMax) / totalRange;
    int zeroY = gaugeTop + (int)(effectiveHeight * zeroRatio);
    
    // Fonction modifiée pour convertir les valeurs en positions Y
    auto valueToYScaled = [gaugeTop, zeroY, effectiveHeight, posMax, negMin](double value) -> int {
        if (value >= 0) {
            // Au-dessus du zéro
            return zeroY - (int)(value / posMax * (zeroY - gaugeTop));
        } else {
            // En dessous du zéro
            return zeroY + (int)(value / negMin * (gaugeTop + effectiveHeight - zeroY));
        }
    };
    
    // Calculer les positions Y des valeurs
    int tpMaxY = valueToYScaled(displayTpMax);
    int tpAvgY = valueToYScaled(displayTpAvg);
    int tpMedianY = valueToYScaled(displayTpMedian);
    int slAvgY = valueToYScaled(displaySlAvg);
    int slMinY = valueToYScaled(displaySlMin);
    int slMedianY = valueToYScaled(displaySlMedian);
    
    // Partie supérieure de la jauge (TP)
    if (displayTpAvg > 0) {
        // Partie moyenne (vert foncé)
        QRect tpAvgRect(gaugeX, tpAvgY, gaugeWidth, zeroY - tpAvgY);
        painter.setPen(m_tpAvgColor);
        painter.setBrush(m_tpAvgColor);
        painter.drawRect(tpAvgRect);
        
        // Partie maximum (vert clair)
        if (displayTpMax > displayTpAvg) {
            QRect tpMaxRect(gaugeX, tpMaxY, gaugeWidth, tpAvgY - tpMaxY);
            painter.setBrush(m_tpMaxColor);
            painter.setPen(m_tpAvgColor);
            painter.drawRect(tpMaxRect);
        }
    }
    
    // Partie inférieure de la jauge (SL)
    if (displaySlAvg < 0) {
        // Partie moyenne (rouge foncé)
        QRect slAvgRect(gaugeX, zeroY, gaugeWidth, slAvgY - zeroY);
        painter.setBrush(m_slAvgColor);
        painter.setPen(m_slAvgColor);
        painter.drawRect(slAvgRect);
        
        // Partie minimum (rouge clair)
        if (displaySlMin < displaySlAvg) {
            QRect slMinRect(gaugeX, slAvgY, gaugeWidth, slMinY - slAvgY);
            painter.setBrush(m_slMinColor);
            painter.setPen(m_slAvgColor);
            painter.drawRect(slMinRect);
        }
    }
    
    // Ligne horizontale pour le zéro (optionnelle)
    // painter.setPen(Qt::gray);
    // painter.drawLine(gaugeX, zeroY, gaugeX + gaugeWidth, zeroY);
    
    // Ligne horizontale pour la médiane TP (pointillés verts)
    // if (displayTpMedian > 0) {
    //     painter.setPen(QPen(m_medianTpColor, 1, Qt::DashLine));
    //     painter.drawLine(gaugeX, tpMedianY, gaugeX + gaugeWidth, tpMedianY);
    // }
    
    // // Ligne horizontale pour la médiane SL (pointillés rouges)
    // if (displaySlMedian < 0) {
    //     painter.setPen(QPen(m_medianSlColor, 1, Qt::DashLine));
    //     painter.drawLine(gaugeX, slMedianY, gaugeX + gaugeWidth, slMedianY);
    // }
    
    // Configuration de la police pour les étiquettes
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont valueFont = painter.font();
    valueFont.setPointSize(m_fontSize);
    valueFont.setWeight(m_fontWeight);
    painter.setFont(valueFont);
    
    // --- DESSIN DES ÉTIQUETTES MAX ET MIN (AU-DESSUS ET EN-DESSOUS) ---
    
    // Max Profit au-dessus de la jauge - directement au-dessus
    if (displayTpMax > 0) {
        QString maxProfitText = QString("Max Profit: %1 %2").arg(displayTpMax, 0, 'f', precision).arg(suffix);
        QRect maxProfitRect(gaugeX - gaugeWidth, gaugeTop - textHeight, 
                          gaugeWidth * 3, textHeight);
        painter.setPen(m_tpMaxColor.darker(150));
        painter.drawText(maxProfitRect, Qt::AlignCenter, maxProfitText);
    }
    
    // Max Loss en-dessous de la jauge - directement en-dessous
    if (displaySlMin < 0) {
        QString maxLossText = QString("Max Loss: %1 %2").arg(displaySlMin, 0, 'f', precision).arg(suffix);
        QRect maxLossRect(gaugeX - gaugeWidth, 
                        gaugeTop + effectiveHeight, 
                        gaugeWidth * 3, textHeight);
        painter.setPen(m_slMinColor.darker(150));
        painter.drawText(maxLossRect, Qt::AlignCenter, maxLossText);
    }
    
    // --- DESSIN DES ÉTIQUETTES MOYENNE (SUR LE CÔTÉ) ---
    
    // Définir les positions X pour les légendes
    int leftLegendX = gaugeX - 10;
    int rightLegendX = gaugeX + gaugeWidth + 10;
    
    // Hauteur de l'étiquette de moyenne
    const int avgLabelHeight = 40;
    
    // Moyenne TP (côté gauche)
    if (displayTpAvg > 0) {
        // S'assurer que le texte ne dépasse pas la zone de la jauge
        int avgY = qMax(
            gaugeTop,        // Minimum Y
            tpAvgY                             // Position idéale
        );
        
        QRect avgRect(leftLegendX - 60, avgY - avgLabelHeight/2, 60, avgLabelHeight);
        painter.setPen(m_tpAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignRight | Qt::AlignVCenter, 
                      QString("Moyenne\n%1 %2").arg(displayTpAvg, 0, 'f', precision).arg(suffix));
    }
    
    // Moyenne SL (côté droit)
    if (displaySlAvg < 0) {
        // S'assurer que le texte ne dépasse pas la zone de la jauge
        int avgY = qMin(
            slAvgY,                                   // Position idéale
            gaugeTop + effectiveHeight - avgLabelHeight/2  // Maximum Y
        );
        
        QRect avgRect(rightLegendX, slAvgY - avgLabelHeight/2, 60, avgLabelHeight);
        painter.setPen(m_slAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignLeft | Qt::AlignVCenter, 
                      QString("Moyenne\n%1 %2").arg(displaySlAvg, 0, 'f', precision).arg(suffix));
    }
}