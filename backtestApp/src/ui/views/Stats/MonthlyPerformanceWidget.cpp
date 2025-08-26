#include "ui/views/Stats/MonthlyPerformanceWidget.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <algorithm>
#include <cmath>

MonthlyPerformanceWidget::MonthlyPerformanceWidget(QWidget* parent)
    : QWidget(parent),
      m_minYear(0),
      m_maxYear(0),
      m_minValue(0.0),
      m_maxValue(0.0)
{
    // Création du layout principal
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Création du groupe box
    m_groupBox = new QGroupBox("Performance Mensuelle par Année");
    m_mainLayout->addWidget(m_groupBox);
    
    // Layout interne du groupbox
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    groupLayout->setContentsMargins(10, 20, 10, 10);
    
    // Création de la scène et de la vue
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(350);
    m_view->setMinimumWidth(750);  // Plus large pour accommoder la légende
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setAlignment(Qt::AlignCenter);
    
    // Ajout de la vue au layout
    groupLayout->addWidget(m_view);
    
    // Installation d'un filtre d'événements pour gérer le redimensionnement
    m_view->viewport()->installEventFilter(this);
}

MonthlyPerformanceWidget::~MonthlyPerformanceWidget()
{
    // Qt se charge de la destruction des objets enfants
}

void MonthlyPerformanceWidget::analyzeTradesByMonthAndYear(const std::vector<be::TradeData>& trades)
{
    // Réinitialiser les données
    m_performanceData.clear();
    m_tradeCountData.clear();
    
    // Réinitialiser les valeurs min/max
    m_minValue = 0.0;
    m_maxValue = 0.0;
    m_minYear = 0;
    m_maxYear = 0;
    bool firstTrade = true;
    
    // Analyser chaque trade
    for (const auto& trade : trades) {
        int year = trade.entryDate.year;
        int month = trade.entryDate.month;
        
        // Mise à jour des années min et max
        if (firstTrade) {
            m_minYear = m_maxYear = year;
            firstTrade = false;
        } else {
            m_minYear = std::min(m_minYear, year);
            m_maxYear = std::max(m_maxYear, year);
        }
        
        // Ajouter la performance et incrémenter le compteur
        m_performanceData[year][month] += trade.pl;
        m_tradeCountData[year][month]++;
    }
    
    // Si aucun trade, sortir
    if (firstTrade) return;
    
    // Trouver les valeurs min et max pour l'échelle de couleur
    bool firstValue = true;
    for (auto yearIt = m_performanceData.begin(); yearIt != m_performanceData.end(); ++yearIt) {
        for (auto monthIt = yearIt.value().begin(); monthIt != yearIt.value().end(); ++monthIt) {
            double value = monthIt.value();
            
            if (firstValue) {
                m_minValue = m_maxValue = value;
                firstValue = false;
            } else {
                m_minValue = std::min(m_minValue, value);
                m_maxValue = std::max(m_maxValue, value);
            }
        }
    }
    
    // Équilibrer les bornes pour que les valeurs absolues soient égales (symétrie)
    double absMax = std::max(std::fabs(m_minValue), std::fabs(m_maxValue));
    m_minValue = -absMax;
    m_maxValue = absMax;
    
    // S'assurer que min et max ne sont pas identiques pour éviter division par zéro
    if (std::abs(m_maxValue - m_minValue) < 1e-6) {
        m_minValue = -1.0;
        m_maxValue = 1.0;
    }
    
    // Ajouter une petite marge pour la visualisation
    double margin = (m_maxValue - m_minValue) * 0.05;
    m_minValue -= margin;
    m_maxValue += margin;
}

QColor MonthlyPerformanceWidget::getColorForValue(double value)
{
    // Assurer que la valeur est dans la plage [min, max]
    value = std::max(m_minValue, std::min(value, m_maxValue));
    
    // Normaliser la valeur entre -1 et 1
    double range = m_maxValue - m_minValue;
    double normalizedValue = range != 0 ? 2.0 * (value - m_minValue) / range - 1.0 : 0.0;
    
    // Échelle de couleur symétrique
    if (normalizedValue < 0) {
        // De rouge à jaune pour les valeurs négatives
        double ratio = 1.0 + normalizedValue; // 0 à 1 (de min à 0)
        
        // Rouge toujours maximum pour les valeurs négatives
        int red = 255;
        
        // Vert varie de 0 à 255
        int green = static_cast<int>(255 * ratio);
        
        // Bleu toujours à 0
        int blue = 0;
        
        return QColor(red, green, blue);
    } 
    else {
        // De jaune à vert pour les valeurs positives
        double ratio = normalizedValue; // 0 à 1 (de 0 à max)
        
        // Rouge varie de 255 à 0
        int red = static_cast<int>(255 * (1.0 - ratio));
        
        // Vert toujours à 255 pour les valeurs positives
        int green = 255;
        
        // Bleu toujours à 0
        int blue = 0;
        
        return QColor(red, green, blue);
    }
}

void MonthlyPerformanceWidget::buildHeatmap()
{
    // Effacer la scène
    m_scene->clear();
    
    // Si pas de données, sortir
    if (m_minYear == 0 || m_maxYear == 0) {
        m_groupBox->setTitle("Performance Mensuelle par Année (pas de données)");
        return;
    }
    
    // Nombre d'années à afficher
    int yearCount = m_maxYear - m_minYear + 1;
    
    // Ajuster la taille des cellules en fonction du nombre d'années
    int adjustedCellSize = std::min(CELL_SIZE, 300 / yearCount);
    
    // Marges pour les labels
    int leftMargin = 60;   // Pour les labels d'années
    int topMargin = 40;    // Pour les labels de mois
    int rightMargin = 180; // Pour la légende
    
    // Calculer le nombre total de trades
    int totalTrades = 0;
    double totalPerformance = 0.0;
    
    for (auto yearIt = m_tradeCountData.begin(); yearIt != m_tradeCountData.end(); ++yearIt) {
        for (auto monthIt = yearIt.value().begin(); monthIt != yearIt.value().end(); ++monthIt) {
            totalTrades += monthIt.value();
        }
    }
    
    for (auto yearIt = m_performanceData.begin(); yearIt != m_performanceData.end(); ++yearIt) {
        for (auto monthIt = yearIt.value().begin(); monthIt != yearIt.value().end(); ++monthIt) {
            totalPerformance += monthIt.value();
        }
    }
    
    // Ajouter un titre
    QGraphicsTextItem* titleText = m_scene->addText(
        QString("Distribution de %1 trades (%2 $)")
            .arg(totalTrades)
            .arg(totalPerformance, 0, 'f', 2)
    );
    QFont titleFont = titleText->font();
    titleFont.setPointSize(10);
    // titleFont.setBold(true);
    titleText->setFont(titleFont);
    titleText->setPos(leftMargin + 100, 5);
    
    // Dessiner les labels des mois (en haut)
    for (int m = 0; m < MONTHS_IN_YEAR; m++) {
        QGraphicsTextItem* monthLabel = m_scene->addText(m_monthNames[m]);
        QFont monthFont = monthLabel->font();
        monthLabel->setFont(monthFont);
        
        // Centrer le texte sur la colonne
        QRectF textRect = monthLabel->boundingRect();
        monthLabel->setPos(leftMargin + m * (adjustedCellSize + CELL_SPACING) + 
                          (adjustedCellSize - textRect.width())/2, topMargin - 25);
    }
    
    // Dessiner les labels des années (à gauche)
    for (int y = 0; y <= m_maxYear - m_minYear; y++) {
        int year = m_minYear + y;
        QGraphicsTextItem* yearLabel = m_scene->addText(QString::number(year));
        QFont yearFont = yearLabel->font();
        yearLabel->setFont(yearFont);
        
        // Aligner à droite
        QRectF textRect = yearLabel->boundingRect();
        yearLabel->setPos(leftMargin - textRect.width() - 10, 
                         topMargin + y * (adjustedCellSize + CELL_SPACING) + 
                         (adjustedCellSize - textRect.height())/2);
    }
    
    // Dessiner la heatmap
    for (int y = 0; y <= m_maxYear - m_minYear; y++) {
        int year = m_minYear + y;
        
        for (int m = 0; m < MONTHS_IN_YEAR; m++) {
            int month = m + 1; // Les mois commencent à 1
            
            // Position de la cellule
            int x = leftMargin + m * (adjustedCellSize + CELL_SPACING);
            int y_pos = topMargin + y * (adjustedCellSize + CELL_SPACING);
            
            // Vérifier s'il y a des trades pour cette cellule
            if (!m_tradeCountData.contains(year) || !m_tradeCountData[year].contains(month) || 
                m_tradeCountData[year][month] <= 0) {
                // Cellule grise pour les périodes sans trades
                m_scene->addRect(
                    x, y_pos, adjustedCellSize, adjustedCellSize,
                    QPen(Qt::black, 0.5), QBrush(QColor(240, 240, 240))
                );
                continue;
            }
            
            double value = m_performanceData[year][month];
            QColor cellColor = getColorForValue(value);
            
            // Ajouter un rectangle avec une bordure fine
            QGraphicsRectItem* cell = m_scene->addRect(
                x, y_pos, adjustedCellSize, adjustedCellSize,
                QPen(Qt::black, 0.5), QBrush(cellColor)
            );
            
            // Ajouter un tooltip
            QString tooltipText = QString("%1 %2\nPnL: %3 $\nTrades: %4")
                                   .arg(m_monthNames[m])
                                   .arg(year)
                                   .arg(value, 0, 'f', 2)
                                   .arg(m_tradeCountData[year][month]);
            cell->setToolTip(tooltipText);
            
            // Afficher le nombre de trades dans chaque cellule
            QGraphicsTextItem* countText = m_scene->addText(QString::number(m_tradeCountData[year][month]));
            QFont countFont = countText->font();
            countText->setFont(countFont);
            
            // Centrer le texte dans la cellule
            QRectF textRect = countText->boundingRect();
            countText->setPos(x + (adjustedCellSize - textRect.width())/2, 
                             y_pos + (adjustedCellSize - textRect.height())/2);
            
            // Ajuster la couleur du texte pour la lisibilité
            QColor textColor = QColor::fromHsv(cellColor.hue(), 
                                              cellColor.saturation(),
                                              cellColor.value() < 128 ? 240 : 30);
            countText->setDefaultTextColor(textColor);
        }
    }
    
    // *** LÉGENDE VERTICALE À DROITE ***
    // Position de départ de la légende
    int legendX = leftMargin + MONTHS_IN_YEAR * (adjustedCellSize + CELL_SPACING) + 30;
    int legendY = topMargin + 10;
    int legendWidth = 30;
    int legendHeight = yearCount * (adjustedCellSize + CELL_SPACING) - 20;
    
    // Titre de la légende
    QGraphicsTextItem* legendTitle = m_scene->addText("PnL ($)");
    QFont legendTitleFont = legendTitle->font();
    // legendTitleFont.setBold(true);
    legendTitle->setFont(legendTitleFont);
    legendTitle->setPos(legendX, legendY - 25);
    
    // Gradient vertical pour la légende
    QLinearGradient gradient(0, legendY + legendHeight, 0, legendY);
    
    // Points d'arrêt pour le gradient (rouge-orange-jaune-vert)
    gradient.setColorAt(0.0, getColorForValue(m_minValue));      // Rouge pour min
    gradient.setColorAt(0.25, getColorForValue(m_minValue/2));   // Orange-rouge
    gradient.setColorAt(0.5, getColorForValue(0.0));             // Jaune pour zéro
    gradient.setColorAt(0.75, getColorForValue(m_maxValue/2));   // Vert-jaune
    gradient.setColorAt(1.0, getColorForValue(m_maxValue));      // Vert pour max
    
    // Rectangle du gradient avec bordure
    m_scene->addRect(legendX, legendY, legendWidth, legendHeight, 
                    QPen(Qt::black, 1), QBrush(gradient));
    
    // Labels des valeurs
    // Maximum (en haut)
    QGraphicsTextItem* maxText = m_scene->addText(QString("%1 $").arg(m_maxValue, 0, 'f', 2));
    QFont valueFont = maxText->font();
    maxText->setFont(valueFont);
    maxText->setPos(legendX + legendWidth + 5, legendY);
    
    // Quart positif
    QGraphicsTextItem* quarterPosText = m_scene->addText(QString("%1 $").arg(m_maxValue/2, 0, 'f', 2));
    quarterPosText->setFont(valueFont);
    quarterPosText->setPos(legendX + legendWidth + 5, legendY + legendHeight/4);
    
    // Zéro (milieu)
    QGraphicsTextItem* zeroText = m_scene->addText("0 $");
    zeroText->setFont(valueFont);
    zeroText->setPos(legendX + legendWidth + 5, legendY + legendHeight/2);
    
    // Quart négatif
    QGraphicsTextItem* quarterNegText = m_scene->addText(QString("%1 $").arg(m_minValue/2, 0, 'f', 2));
    quarterNegText->setFont(valueFont);
    quarterNegText->setPos(legendX + legendWidth + 5, legendY + 3*legendHeight/4);
    
    // Minimum (en bas)
    QGraphicsTextItem* minText = m_scene->addText(QString("%1 $").arg(m_minValue, 0, 'f', 2));
    minText->setFont(valueFont);
    minText->setPos(legendX + legendWidth + 5, legendY + legendHeight);
    
    // Ajuster la taille de la scène pour inclure tout le contenu
    QRectF boundingRect = m_scene->itemsBoundingRect();
    m_scene->setSceneRect(boundingRect);
    
    // Ajuster la vue pour afficher toute la scène
    m_view->fitInView(boundingRect, Qt::KeepAspectRatio);
    m_view->centerOn(boundingRect.center());
    
    // Mettre à jour le titre du groupbox
    m_groupBox->setTitle(QString("Performance Mensuelle par Année (%1 - %2)").arg(m_minYear).arg(m_maxYear));
}

void MonthlyPerformanceWidget::updateData(const be::Stats& stats)
{
    if (stats.trades.empty()) {
        clear();
        return;
    }
    
    analyzeTradesByMonthAndYear(stats.trades);
    buildHeatmap();
    
    qDebug() << "Monthly Performance Widget mis à jour avec" << stats.trades.size() << "trades"
             << "couvrant les années" << m_minYear << "à" << m_maxYear
             << "Échelle de PnL: [" << m_minValue << "," << m_maxValue << "]";
}

void MonthlyPerformanceWidget::clear()
{
    m_performanceData.clear();
    m_tradeCountData.clear();
    m_minYear = m_maxYear = 0;
    m_minValue = m_maxValue = 0.0;
    
    m_scene->clear();
    m_groupBox->setTitle("Performance Mensuelle par Année (pas de données)");
}

bool MonthlyPerformanceWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Resize) {
        if (!m_scene->items().isEmpty()) {
            m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MonthlyPerformanceWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // Réajuster la vue au contenu après redimensionnement
    if (!m_scene->items().isEmpty()) {
        QRectF bounds = m_scene->itemsBoundingRect();
        if (!bounds.isEmpty()) {
            m_view->fitInView(bounds, Qt::KeepAspectRatio);
        }
    }
}