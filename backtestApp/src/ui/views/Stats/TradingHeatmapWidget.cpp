#include "ui/views/Stats/TradingHeatmapWidget.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <algorithm>
#include <cmath> // Pour std::fabs

TradingHeatmapWidget::TradingHeatmapWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent),
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
    
    // Initialiser les noms des jours
    m_dayNames = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    
    // Définir une taille minimale recommandée
    setMinimumSize(600, 400);
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

    // Initialiser notre tableau de jours actifs
    m_activeDays.clear();
    m_activeDays.resize(DAYS_IN_WEEK, false);
    m_activeDayIndices.clear();
    
    // Analyser chaque trade
    for (const auto& trade : trades) {
        // Utiliser la date d'entrée pour déterminer l'heure et le jour
        int hour = trade.entryDate.hour;
        int day = getDayOfWeek(trade.entryDate);
        
        // Vérifier que les indices sont dans la plage
        if (hour < 0 || hour >= HOURS_IN_DAY || day < 0 || day >= DAYS_IN_WEEK)
            continue;

        // Ajouter la performance du trade
        m_activeDays[day] = true;
        m_performanceData[hour][day] += trade.pl;
        m_tradeCountData[hour][day]++;
        m_squaredSumData[hour][day] += trade.pl * trade.pl; // Pour l'écart-type
        
        // Mettre à jour la plage horaire
        m_minHour = std::min(m_minHour, hour);
        m_maxHour = std::max(m_maxHour, hour);
    }

    // Construire le mapping des jours actifs
    for (int d = 0; d < DAYS_IN_WEEK; d++) {
        if (m_activeDays[d]) {
            m_activeDayIndices.push_back(d);
        }
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

void TradingHeatmapWidget::paintContent(QPainter& painter, const QRect& contentRect) {
    // Calculer le nombre d'heures à afficher
    int numHoursToShow = m_maxHour - m_minHour + 1;
    
    // Si aucune donnée, rien à dessiner
    if (m_activeDayIndices.empty() || numHoursToShow <= 0) {
        painter.drawText(contentRect, Qt::AlignCenter, "Pas de données à afficher");
        return;
    }
    
    // Marges et taille des cellules
    int leftMargin = 60;  // Marge pour les labels d'heure
    int topMargin = 40;   // Marge pour les labels de jour
    int rightMargin = 120; // Marge pour la légende
    int bottomMargin = 20;
    
    // Calculer la taille des cellules en fonction de l'espace disponible
    int availableWidth = contentRect.width() - leftMargin - rightMargin;
    int availableHeight = contentRect.height() - topMargin - bottomMargin;
    
    int cellWidth = std::min(CELL_SIZE, availableWidth / std::max(1, static_cast<int>(m_activeDayIndices.size())));
    int cellHeight = std::min(CELL_SIZE, availableHeight / numHoursToShow);
    int cellSize = std::min(cellWidth, cellHeight);
    int cellSpacing = 1; // Espace entre les cellules
    
    // Calculer la position de départ (centrage horizontal)
    int startX = contentRect.left() + leftMargin;
    int startY = contentRect.top() + topMargin;
    
    // Calculer la largeur totale de la heatmap
    int heatmapWidth = m_activeDayIndices.size() * (cellSize + cellSpacing) - cellSpacing;
    
    // Dessiner les labels des jours (en haut)
    painter.save();
    QFont dayFont = painter.font();
    dayFont.setBold(true);
    painter.setFont(dayFont);
    
    for (size_t i = 0; i < m_activeDayIndices.size(); i++) {
        int d = m_activeDayIndices[i];
        QString dayName = m_dayNames[d];
        QRect textRect(startX + i * (cellSize + cellSpacing), 
                      startY - 25, 
                      cellSize, 
                      20);
        painter.drawText(textRect, Qt::AlignCenter, dayName);
    }
    painter.restore();
    
    // Dessiner les labels des heures (à gauche)
    painter.save();
    QFont hourFont = painter.font();
    painter.setFont(hourFont);
    
    for (int h = m_minHour; h <= m_maxHour; h++) {
        int rowIndex = h - m_minHour;
        int y = startY + rowIndex * (cellSize + cellSpacing) + cellSize/2;
        
        QString hourText = QString::number(h) + "h";
        QRect textRect(startX - 50, y - 10, 45, 20);
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, hourText);
    }
    painter.restore();
    
    // Dessiner la heatmap
    for (int h = m_minHour; h <= m_maxHour; h++) {
        int rowIndex = h - m_minHour;
        
        for (size_t i = 0; i < m_activeDayIndices.size(); i++) {
            int d = m_activeDayIndices[i];
            
            // Position de la cellule
            int x = startX + i * (cellSize + cellSpacing);
            int y = startY + rowIndex * (cellSize + cellSpacing);
            
            // Créer le rectangle de la cellule
            QRect cellRect(x, y, cellSize, cellSize);
            
            // Vérifier s'il y a des trades pour cette cellule
            if (m_tradeCountData[h][d] <= 0) {
                // Cellule grise pour les périodes sans trades
                painter.fillRect(cellRect, QColor(240, 240, 240));
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
            
            // Couleur de la cellule basée sur l'espérance
            QColor cellColor = getColorForValue(expectation);
            painter.fillRect(cellRect, cellColor);
            
            // Texte pour l'espérance et l'écart-type
            QString expText = QString("%1").arg(expectation, 0, 'f', 1);
            QString stddevText = QString("%1").arg(stddev, 0, 'f', 0);
            
            // Ajuster la couleur du texte pour la lisibilité
            QColor textColor = QColor::fromHsv(cellColor.hue(), 
                                               cellColor.saturation(),
                                               cellColor.value() < 128 ? 240 : 30);
            
            // Dessiner l'espérance (en haut)
            painter.save();
            QFont expFont = painter.font();
            expFont.setBold(true);
            expFont.setPointSize(10);
            painter.setFont(expFont);
            painter.setPen(textColor);
            painter.drawText(QRect(x, y, cellSize, cellSize/2), 
                            Qt::AlignCenter, 
                            expText);
            
            // Dessiner l'écart-type (en bas)
            QFont stddevFont = painter.font();
            stddevFont.setPointSize(8);
            painter.setFont(stddevFont);
            painter.drawText(QRect(x, y + cellSize/2, cellSize, cellSize/2), 
                            Qt::AlignCenter, 
                            stddevText);
            painter.restore();
        }
    }
    
    // Dessiner la légende verticale
    int legendX = startX + heatmapWidth + 40;
    int legendY = startY;
    int legendWidth = 20;
    int legendHeight = numHoursToShow * (cellSize + cellSpacing) - cellSpacing;
    
    // Titre de la légende
    painter.save();
    painter.drawText(QRect(legendX, legendY - 25, 80, 20), 
                    Qt::AlignLeft | Qt::AlignVCenter, 
                    "E/σ (€)");
    
    // Rectangle du gradient
    QLinearGradient gradient(0, legendY + legendHeight, 0, legendY);
    gradient.setColorAt(0.0, getColorForValue(m_minValue));
    gradient.setColorAt(0.25, getColorForValue(m_minValue/2));
    gradient.setColorAt(0.5, getColorForValue(0.0));
    gradient.setColorAt(0.75, getColorForValue(m_maxValue/2));
    gradient.setColorAt(1.0, getColorForValue(m_maxValue));
    
    QRect gradientRect(legendX, legendY, legendWidth, legendHeight);
    painter.fillRect(gradientRect, gradient);
    painter.drawRect(gradientRect);
    
    // Labels de la légende
    QFont valueFont = painter.font();
    painter.setFont(valueFont);
    
    // Maximum
    painter.drawText(QRect(legendX + legendWidth + 5, legendY - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1 €").arg(m_maxValue, 0, 'f', 2));
    
    // Quart positif
    painter.drawText(QRect(legendX + legendWidth + 5, legendY + legendHeight/4 - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1 €").arg(m_maxValue/2, 0, 'f', 2));
    
    // Zéro
    painter.drawText(QRect(legendX + legendWidth + 5, legendY + legendHeight/2 - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    "0 €");
    
    // Quart négatif
    painter.drawText(QRect(legendX + legendWidth + 5, legendY + 3*legendHeight/4 - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1 €").arg(m_minValue/2, 0, 'f', 2));
    
    // Minimum
    painter.drawText(QRect(legendX + legendWidth + 5, legendY + legendHeight - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1 €").arg(m_minValue, 0, 'f', 2));
    
    painter.restore();
}

bool TradingHeatmapWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_view->viewport() && event->type() == QEvent::Resize) {
        if (!m_scene->items().isEmpty()) {
            m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TradingHeatmapWidget::updateContent(const std::vector<be::TradeData>& trades) {
    if (trades.empty()) {
        return;
    }
    
    // Analyser les trades par heure et jour
    analyzeTradesByTimeAndDay(trades);
    
    // Construire la heatmap
    // buildHeatmap();
    update();
}

