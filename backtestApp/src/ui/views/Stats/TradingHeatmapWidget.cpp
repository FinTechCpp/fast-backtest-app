#include "ui/views/Stats/TradingHeatmapWidget.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <algorithm>
#include <cmath> // Pour std::fabs

TradingHeatmapWidget::TradingHeatmapWidget(QWidget* parent)
    : QWidget(parent),
      m_minValue(0.0),
      m_maxValue(0.0),
      m_minHour(24),    // Initialiser à une valeur extrême
      m_maxHour(0)      // Initialiser à une valeur minimale
{
    // Initialiser les données
    m_performanceData.resize(HOURS_IN_DAY);
    m_tradeCountData.resize(HOURS_IN_DAY);
    
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        m_performanceData[h].resize(DAYS_IN_WEEK);
        m_tradeCountData[h].resize(DAYS_IN_WEEK);
        
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
        }
    }
    
    // Créer le groupe box principal
    m_groupBox = new QGroupBox("Analyse des Performances par Heure et Jour");
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_groupBox);
    
    // Layout interne - Utiliser QHBoxLayout au lieu de QVBoxLayout pour mettre la légende à droite
    QHBoxLayout* groupLayout = new QHBoxLayout(m_groupBox);
    groupLayout->setContentsMargins(5, 15, 5, 5); // Réduire les marges pour maximiser l'espace
    
    // Layout vertical pour le contenu principal (graphique + explication)
    QVBoxLayout* mainContentLayout = new QVBoxLayout();
    groupLayout->addLayout(mainContentLayout, 1); // Priorité d'expansion plus élevée pour le contenu principal
    
    // Créer la scène et la vue pour la heatmap
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(400);
    m_view->setMinimumWidth(700);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setAlignment(Qt::AlignCenter);
    
    // Ajouter la vue de la heatmap au layout principal
    mainContentLayout->addWidget(m_view, 1);
    
    // Créer la scène et la vue pour la légende
    m_legendScene = new QGraphicsScene(this);
    m_legendView = new QGraphicsView(m_legendScene);
    m_legendView->setRenderHint(QPainter::Antialiasing, true);
    m_legendView->setMinimumWidth(150);  // Largeur minimale pour la légende
    m_legendView->setMinimumHeight(400); // Hauteur minimale pour la légende
    m_legendView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_legendView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_legendView->setAlignment(Qt::AlignCenter);
    
    // Ajouter la légende à droite de la heatmap
    groupLayout->addWidget(m_legendView);
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

            // Mettre à jour min/max
            if (firstValue || m_performanceData[h][d] < m_minValue) {
                m_minValue = m_performanceData[h][d];
            }
            
            if (firstValue || m_performanceData[h][d] > m_maxValue) {
                m_maxValue = m_performanceData[h][d];
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
    
    // Assurer une petite marge pour mieux visualiser les différences
    double margin = (m_maxValue - m_minValue) * 0.05;  // Réduire la marge à 5%
    m_minValue -= margin;
    m_maxValue += margin;
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
    // Effacer les scènes
    m_scene->clear();
    m_legendScene->clear();
    
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
    
    QGraphicsTextItem* titleText = m_scene->addText(QString("Distribution des %1 trades par heure et jour").arg(totalTrades));
    QFont titleFont = titleText->font();
    titleFont.setPointSize(10);
    titleText->setFont(titleFont);
    titleText->setPos(leftMargin + 100, 5);
    
    // Dessiner les labels des jours (en haut)
    for (int d = 0; d < DAYS_IN_WEEK; d++) {
        QGraphicsTextItem* dayLabel = m_scene->addText(m_dayNames[d]);
        QFont dayFont = dayLabel->font();
        dayLabel->setFont(dayFont);
        // dayLabel->setRotation(-45); // Rotation de 45 degrés
        
        // Centrer le texte sur la colonne
        QRectF textRect = dayLabel->boundingRect();
        dayLabel->setPos(leftMargin + d * (adjustedCellSize + CELL_SPACING) + 
                        (adjustedCellSize - textRect.width())/2, topMargin - 25);
    }
    
    // NOUVEAU: Dessiner les bordures d'heures aux limites des cellules
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
                    QPen(Qt::black, 0.5), QBrush(QColor(240, 240, 240))
                );
                continue;
            }
            
            double value = m_performanceData[h][d];
            QColor cellColor = getColorForValue(value);
            
            // Ajouter un rectangle avec une bordure fine
            QGraphicsRectItem* cell = m_scene->addRect(
                x, y, adjustedCellSize, adjustedCellSize,
                QPen(Qt::black, 0.5), QBrush(cellColor)
            );
            
            // Modifier le tooltip pour inclure la plage horaire
            QString tooltipText = QString("Jour: %1\nHeure: %2h - %3h\nPnL total: $%4\nTrades: %5")
                                    .arg(m_dayNames[d])
                                    .arg(h)
                                    .arg(h+1)
                                    .arg(value, 0, 'f', 2)
                                    .arg(m_tradeCountData[h][d]);
            cell->setToolTip(tooltipText);
            
            // Afficher le nombre de trades dans chaque cellule
            QGraphicsTextItem* countText = m_scene->addText(QString::number(m_tradeCountData[h][d]));
            QFont countFont = countText->font();
            countText->setFont(countFont);
            
            // Centrer le texte dans la cellule
            QRectF textRect = countText->boundingRect();
            countText->setPos(x + (adjustedCellSize - textRect.width())/2, 
                                y + (adjustedCellSize - textRect.height())/2);
            
            // Ajuster la couleur du texte pour la lisibilité
            QColor textColor = QColor::fromHsv(cellColor.hue(), 
                                                cellColor.saturation(),
                                                cellColor.value() < 128 ? 240 : 30);
            countText->setDefaultTextColor(textColor);
        }
    }
    
    // Ajuster la vue pour afficher toute la scène
    QRectF boundingRect = m_scene->itemsBoundingRect();
    m_scene->setSceneRect(boundingRect);
    m_view->fitInView(boundingRect, Qt::KeepAspectRatio);
    m_view->centerOn(boundingRect.center());
    
    // *** NOUVELLE LÉGENDE VERTICALE À DROITE ***
    // Comme elle est maintenant dans son propre widget à droite, ajustons ses dimensions
    
    // Dimensions de la légende
    const int legendHeight = 350;
    const int legendWidth = 30;
    const int legendX = 30;
    const int legendY = 50;
    
    // Titre de la légende
    QGraphicsTextItem* legendTitle = m_legendScene->addText("PnL cumulé ($)");
    QFont legendTitleFont = legendTitle->font();
    legendTitle->setFont(legendTitleFont);
    legendTitle->setPos(legendX, 10);
    
    // Gradient vertical (de bas en haut)
    QLinearGradient gradient(0, legendY + legendHeight, 0, legendY);
    
    // Points d'arrêt pour le gradient (rouge-orange-jaune-vert)
    gradient.setColorAt(0.0, getColorForValue(m_minValue));        // Rouge pour min
    gradient.setColorAt(0.25, getColorForValue(m_minValue/2));     // Orange
    gradient.setColorAt(0.5, getColorForValue(0.0));               // Jaune pour zéro
    gradient.setColorAt(0.75, getColorForValue(m_maxValue/2));     // Jaune-vert
    gradient.setColorAt(1.0, getColorForValue(m_maxValue));        // Vert pour max
    
    // Rectangle du gradient avec bordure
    m_legendScene->addRect(legendX, legendY, legendWidth, legendHeight, 
                          QPen(Qt::black, 1), QBrush(gradient));
    
    // Labels des valeurs à droite du rectangle
    
    // Maximum (en haut)
    QGraphicsTextItem* maxText = m_legendScene->addText(QString("$%1").arg(m_maxValue, 0, 'f', 2));
    QFont valueFont = maxText->font();
    maxText->setFont(valueFont);
    maxText->setPos(legendX + legendWidth + 5, legendY - maxText->boundingRect().height()/2);
    
    // Quart positif
    QGraphicsTextItem* quarterPosText = m_legendScene->addText(QString("$%1").arg(m_maxValue/2, 0, 'f', 2));
    quarterPosText->setFont(valueFont);
    quarterPosText->setPos(legendX + legendWidth + 5, 
                          legendY + legendHeight/4 - quarterPosText->boundingRect().height()/2);
    
    // Zéro (milieu)
    QGraphicsTextItem* zeroText = m_legendScene->addText("$0.00");
    zeroText->setFont(valueFont);
    zeroText->setPos(legendX + legendWidth + 5, 
                    legendY + legendHeight/2 - zeroText->boundingRect().height()/2);
    
    // Quart négatif
    QGraphicsTextItem* quarterNegText = m_legendScene->addText(QString("$%1").arg(m_minValue/2, 0, 'f', 2));
    quarterNegText->setFont(valueFont);
    quarterNegText->setPos(legendX + legendWidth + 5, 
                          legendY + 3*legendHeight/4 - quarterNegText->boundingRect().height()/2);
    
    // Minimum (en bas)
    QGraphicsTextItem* minText = m_legendScene->addText(QString("$%1").arg(m_minValue, 0, 'f', 2));
    minText->setFont(valueFont);
    minText->setPos(legendX + legendWidth + 5, 
                   legendY + legendHeight - minText->boundingRect().height()/2);
    
    // Ajuster la scène de légende et la vue
    m_legendScene->setSceneRect(m_legendScene->itemsBoundingRect());
    m_legendView->fitInView(m_legendScene->sceneRect(), Qt::KeepAspectRatio);
    
    // Mettre à jour le titre du groupe box
    m_groupBox->setTitle("Analyse du PnL cumulé par Heure et Jour");
    
    m_view->viewport()->installEventFilter(this);
}

bool TradingHeatmapWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_view->viewport() && event->type() == QEvent::Resize) {
        if (!m_scene->items().isEmpty()) {
            m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TradingHeatmapWidget::updateData(const be::Stats& stats) {
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
        }
    }
    
    // Effacer les scènes
    m_scene->clear();
    m_legendScene->clear();
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
    
    if (m_legendScene && !m_legendScene->items().isEmpty()) {
        QRectF legendBounds = m_legendScene->sceneRect();
        if (!legendBounds.isEmpty()) {
            m_legendView->fitInView(legendBounds, Qt::KeepAspectRatio);
        }
    }
}