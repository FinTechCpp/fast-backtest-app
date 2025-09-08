#include "ui/views/Stats/TradingHeatmapWidget.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <algorithm>
#include <cmath> // Pour std::fabs

TradingHeatmapWidget::TradingHeatmapWidget(QWidget* parent)
    : StatsBaseWidget(parent),
      m_minValue(0.0),
      m_maxValue(0.0),
      m_minHour(24),
      m_maxHour(0)
{
    // Initialiser les données
    m_performanceData.resize(HOURS_IN_DAY);
    m_tradeCountData.resize(HOURS_IN_DAY);
    m_squaredSumData.resize(HOURS_IN_DAY);
    
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        m_performanceData[h].resize(DAYS_IN_WEEK);
        m_tradeCountData[h].resize(DAYS_IN_WEEK);
        m_squaredSumData[h].resize(DAYS_IN_WEEK);
        
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
            m_squaredSumData[h][d] = 0.0;
        }
    }
    
    // Créer le groupe box principal
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel("Analyse des Performances par Heure et Jour");
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel);

    
    // Créer la scène et la vue pour la heatmap (une seule scène qui contiendra tout)
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(400);
    m_view->setMinimumWidth(600); // Plus large pour accommoder la légende
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setAlignment(Qt::AlignCenter);
    
    // Ajouter la vue au layout
    m_mainLayout->addWidget(m_view);
    
    // Installer un filtre d'événements pour gérer le redimensionnement
    m_view->viewport()->installEventFilter(this);
}

int TradingHeatmapWidget::getDayOfWeek(const be::Date& date) {
    // Algorithme de Zeller pour déterminer le jour de la semaine
    int q = date.day;   // jour du mois
    int m = date.month; // mois
    int y = date.year;  // année
    
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    
    int h = (q + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    
    // Convertir de l'algorithme de Zeller (0=samedi) à notre format (0=lundi)
    return (h + 5) % 7;
}

void TradingHeatmapWidget::analyzeTradesByTimeAndDay(const std::vector<be::TradeData>& trades) {
    // Réinitialiser les données
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
            m_squaredSumData[h][d] = 0.0;
        }
    }
    
    // Réinitialiser les min/max
    m_minValue = 0.0;
    m_maxValue = 0.0;
    m_minHour = 24;
    m_maxHour = 0;
    bool firstValue = true;
    
    // Analyser chaque trade
    for (const auto& trade : trades) {
        // Utiliser la date d'entrée pour déterminer l'heure et le jour
        int hour = trade.entryDate.hour;
        int day = getDayOfWeek(trade.entryDate);
        
        // Vérifier que les indices sont dans la plage
        if (hour < 0 || hour >= HOURS_IN_DAY || day < 0 || day >= DAYS_IN_WEEK)
            continue;

        // Ajouter la performance du trade
        m_performanceData[hour][day] += trade.pl;
        m_tradeCountData[hour][day]++;
        m_squaredSumData[hour][day] += trade.pl * trade.pl; // Pour l'écart-type
        
        // Mettre à jour la plage horaire
        m_minHour = std::min(m_minHour, hour);
        m_maxHour = std::max(m_maxHour, hour);
    }
    
    // Assurer un minimum d'espace pour l'affichage (au moins 3h d'amplitude)
    if (m_maxHour - m_minHour < 3) {
        int extension = (3 - (m_maxHour - m_minHour)) / 2;
        m_minHour = std::max(0, m_minHour - extension);
        m_maxHour = std::min(23, m_maxHour + extension);
    }
    
    // Trouver les valeurs min/max pour l'échelle de couleur
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            if (m_tradeCountData[h][d] <= 0)
                continue; // Pas de trades pour cette heure/jour

            // Calculer la performance moyenne
            double expectation = m_performanceData[h][d] / m_tradeCountData[h][d];

            // Mettre à jour min/max
            if (firstValue || expectation < m_minValue) {
                m_minValue = expectation;
            }

            if (firstValue || expectation > m_maxValue) {
                m_maxValue = expectation;
            }
            
            firstValue = false;
        }
    }
    
    // CORRECTION: Équilibrer les bornes pour que les valeurs absolues soient égales
    // Cela garantit que l'intensité de couleur est symétrique
    double absMax = std::max(std::fabs(m_minValue), std::fabs(m_maxValue));
    m_minValue = -absMax;  // Forcer le minimum à être l'opposé du maximum en valeur absolue
    m_maxValue = absMax;   // Maximum reste inchangé en valeur absolue
    
    // S'assurer que min et max ne sont pas identiques pour éviter une division par zéro
    if (qFuzzyCompare(m_minValue, m_maxValue)) {
        m_minValue = -1.0;
        m_maxValue = 1.0;
    }
}

QColor TradingHeatmapWidget::getColorForValue(double value) {
    // Assurer que la valeur est dans la plage [min, max]
    value = qBound(m_minValue, value, m_maxValue);
    
    // Normaliser la valeur entre -1 et 1
    double range = m_maxValue - m_minValue;
    double normalizedValue = range != 0 ? 2.0 * (value - m_minValue) / range - 1.0 : 0.0;
    
    // Gradient rouge-orange-jaune-vert
    if (normalizedValue < 0) {
        // De rouge à orange/jaune
        double ratio = 1.0 + normalizedValue; // 0 à 1 (de min à 0)
        
        // Rouge toujours à max pour les valeurs négatives
        int red = 255;
        
        // Vert varie de 0 à 165 (orange)
        int green = static_cast<int>(165 * ratio);
        
        // Bleu toujours à 0 pour les valeurs négatives/neutres
        int blue = 0;
        
        return QColor(red, green, blue);
    } 
    else {
        // De jaune à vert
        double ratio = normalizedValue; // 0 à 1 (de 0 à max)
        
        // Rouge varie de 255 à 0
        int red = static_cast<int>(255 * (1.0 - ratio));
        
        // Vert toujours à max pour les valeurs positives/neutres
        int green = 255;
        
        // Bleu toujours à 0
        int blue = 0;
        
        return QColor(red, green, blue);
    }
}

void TradingHeatmapWidget::buildHeatmap() {
    // Effacer la scène
    m_scene->clear();
    
    // Calculer le nombre d'heures à afficher
    int numHoursToShow = m_maxHour - m_minHour + 1;
    
    // Ajuster la taille des cellules en fonction du nombre d'heures à afficher
    int adjustedCellSize = std::min(CELL_SIZE, 400 / numHoursToShow);
    
    // Marge pour les labels
    int leftMargin = 50;  // Augmenté pour accommoder les labels d'heures
    int topMargin = 70;
    
    // Ajouter un titre global centré au-dessus de la heatmap
    int totalTrades = 0;
    for (int h = m_minHour; h <= m_maxHour; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            totalTrades += m_tradeCountData[h][d];
        }
    }
    
    // Dessiner les labels des jours (en haut)
    for (int d = 0; d < DAYS_IN_WEEK; d++) {
        QGraphicsTextItem* dayLabel = m_scene->addText(m_dayNames[d]);
        QFont dayFont = dayLabel->font();
        dayLabel->setFont(dayFont);
        
        // Centrer le texte sur la colonne
        QRectF textRect = dayLabel->boundingRect();
        dayLabel->setPos(leftMargin + d * (adjustedCellSize + CELL_SPACING) + 
                        (adjustedCellSize - textRect.width())/2, topMargin - 25);
    }
    
    // Dessiner les bordures d'heures aux limites des cellules
    for (int h = m_minHour; h <= m_maxHour + 1; h++) {  // +1 pour ajouter la dernière limite
        int rowIndex = h - m_minHour;
        
        // Position Y de la limite
        int y = topMargin + rowIndex * (adjustedCellSize + CELL_SPACING);
        
        // Ajouter une ligne horizontale fine (optionnel, pour visualiser la limite)
        if (h <= m_maxHour) {
            QGraphicsLineItem* line = m_scene->addLine(
                leftMargin - 5, y, 
                leftMargin + DAYS_IN_WEEK * (adjustedCellSize + CELL_SPACING), y,
                QPen(Qt::gray, 0.5, Qt::DotLine)
            );
        }
        
        // Ajouter le label d'heure à gauche
        QGraphicsTextItem* hourLabel = m_scene->addText(QString::number(h) + "h");
        QFont hourFont = hourLabel->font();
        hourLabel->setFont(hourFont);
        
        // Aligner à droite de la ligne
        hourLabel->setPos(leftMargin - hourLabel->boundingRect().width() - 5, y - hourLabel->boundingRect().height() / 2);
    }
    
    // Dessiner la heatmap - seulement pour la plage pertinente
    for (int h = m_minHour; h <= m_maxHour; h++) {
        int rowIndex = h - m_minHour;
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            // Position de la cellule
            int x = leftMargin + d * (adjustedCellSize + CELL_SPACING);
            int y = topMargin + rowIndex * (adjustedCellSize + CELL_SPACING);
            
            // Vérifier s'il y a des trades pour cette cellule
            if (m_tradeCountData[h][d] <= 0) {
                // Cellule grise pour les périodes sans trades
                m_scene->addRect(
                    x, y, adjustedCellSize, adjustedCellSize,
                    QPen(Qt::NoPen), QBrush(QColor(240, 240, 240))
                );
                continue;
            }

            // Calculer l'espérance et la variance
            int count = m_tradeCountData[h][d];
            double totalPnL = m_performanceData[h][d];
            double expectation = totalPnL / count; // Espérance
            
            // Variance = E[X²] - (E[X])²
            double meanOfSquares = m_squaredSumData[h][d] / count;
            double variance = meanOfSquares - (expectation * expectation);
            double stddev = variance > 0 ? std::sqrt(variance) : 0.0; // Écart-type
            
            QColor cellColor = getColorForValue(expectation);
            
            // Ajouter un rectangle avec une bordure fine
            QGraphicsRectItem* cell = m_scene->addRect(
                x, y, adjustedCellSize, adjustedCellSize,
                QPen(Qt::NoPen), QBrush(cellColor)
            );

            // Modifier le tooltip pour inclure espérance et écart-type
            QString tooltipText = QString("Jour: %1\nHeure: %2h - %3h\nEspérance: %4 €\nÉcart-type: %5 €\nTrades: %6")
                                    .arg(m_dayNames[d])
                                    .arg(h)
                                    .arg(h+1)
                                    .arg(expectation, 0, 'f', 2)
                                    .arg(stddev, 0, 'f', 2)
                                    .arg(count);
            cell->setToolTip(tooltipText);

            // Afficher l'espérance et l'écart-type dans la cellule
            // Format: E=XX.XX
            //         V=XX.XX
            QString expText = QString("%1").arg(expectation, 0, 'f', 1);
            QGraphicsTextItem* expTextItem = m_scene->addText(expText);
            QFont expFont = expTextItem->font();
            expFont.setPointSize(10); // Taille de police plus grande pour l'espérance
            expFont.setBold(true);
            expTextItem->setFont(expFont);

            QString stddevText = QString("%1").arg(stddev, 0, 'f', 0);
            QGraphicsTextItem* stddevTextItem = m_scene->addText(stddevText);
            QFont stddevFont = stddevTextItem->font();
            stddevFont.setPointSize(8); // Taille de police plus petite pour l'écart-type
            stddevTextItem->setFont(stddevFont);

            // Centrer les textes dans la cellule
            QRectF expRect = expTextItem->boundingRect();
            QRectF stddevRect = stddevTextItem->boundingRect();

            // Positionner l'espérance au milieu-haut
            expTextItem->setPos(
                x + (adjustedCellSize - expRect.width())/2,
                y + adjustedCellSize * 0.25 - expRect.height()/2
            );

            // Positionner l'écart-type au milieu-bas
            stddevTextItem->setPos(
                x + (adjustedCellSize - stddevRect.width())/2,
                y + adjustedCellSize * 0.75 - stddevRect.height()/2
            );
            
            // Ajuster la couleur du texte pour la lisibilité
            QColor textColor = QColor::fromHsv(cellColor.hue(), 
                                              cellColor.saturation(),
                                              cellColor.value() < 128 ? 240 : 30);
            expTextItem->setDefaultTextColor(textColor);
            stddevTextItem->setDefaultTextColor(textColor);
        }
    }
    
    // *** LÉGENDE VERTICALE DANS LE MÊME CANVAS ***
    
    // Position de départ de la légende (à droite de la heatmap avec une marge fixe)
    int heatmapRightX = leftMargin + DAYS_IN_WEEK * (adjustedCellSize + CELL_SPACING);
    int fixedMargin = 40; // Marge fixe entre la heatmap et la légende
    
    int legendX = heatmapRightX + fixedMargin;
    int legendY = topMargin;
    int legendWidth = 30;
    int legendHeight = numHoursToShow * (adjustedCellSize + CELL_SPACING) - CELL_SPACING;
    
    // Titre de la légende
    QGraphicsTextItem* legendTitle = m_scene->addText("E/σ (€)");
    QFont legendTitleFont = legendTitle->font();
    legendTitle->setFont(legendTitleFont);
    legendTitle->setPos(legendX, legendY - 35);
    
    // Gradient vertical (de bas en haut)
    QLinearGradient gradient(0, legendY + legendHeight, 0, legendY);
    
    // Points d'arrêt pour le gradient (rouge-orange-jaune-vert)
    gradient.setColorAt(0.0, getColorForValue(m_minValue));        // Rouge pour min
    gradient.setColorAt(0.25, getColorForValue(m_minValue/2));     // Orange
    gradient.setColorAt(0.5, getColorForValue(0.0));               // Jaune pour zéro
    gradient.setColorAt(0.75, getColorForValue(m_maxValue/2));     // Jaune-vert
    gradient.setColorAt(1.0, getColorForValue(m_maxValue));        // Vert pour max
    
    // Rectangle du gradient avec bordure
    m_scene->addRect(legendX, legendY, legendWidth, legendHeight, 
                    QPen(Qt::black, 1), QBrush(gradient));
    
    // Labels des valeurs à droite du rectangle
    
    // Maximum (en haut)
    QGraphicsTextItem* maxText = m_scene->addText(QString("%1 €").arg(m_maxValue, 0, 'f', 2));
    QFont valueFont = maxText->font();
    maxText->setFont(valueFont);
    maxText->setPos(legendX + legendWidth + 5, legendY - maxText->boundingRect().height()/2);
    
    // Quart positif
    QGraphicsTextItem* quarterPosText = m_scene->addText(QString("%1 €").arg(m_maxValue/2, 0, 'f', 2));
    quarterPosText->setFont(valueFont);
    quarterPosText->setPos(legendX + legendWidth + 5, 
                          legendY + legendHeight/4 - quarterPosText->boundingRect().height()/2);
    
    // Zéro (milieu)
    QGraphicsTextItem* zeroText = m_scene->addText("0 €");
    zeroText->setFont(valueFont);
    zeroText->setPos(legendX + legendWidth + 5, 
                    legendY + legendHeight/2 - zeroText->boundingRect().height()/2);
    
    // Quart négatif
    QGraphicsTextItem* quarterNegText = m_scene->addText(QString("%1 €").arg(m_minValue/2, 0, 'f', 2));
    quarterNegText->setFont(valueFont);
    quarterNegText->setPos(legendX + legendWidth + 5, 
                          legendY + 3*legendHeight/4 - quarterNegText->boundingRect().height()/2);
    
    // Minimum (en bas)
    QGraphicsTextItem* minText = m_scene->addText(QString("%1 €").arg(m_minValue, 0, 'f', 2));
    minText->setFont(valueFont);
    minText->setPos(legendX + legendWidth + 5, 
                   legendY + legendHeight - minText->boundingRect().height()/2);
    
    // Ajuster la vue pour afficher toute la scène
    QRectF boundingRect = m_scene->itemsBoundingRect();
    m_scene->setSceneRect(boundingRect);
    m_view->fitInView(boundingRect, Qt::KeepAspectRatio);
    m_view->centerOn(boundingRect.center());
}

bool TradingHeatmapWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_view->viewport() && event->type() == QEvent::Resize) {
        if (!m_scene->items().isEmpty()) {
            m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TradingHeatmapWidget::updateContent(const be::Stats& stats) {
    if (stats.trades.empty()) {
        return;
    }
    
    // Analyser les trades par heure et jour
    analyzeTradesByTimeAndDay(stats.trades);
    
    // Construire la heatmap
    buildHeatmap();
    
    qDebug() << "Heatmap mise à jour avec" << stats.trades.size() << "trades"
             << "couvrant les heures" << m_minHour << "à" << m_maxHour
             << "Échelle de PnL: [" << m_minValue << "," << m_maxValue << "]";
}

void TradingHeatmapWidget::clear() {
    // Réinitialiser les données
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
            m_squaredSumData[h][d] = 0.0;
        }
    }
    
    // Effacer la scène
    m_scene->clear();
}

void TradingHeatmapWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    // Réajuster la vue au contenu après redimensionnement si elle contient des éléments
    if (m_scene && !m_scene->items().isEmpty()) {
        QRectF bounds = m_scene->itemsBoundingRect();
        if (!bounds.isEmpty()) {
            m_view->fitInView(bounds, Qt::KeepAspectRatio);
        }
    }
}