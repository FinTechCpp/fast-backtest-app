#include "ui/views/Stats/PnLGaugeWidget.h"
#include <algorithm>
#include <numeric>
#include <QDebug>
#include <QGroupBox>
#include <QFontMetrics>

// VerticalGaugeWidget Implementation
// ===============================

PnLGaugeWidget::PnLGaugeWidget(QWidget* parent)
    : StatsBaseWidget(parent),
      m_tpAvg(0.0),
      m_tpMax(0.0),
      m_tpMedian(0.0),
      m_slAvg(0.0),
      m_slMin(0.0),
      m_slMedian(0.0),
      m_tpCount(0),
      m_slCount(0)
{
    setupUI();
}

PnLGaugeWidget::~PnLGaugeWidget()
{
    // Qt gère automatiquement la libération de mémoire des widgets enfants
}

void PnLGaugeWidget::setupUI()
{
    // Layout principal
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Titre du widget
    m_titleLabel = new QLabel("Distribution Relative des P&L");
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel);
    
    // Layout pour la jauge et les statistiques complémentaires
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setSpacing(10);
    
    // Widget de la jauge - maintenant avec sa propre légende
    m_gaugeWidget = new VerticalGaugeRenderWidget();
    m_contentLayout->addWidget(m_gaugeWidget);
    
    // Widget pour les statistiques complémentaires
    m_legendWidget = new QWidget();
    m_legendLayout = new QVBoxLayout(m_legendWidget);
    m_legendLayout->setSpacing(5);
    m_legendLayout->setContentsMargins(0, 10, 0, 10);
    
    // Statistiques combinées
    QGroupBox* statsGroupBox = new QGroupBox("Informations complémentaires");
    QGridLayout* statsLayout = new QGridLayout(statsGroupBox);
    
    // Ratio et edge
    m_ratioLabel = new QLabel("Ratio Gain/Perte: N/A");
    statsLayout->addWidget(m_ratioLabel, 4, 0, 1, 2);
    
    m_edgeLabel = new QLabel("Edge: N/A");
    statsLayout->addWidget(m_edgeLabel, 5, 0, 1, 2);
    
    m_legendLayout->addWidget(statsGroupBox);
    m_legendLayout->addStretch();
    
    m_contentLayout->addWidget(m_legendWidget);
    
    // Ajouter le layout de contenu au layout principal
    m_mainLayout->addLayout(m_contentLayout);
}

void PnLGaugeWidget::updateContent(const be::Stats& stats)
{
    if (stats.trades.empty()) {
        clear();
        return;
    }
    
    // Collecter les PL des trades en fonction de leur raison de fermeture
    std::vector<double> tpPLs;
    std::vector<double> slPLs;
    
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
        } else if (trade.closeReason == be::CloseReason::StopLoss) {
            slPLs.push_back(trade.pl);
        }
    }
    
    // Stocker le nombre de trades
    m_tpCount = tpPLs.size();
    m_slCount = slPLs.size();
    
    // Calcul des statistiques pour les trades gagnants (TP)
    if (!tpPLs.empty()) {
        m_tpAvg = std::accumulate(tpPLs.begin(), tpPLs.end(), 0.0) / tpPLs.size();
        m_tpMax = *std::max_element(tpPLs.begin(), tpPLs.end());
        
        // Calculer la médiane des trades TP
        std::vector<double> sortedTpPLs = tpPLs;
        std::sort(sortedTpPLs.begin(), sortedTpPLs.end());
        
        if (sortedTpPLs.size() % 2 == 0) {
            m_tpMedian = (sortedTpPLs[sortedTpPLs.size() / 2 - 1] + 
                         sortedTpPLs[sortedTpPLs.size() / 2]) / 2.0;
        } else {
            m_tpMedian = sortedTpPLs[sortedTpPLs.size() / 2];
        }
    } else {
        m_tpAvg = 0.0;
        m_tpMax = 0.0;
        m_tpMedian = 0.0;
    }
    
    // Calcul des statistiques pour les trades perdants (SL)
    if (!slPLs.empty()) {
        m_slAvg = std::accumulate(slPLs.begin(), slPLs.end(), 0.0) / slPLs.size();
        m_slMin = *std::min_element(slPLs.begin(), slPLs.end());
        
        // Calculer la médiane des trades SL
        std::vector<double> sortedSlPLs = slPLs;
        std::sort(sortedSlPLs.begin(), sortedSlPLs.end());
        
        if (sortedSlPLs.size() % 2 == 0) {
            m_slMedian = (sortedSlPLs[sortedSlPLs.size() / 2 - 1] + 
                         sortedSlPLs[sortedSlPLs.size() / 2]) / 2.0;
        } else {
            m_slMedian = sortedSlPLs[sortedSlPLs.size() / 2];
        }
    } else {
        m_slAvg = 0.0;
        m_slMin = 0.0;
        m_slMedian = 0.0;
    }
    
    // Informations détaillées sur les meilleurs/pires trades
    if (bestTrade) {
        m_bestTradeInfo = QString("%1 (%2)")
            .arg(QString::number(bestTrade->pl, 'f', 2))
            .arg(QString::fromStdString(bestTrade->tag));
    } else {
        m_bestTradeInfo = "N/A";
    }
    
    if (worstTrade) {
        m_worstTradeInfo = QString("%1 (%2)")
            .arg(QString::number(worstTrade->pl, 'f', 2))
            .arg(QString::fromStdString(worstTrade->tag));
    } else {
        m_worstTradeInfo = "N/A";
    }
    
    // Mettre à jour la jauge
    m_gaugeWidget->setValues(m_tpAvg, m_tpMax, m_tpMedian, m_slAvg, m_slMin, m_slMedian);
    
    // Mettre à jour les labels
    updateLabels();
}

void PnLGaugeWidget::updateLabels()
{    
    // Calculer et afficher le ratio gain/perte
    double absRatio = 0.0;
    if (m_slAvg < 0 && m_tpAvg > 0) {
        absRatio = m_tpAvg / std::abs(m_slAvg);
        m_ratioLabel->setText(QString("Ratio Gain/Perte: %1").arg(absRatio, 0, 'f', 2));
        
        // Définir la couleur en fonction du ratio
        if (absRatio >= 2.0) {
            m_ratioLabel->setStyleSheet("color: green; font-weight: bold;");
        } else if (absRatio >= 1.0) {
            m_ratioLabel->setStyleSheet("color: darkgreen;");
        } else {
            m_ratioLabel->setStyleSheet("color: red;");
        }
    } else {
        m_ratioLabel->setText("Ratio Gain/Perte: N/A");
        m_ratioLabel->setStyleSheet("");
    }
    
    // Calculer et afficher l'edge
    if (m_tpCount > 0 && m_slCount > 0) {
        double winRate = static_cast<double>(m_tpCount) / (m_tpCount + m_slCount);
        double edge = (winRate * m_tpAvg) - ((1 - winRate) * std::abs(m_slAvg));
        
        m_edgeLabel->setText(QString("Edge: %1").arg(edge, 0, 'f', 2));
        
        if (edge > 0) {
            m_edgeLabel->setStyleSheet("color: green;");
        } else {
            m_edgeLabel->setStyleSheet("color: red;");
        }
    } else {
        m_edgeLabel->setText("Edge: N/A");
        m_edgeLabel->setStyleSheet("");
    }
}

void PnLGaugeWidget::clear()
{
    // Réinitialiser toutes les valeurs
    m_tpAvg = 0.0;
    m_tpMax = 0.0;
    m_tpMedian = 0.0;
    m_slAvg = 0.0;
    m_slMin = 0.0;
    m_slMedian = 0.0;
    m_tpCount = 0;
    m_slCount = 0;
    m_bestTradeInfo = "N/A";
    m_worstTradeInfo = "N/A";
    
    // Réinitialiser le widget de la jauge
    m_gaugeWidget->clear();
    
    // Réinitialiser les labels
    m_ratioLabel->setText("Ratio Gain/Perte: N/A");
    m_ratioLabel->setStyleSheet("");
    m_edgeLabel->setText("Edge: N/A");
    m_edgeLabel->setStyleSheet("");
}





// ===========================================================
// VerticalGaugeRenderWidget Implementation
// ===========================================================

VerticalGaugeRenderWidget::VerticalGaugeRenderWidget(QWidget* parent)
    : QWidget(parent),
      m_tpAvg(0.0),
      m_tpMax(0.0),
      m_tpMedian(0.0),
      m_slAvg(0.0),
      m_slMin(0.0),
      m_slMedian(0.0)
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
    setMinimumSize(150, 300);  // Augmenté de 40 à 150 pour avoir de l'espace pour la légende
    setMaximumWidth(180);      // Augmenté de 60 à 180
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void VerticalGaugeRenderWidget::setValues(double tpAvg, double tpMax, double tpMedian,
                                        double slAvg, double slMin, double slMedian)
{
    m_tpAvg = tpAvg;
    m_tpMax = tpMax;
    m_tpMedian = tpMedian;
    m_slAvg = slAvg;
    m_slMin = slMin;
    m_slMedian = slMedian;
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

void VerticalGaugeRenderWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    int gaugeWidth = 45;        // Largeur fixe pour la jauge
    int gaugeX = 15;            // Position X fixe, décalée du bord gauche
    int padding = 15;           // Marge en haut et en bas
    int legendPadding = 10;     // Espace entre la jauge et le début de la légende
    
    // Calculer la plage symétrique autour de zéro
    double maxAbsValue = std::max(std::abs(m_tpMax), std::abs(m_slMin));
    double minValue = -maxAbsValue;
    double maxValue = maxAbsValue;
    
    // Tracer le fond de la jauge
    painter.setPen(QPen(m_lineColor, 1));
    painter.setBrush(Qt::white);
    painter.drawRect(gaugeX, padding, gaugeWidth, h - 2 * padding);
    
    // Position Y du zéro (milieu de la jauge)
    int zeroY = padding + (h - 2 * padding) / 2;
    
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
        painter.setBrush(m_tpAvgColor);
        painter.drawRect(tpAvgRect);
        
        // Partie maximum (vert clair)
        if (m_tpMax > m_tpAvg) {
            QRect tpMaxRect(gaugeX, tpMaxY, gaugeWidth, tpAvgY - tpMaxY);
            painter.setBrush(m_tpMaxColor);
            painter.drawRect(tpMaxRect);
        }
    }
    
    // Partie inférieure de la jauge (SL)
    if (m_slAvg < 0) {
        // Partie moyenne (rouge foncé)
        QRect slAvgRect(gaugeX, zeroY, gaugeWidth, slAvgY - zeroY);
        painter.setBrush(m_slAvgColor);
        painter.drawRect(slAvgRect);
        
        // Partie minimum (rouge clair)
        if (m_slMin < m_slAvg) {
            QRect slMinRect(gaugeX, slAvgY, gaugeWidth, slMinY - slAvgY);
            painter.setBrush(m_slMinColor);
            painter.drawRect(slMinRect);
        }
    }
    
    // Ligne horizontale au niveau zéro
    painter.setPen(QPen(m_lineColor, 2));
    painter.drawLine(gaugeX - 5, zeroY, gaugeX + gaugeWidth + 5, zeroY);
    
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
    QFont valueFont = painter.font();
    valueFont.setPointSize(11);
    painter.setFont(valueFont);
    
    int legendX = gaugeX + gaugeWidth + legendPadding;
    int textWidth = w - legendX - 5; // Largeur disponible pour le texte
    
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
    
    // --- DESSIN DES ÉTIQUETTES AVEC LES POSITIONS AJUSTÉES ---

    // Étiquettes pour la partie TP (trades gagnants)
    if (m_tpMax > 0) {
        QRect maxRect(legendX, tpMaxLabelY, textWidth, labelHeight);
        painter.setPen(m_tpMaxColor.darker(150));
        painter.drawText(maxRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("Max: %1").arg(m_tpMax, 0, 'f', 1));
    }
    
    if (m_tpAvg > 0) {
        QRect avgRect(legendX, tpAvgLabelY, textWidth, labelHeight);
        painter.setPen(m_tpAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("Moy: %1").arg(m_tpAvg, 0, 'f', 1));
    }
    
    // Étiquette du zéro
    QRect zeroRect(legendX, zeroY - labelHeight/2, textWidth, labelHeight);
    painter.setPen(m_textColor);
    painter.drawText(zeroRect, Qt::AlignLeft | Qt::AlignVCenter, "0");
    
    // Étiquettes pour la partie SL (trades perdants)
    if (m_slAvg < 0) {
        QRect avgRect(legendX, slAvgLabelY, textWidth, labelHeight);
        painter.setPen(m_slAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("Moy: %1").arg(m_slAvg, 0, 'f', 1));
    }
    
    if (m_slMin < 0) {
        QRect minRect(legendX, slMinLabelY, textWidth, labelHeight);
        painter.setPen(m_slMinColor.darker(150));
        painter.drawText(minRect, Qt::AlignLeft | Qt::AlignVCenter, 
                       QString("Min: %1").arg(m_slMin, 0, 'f', 1));
    }
}