#include "ui/views/Stats/EquityWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>
#include <QPainterPath>
#include <set>

static constexpr double EPSILON_D = 1e-9;

EquityWidget::EquityWidget(const QString& title, QWidget *parent)
    : TitledWidget(title, parent),
      m_xmin(0), m_xmax(1), m_ymin(0), m_ymax(1),
      m_showCrosshair(false)
{
    setMinimumSize(160, 120);
    setMouseTracking(true); // necessary to receive mouseMoveEvent without pressing buttons

    // QCheckBox pour switcher entre m_statText et m_statTextBis
    m_checkBox = new QCheckBox("Pourcentage", this);
    m_checkBox->setChecked(false);

    // Connecter le signal toggled de la QCheckBox à un slot lambda
    connect(m_checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        invalidateCache();  // Invalider tout le cache (équités + points widget + labels)
        updateBounds();     // Recalculer les limites avec le bon ensemble de points
        update();           // Redessiner le widget
    });

    // Ajouter la QCheckBox comme widget compagnon dans le titre
    setTitleCompanionWidget(m_checkBox);
}

void EquityWidget::setPoints(const QVector<QPointF>& pts)
{
    if (pts.isEmpty()) {
        m_points.clear();
        m_pointsPercent.clear(); // Vider aussi les points en pourcentage
        invalidateCache();
        updateBounds();
        update();
        return;
    }

    std::vector<QPointF> tmp;
    tmp.reserve(pts.size());
    for (const auto &p: pts) tmp.push_back(p);

    std::sort(tmp.begin(), tmp.end(), [](const QPointF &a, const QPointF &b){
        return a.x() < b.x();
    });

    m_points.clear();
    m_points.reserve(tmp.size());
    for (const auto &p: tmp) m_points.push_back(p);

    // Calculer les points en pourcentage
    calculatePercentPoints();
    
    invalidateCache();
    updateBounds();
    update();
}

void EquityWidget::setPoints(const std::vector<be::Date>& dates, const std::vector<be::EquityPoint>& equityCurve) {
    if (dates.empty() || equityCurve.empty()) {
        m_points.clear();
        m_pointsPercent.clear();
        updateBounds();
        update();
        return;
    }
    
    QVector<QPointF> newPoints;

    // CA c'est pas opti mais bon... a corriger plus tard
    m_dates = dates; // Stocker les dates pour un usage futur
    
    // Si le tableau contient uniquement des valeurs Y
    if (!equityCurve.empty()) {
        // Créer des points avec X = index et Y = valeur
        for (size_t i = 0; i < equityCurve.size(); ++i) {
            newPoints.append(QPointF(static_cast<double>(equityCurve[i].index), equityCurve[i].value));
        }
    }

    // Dernier point
    newPoints.append(QPointF(static_cast<double>(m_dates.size()), equityCurve.back().value));
    
    setPoints(newPoints);
}

void EquityWidget::invalidateCache()
{
    m_cacheValid = false;
    m_widgetPointsValid = false;
    m_cachedDateLabels.clear();
}

double EquityWidget::getInitialEquity() const
{
    if (!m_cacheValid) {
        // Calculer et mettre en cache toutes les valeurs d'un coup
        const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
        
        if (!activePoints.isEmpty()) {
            m_cachedInitialEquity = activePoints.first().y();
            m_cachedFinalEquity = activePoints.last().y();
            
            // Calculer le peak en même temps
            double peak = activePoints.first().y();
            for (const auto& pt : activePoints) {
                if (pt.y() > peak) {
                    peak = pt.y();
                }
            }
            m_cachedPeakEquity = peak;
        } else {
            m_cachedInitialEquity = 0.0;
            m_cachedPeakEquity = 0.0;
            m_cachedFinalEquity = 0.0;
        }
        
        m_cacheValid = true;
    }
    
    return m_cachedInitialEquity;
}

double EquityWidget::getPeakEquity() const
{
    if (!m_cacheValid) {
        getInitialEquity();  // Va calculer et mettre en cache toutes les valeurs
    }
    return m_cachedPeakEquity;
}

double EquityWidget::getFinalEquity() const
{
    if (!m_cacheValid) {
        getInitialEquity();  // Va calculer et mettre en cache toutes les valeurs
    }
    return m_cachedFinalEquity;
}

std::vector<EquityWidget::DateLabel> EquityWidget::generateDateLabels() const {
    // Utiliser le cache si valide
    if (!m_cachedDateLabels.empty()) {
        return m_cachedDateLabels;
    }

    std::vector<DateLabel> labels;
    
    if (m_dates.empty()) {
        return labels;
    }

    // Tables de conversion mois -> texte
    static const QStringList monthNames = {"", "Jan", "Fév", "Mar", "Avr", "Mai", "Juin", 
                                          "Juil", "Aoû", "Sep", "Oct", "Nov", "Déc"};
    
    // Déterminer la plage totale des dates
    be::Date startDate;
    be::Date endDate;
    bool validDatesFound = false;

    for (const auto& date : m_dates) {
        if (date.year > 0) {
            if (!validDatesFound) {
                startDate = date;
                endDate = date;
                validDatesFound = true;
            } else {
                endDate = date;
            }
        }
    }
    
    // Si aucune date valide trouvée
    if (!validDatesFound) {
        return labels;
    }
    
    // Calculer la durée entre la première et la dernière date
    // Nouveau: seuil de 2 ans au lieu de 1 an
    bool isMoreThanTwoYears = false;
    if (endDate.year > startDate.year + 2 || 
        (endDate.year == startDate.year + 2 && endDate.month >= startDate.month)) {
        isMoreThanTwoYears = true;
    }

    int totalMonthsDiff = (endDate.year - startDate.year) * 12 + (endDate.month - startDate.month);
    bool isMoreThanTwoMonths = totalMonthsDiff > 2;
    
    // Parcourir les dates pour créer les labels
    int lastYear = m_dates[0].year;
    int lastMonth = m_dates[0].month;
    int lastDay = m_dates[0].day;

    for (size_t i = 0; i < m_dates.size(); ++i) {
        const be::Date& date = m_dates[i];
        if (date.year <= 0) continue;  // Date invalide
        
        int year = static_cast<int>(date.year);
        int month = static_cast<int>(date.month);
        int day = static_cast<int>(date.day);
        
        bool addLabel = false;
        QString labelText;
        int importance = 0;
        
        // Si période > 2 ans : uniquement les années
        if (isMoreThanTwoYears) {
            // Années uniquement
            if (year != lastYear) {
                labelText = QString::number(year);
                importance = 3; // Année
                addLabel = true;
                lastYear = year;
                lastMonth = -1;
            }
        }
        // Si période > 2 mois et <= 2 ans : tous les mois ET les années
        else if (isMoreThanTwoMonths) {
            // Année si elle change
            if (year != lastYear) {
                labelText = QString::number(year);
                importance = 3; // Année
                addLabel = true;
                lastYear = year;
                lastMonth = -1; // Réinitialiser pour afficher le mois qui suit
            }
            
            // Tous les mois (si ce n'est pas le même que le dernier affiché)
            else if (month != lastMonth) {
                labelText = monthNames[month];
                importance = 2; // Mois
                addLabel = true;
            }
            
            // Mémoriser le dernier mois affiché
            if (addLabel) {
                lastMonth = month;
            }
        }
        // Si période <= 2 mois : tous les jours, mois et années
        else {
            // Année si elle change
            if (year != lastYear) {
                labelText = QString::number(year);
                importance = 3; // Année
                addLabel = true;
                lastYear = year;
                lastMonth = -1; // Réinitialiser pour afficher le mois qui suit
            }
            // Mois si il change
            else if (month != lastMonth) {
                labelText = monthNames[month];
                importance = 2; // Mois
                addLabel = true;
                lastMonth = month;
                lastDay = day;
            }
            // Jour (toujours)
            else if (day != lastDay) {
                labelText = QString::number(static_cast<int>(date.day));
                importance = 1; // Jour
                addLabel = true;
                lastDay = day;
            }
        }
        
        if (addLabel) {
            labels.push_back({static_cast<double>(i), labelText, importance});
        }
    }
    
    // Mettre en cache le résultat
    m_cachedDateLabels = labels;
    
    return labels;
}

void EquityWidget::calculatePercentPoints()
{
    if (m_points.isEmpty()) {
        m_pointsPercent.clear();
        return;
    }
    
    // Premier point (référence à 0%)
    double initialValue = m_points.first().y();
    if (std::abs(initialValue) < EPSILON_D) {
        initialValue = 1.0; // Éviter la division par zéro
    }
    
    m_pointsPercent.resize(m_points.size());
    for (int i = 0; i < m_points.size(); ++i) {
        double xval = m_points[i].x();
        double yval = m_points[i].y();
        double percentValue = (yval / initialValue - 1.0) * 100.0;
        m_pointsPercent[i] = QPointF(xval, percentValue);
    }
}

void EquityWidget::updateBounds()
{
    const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
    
    if (activePoints.isEmpty()) {
        m_xmin = 0; m_xmax = 1;
        m_ymin = 0; m_ymax = 1;
        return;
    }
    
    m_xmin = m_xmax = activePoints[0].x();
    m_ymin = m_ymax = activePoints[0].y();
    for (const auto &p : activePoints) {
        m_xmin = qMin(m_xmin, p.x());
        m_xmax = qMax(m_xmax, p.x());
        m_ymin = qMin(m_ymin, p.y());
        m_ymax = qMax(m_ymax, p.y());
    }
}

void EquityWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    // 1. Définir la zone de contenu (avec marges extérieures)
    m_contentRect = contentRect.adjusted(m_margin, m_margin, -m_margin, -m_margin);
    
    // 2. Définir la zone du graphique (sans les marges pour axes/labels)
    m_plotRect = QRect(
        m_contentRect.left() + m_leftMargin,
        m_contentRect.top() + m_topMargin,
        m_contentRect.width() - m_leftMargin - m_rightMargin,
        m_contentRect.height() - m_topMargin - m_bottomMargin
    );
    
    // 2b. Invalider le cache des points widget si la taille du plotRect a changé
    if (m_plotRect.size() != m_cachedPlotSize) {
        m_widgetPointsValid = false;
        m_cachedPlotSize = m_plotRect.size();
    }
    
    // 3. Recalculer les limites selon le mode actuel
    updateBounds();
    
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 4. Background du contenu
    painter.fillRect(m_contentRect, Qt::white);

    // 5. Background de la zone de tracé (légèrement différent pour bien voir les limites)
    painter.fillRect(m_plotRect, QColor(250, 250, 250));

    // 6. Dessiner la grille (dans m_plotRect uniquement)
    drawGrid(painter);

    // 7. Dessiner les axes (autour de m_plotRect)
    drawAxes(painter);

    // 8. Sélectionner le bon ensemble de points selon le mode
    const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;

    // 8b. Dessiner les zones colorées sous la courbe (AVANT la courbe elle-même)
    if (!activePoints.isEmpty()) {
        drawFilledAreas(painter);
    }

    // 9. Dessiner la courbe (clippée dans m_plotRect)
    if (!activePoints.isEmpty()) {
        painter.setClipRect(m_plotRect);
        
        // Utiliser les points widget en cache (conversion une seule fois)
        const QVector<QPointF>& widgetPoints = getCachedWidgetPoints();
        
        // Dessiner la polyligne (sans fermer le chemin)
        QPen linePen(Qt::blue);
        linePen.setWidth(2);
        linePen.setCapStyle(Qt::RoundCap);
        linePen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(linePen);
        painter.setBrush(Qt::NoBrush);  // Important: pas de remplissage
        painter.drawPolyline(widgetPoints.data(), widgetPoints.size());
        
        painter.setClipping(false);
    }

    // 10. Dessiner les markers d'equity (lignes initial/peak + highlight final)
    if (!activePoints.isEmpty()) {
        drawEquityMarkers(painter);
    }

    // 11. Dessiner le crosshair
    if (m_showCrosshair) {
        QPen crossPen(Qt::black);
        crossPen.setStyle(Qt::DashLine);
        crossPen.setWidth(1);
        painter.setPen(crossPen);

        // La position de la souris est déjà en coordonnées widget
        QPoint clampedPos = m_mousePos;
        
        // Clamper dans m_plotRect
        clampedPos.setX(qBound(m_plotRect.left(), clampedPos.x(), m_plotRect.right()));
        clampedPos.setY(qBound(m_plotRect.top(), clampedPos.y(), m_plotRect.bottom()));

        // Dessiner les lignes du crosshair
        painter.drawLine(clampedPos.x(), m_plotRect.top(),
                         clampedPos.x(), m_plotRect.bottom());
        painter.drawLine(m_plotRect.left(), clampedPos.y(),
                         m_plotRect.right(), clampedPos.y());

        // Calculer les coordonnées monde
        QPointF world = mapToWorld(clampedPos);
        QString info;
        
        // Formater différemment selon le mode
        if (m_checkBox->isChecked()) {
            info = formatValue(world.y(), true, true, false);
        } else {
            info = formatValue(world.y(), true, false, false);
        }
        
        // Positionner l'info box intelligemment
        QRect infoRect(clampedPos.x() + 10, clampedPos.y() - 25, 70, 20);
        
        // Ajuster si ça sort du plotRect
        if (infoRect.right() > m_plotRect.right()) {
            infoRect.moveLeft(clampedPos.x() - infoRect.width() - 10);
        }
        if (infoRect.top() < m_plotRect.top()) {
            infoRect.moveTop(clampedPos.y() + 10);
        }
        
        painter.fillRect(infoRect, QColor(255, 255, 224, 240));
        painter.setPen(Qt::transparent);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(infoRect);
        painter.setPen(Qt::black);
        painter.drawText(infoRect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, info);
    }
}

QPointF EquityWidget::mapToWidget(const QPointF &pt) const
{
    // Mapper un point monde vers la zone de tracé (m_plotRect)
    if (m_xmax - m_xmin < EPSILON_D || m_ymax - m_ymin < EPSILON_D) {
        return QPointF(m_plotRect.center());
    }
    
    double nx = (pt.x() - m_xmin) / (m_xmax - m_xmin);
    double ny = (pt.y() - m_ymin) / (m_ymax - m_ymin);
    
    double x = m_plotRect.left() + nx * m_plotRect.width();
    double y = m_plotRect.bottom() - ny * m_plotRect.height();
    
    return QPointF(x, y);
}

QPointF EquityWidget::mapToWorld(const QPointF &pixel) const
{
    // Mapper un pixel widget vers les coordonnées monde
    if (m_plotRect.width() <= 0 || m_plotRect.height() <= 0) {
        return QPointF(m_xmin, m_ymin);
    }
    
    double nx = (pixel.x() - m_plotRect.left()) / m_plotRect.width();
    double ny = (m_plotRect.bottom() - pixel.y()) / m_plotRect.height();
    
    double wx = m_xmin + nx * (m_xmax - m_xmin);
    double wy = m_ymin + ny * (m_ymax - m_ymin);
    
    return QPointF(wx, wy);
}

const QVector<QPointF>& EquityWidget::getCachedWidgetPoints() const
{
    // Si le cache est invalide, recalculer les points widget
    if (!m_widgetPointsValid) {
        const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
        
        m_cachedWidgetPoints.clear();
        m_cachedWidgetPoints.reserve(activePoints.size());
        
        for (const auto& pt : activePoints) {
            m_cachedWidgetPoints.append(mapToWidget(pt));
        }
        
        m_widgetPointsValid = true;
    }
    
    return m_cachedWidgetPoints;
}

void EquityWidget::drawGrid(QPainter &painter)
{
    // Dessiner la grille uniquement dans m_plotRect
    const int desiredLines = 8;
    double xrange = m_xmax - m_xmin;
    double yrange = m_ymax - m_ymin;
    if (xrange <= EPSILON_D || yrange <= EPSILON_D) return;

    // Calculer un pas "joli" pour les lignes de grille
    auto niceStep = [](double range, int target){
        if (range <= 0) return 1.0;
        double raw = range / target;
        double expv = qPow(10.0, qFloor(qLn(raw) / qLn(10.0)));
        double f = raw / expv;
        double nicef;
        if (f < 1.5) nicef = 1.0;
        else if (f < 3.0) nicef = 2.0;
        else if (f < 7.0) nicef = 5.0;
        else nicef = 10.0;
        return nicef * expv;
    };

    double xstep = niceStep(xrange, desiredLines);
    double ystep = niceStep(yrange, desiredLines);

    QPen gridPen(QColor(220, 220, 220));
    gridPen.setWidth(1);
    painter.setPen(gridPen);

    // Lignes verticales (parallèles à Y)
    double xstart = std::ceil(m_xmin / xstep) * xstep;
    for (double x = xstart; x <= m_xmax; x += xstep) {
        QPointF top = mapToWidget(QPointF(x, m_ymax));
        QPointF bottom = mapToWidget(QPointF(x, m_ymin));
        
        // S'assurer que les lignes restent dans m_plotRect
        top.setX(qBound((double)m_plotRect.left(), top.x(), (double)m_plotRect.right()));
        bottom.setX(qBound((double)m_plotRect.left(), bottom.x(), (double)m_plotRect.right()));
        
        painter.drawLine(top, bottom);
    }

    // Lignes horizontales (parallèles à X)
    double ystart = std::ceil(m_ymin / ystep) * ystep;
    for (double y = ystart; y <= m_ymax; y += ystep) {
        QPointF left = mapToWidget(QPointF(m_xmin, y));
        QPointF right = mapToWidget(QPointF(m_xmax, y));
        
        // S'assurer que les lignes restent dans m_plotRect
        left.setY(qBound((double)m_plotRect.top(), left.y(), (double)m_plotRect.bottom()));
        right.setY(qBound((double)m_plotRect.top(), right.y(), (double)m_plotRect.bottom()));
        
        painter.drawLine(left, right);
    }
}

QString EquityWidget::formatValue(double value, bool useThousandsSeparator, bool isPercent, bool roundValue) const {
    // Gérer les valeurs proches de zéro
    if (std::abs(value) < 0.01) {
        return isPercent ? "0%" : "0";
    }

    // Si on est en mode pourcentage
    if (isPercent) {
        // Formater avec 2 décimales pour les pourcentages
        return QString::number(value, 'f', 1) + "%";
    }

    if (!roundValue) {
        // Ne pas arrondir, juste formater directement
        if (useThousandsSeparator) {
            QLocale locale;
            return locale.toString(value, 'f', 1);
        } else {
            return QString::number(value, 'f', 1);
        }
    }

    // Trouver l'ordre de grandeur pour l'arrondi (reste du code inchangé)
    double absValue = std::abs(value);
    int digits = std::floor(std::log10(absValue));
    double factor;
    
    // Déterminer le facteur d'arrondi selon l'ordre de grandeur
    if (absValue >= 100000) {
        factor = std::pow(10, digits - 1);  // Arrondi aux 10000
    } else if (absValue >= 10000) {
        factor = 1000;  // Arrondi aux 1000
    } else if (absValue >= 1000) {
        factor = 100;   // Arrondi aux 100
    } else if (absValue >= 100) {
        factor = 10;    // Arrondi aux 10
    } else {
        factor = 1;     // Arrondi aux unités
    }
    
    // Arrondir à la précision déterminée
    double rounded = std::round(value / factor) * factor;
    
    // Formater avec QLocale pour les séparateurs de milliers
    if (useThousandsSeparator) {
        QLocale locale;
        return locale.toString(rounded, 'f', rounded < 10 ? 1 : 0);
    } else {
        return QString::number(rounded, 'f', rounded < 10 ? 1 : 0);
    }
}

void EquityWidget::drawAxes(QPainter &painter)
{
    QPen axisPen(Qt::black);
    axisPen.setWidth(1);
    painter.setPen(axisPen);

    // Dessiner l'axe X en bas de m_plotRect
    painter.drawLine(m_plotRect.bottomLeft(), m_plotRect.bottomRight());
    
    // Dessiner l'axe Y à droite de m_plotRect
    painter.drawLine(m_plotRect.topRight(), m_plotRect.bottomRight());

    QFontMetrics fm(font());

    // Axe Y - Labels de valeurs (à droite)
    const int yTicks = 5;
    double yrange = m_ymax - m_ymin;
    if (yrange <= EPSILON_D) return;
    
    double ystep = yrange / yTicks;
    
    // Récupérer la valeur finale pour le highlight
    double finalEquity = getFinalEquity();
    QPointF finalWidgetPos = mapToWidget(QPointF(m_xmax, finalEquity));
    
    for (int i = 0; i <= yTicks; ++i) {
        double yv = m_ymin + i * ystep;
        QPointF wp = mapToWidget(QPointF(m_xmax, yv));
        
        // Tick à droite de l'axe Y
        painter.drawLine(QPointF(m_plotRect.right(), wp.y()), 
                         QPointF(m_plotRect.right() + 4, wp.y()));
        
        // Label aligné à gauche après le tick
        QString txt = formatValue(yv, true, m_checkBox->isChecked());
        painter.drawText(QPointF(m_plotRect.right() + 8, wp.y() + fm.ascent() / 2 - 2), txt);
    }
    
    // ==================== Highlight de la valeur INITIAL ====================
    // Dessiner un label spécial pour la valeur initiale (comme le final)
    double initialEquity = getInitialEquity();
    QPointF initialWidgetPos = mapToWidget(QPointF(m_xmax, initialEquity));
    
    QString initialText = formatValue(initialEquity, true, m_checkBox->isChecked(), false);
    
    QFont boldFont = painter.font();
    boldFont.setWeight(QFont::DemiBold);
    painter.setFont(boldFont);
    QFontMetrics fmBold(boldFont);
    
    int textWidth = fmBold.horizontalAdvance(initialText);
    int textHeight = fmBold.height();
    
    // Rectangle pour le highlight (aligné avec les labels Y)
    QRect initialHighlightRect(m_plotRect.right() + 6, initialWidgetPos.y() - textHeight / 2 - 3,
                              textWidth + 10, textHeight + 6);
    
    // Fond gris clair pour le highlight
    painter.setPen(QPen(Qt::transparent));
    painter.setBrush(QColor(230, 230, 230));
    painter.drawRoundedRect(initialHighlightRect, 1, 1);
    
    painter.setPen(Qt::black);
    painter.drawText(initialHighlightRect, Qt::AlignCenter, initialText);
    
    // Restaurer la police normale
    painter.setFont(font());
    painter.setPen(Qt::black);
    
    // ==================== Highlight de la valeur FINAL ====================
    // Dessiner un label spécial pour la valeur finale
    QString finalText = formatValue(finalEquity, true, m_checkBox->isChecked(), false); // "Final: " + 
    
    boldFont = painter.font();
    boldFont.setWeight(QFont::DemiBold);
    painter.setFont(boldFont);
    fmBold = QFontMetrics(boldFont);
    
    textWidth = fmBold.horizontalAdvance(finalText);
    textHeight = fmBold.height();
    
    // Rectangle pour le highlight (aligné avec les labels Y)
    QRect highlightRect(m_plotRect.right() + 6, finalWidgetPos.y() - textHeight / 2 - 3,
                       textWidth + 10, textHeight + 6);
    
    // Fond rouge/orange pour le highlight
    painter.setPen(QPen(Qt::transparent));
    painter.setBrush(QColor(255, 220, 200));
    painter.drawRoundedRect(highlightRect, 1, 1);
    
    painter.setPen(Qt::black);
    painter.drawText(highlightRect, Qt::AlignCenter, finalText);
    
    // Restaurer la police normale
    painter.setFont(font());
    painter.setPen(Qt::black);

    // Axe X - Labels de dates intelligents
    if (!m_dates.empty()) {
        auto dateLabels = generateDateLabels();
        
        for (const auto& label : dateLabels) {
            double xPos = label.position;
            if (xPos >= 0 && xPos < m_dates.size()) {
                QPointF wp = mapToWidget(QPointF(xPos, m_ymin));
                
                // Tick en bas de l'axe X
                painter.drawLine(QPointF(wp.x(), m_plotRect.bottom()), 
                                QPointF(wp.x(), m_plotRect.bottom() + 4));
                
                // Adapter le style selon l'importance
                QFont labelFont = painter.font();
                if (label.importance == 3) {
                    // Année: gras
                    labelFont.setBold(true);
                } else if (label.importance == 2) {
                    // Mois: normal
                    labelFont.setBold(false);
                } else {
                    // Jour: plus petit
                    labelFont.setBold(false);
                    labelFont.setPointSize(qMax(6, labelFont.pointSize() - 1));
                }
                painter.setFont(labelFont);

                // Dessiner le texte centré sous le tick
                int tw = fm.horizontalAdvance(label.text);
                painter.drawText(QPointF(wp.x() - tw / 2, m_plotRect.bottom() + 18), label.text);
                
                // Restaurer la police
                painter.setFont(font());
            }
        }
    }
}

void EquityWidget::drawFilledAreas(QPainter &painter)
{
    const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
    if (activePoints.isEmpty()) return;
    
    double initialEquity = getInitialEquity();
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRect(m_plotRect);
    painter.setPen(Qt::NoPen);
    
    // On va créer des segments séparés pour chaque zone continue
    // Cela évite les artefacts visuels entre segments discontinus
    
    QVector<QPointF> currentSegment;
    bool isGainSegment = false;
    
    auto finishSegment = [&]() {
        if (currentSegment.size() < 2) return;
        
        QPainterPath path;
        
        // Commencer à la baseline du premier point
        QPointF firstBaseline = mapToWidget(QPointF(currentSegment.first().x(), initialEquity));
        path.moveTo(firstBaseline);
        
        // Suivre la courbe
        for (const auto& pt : currentSegment) {
            QPointF widgetPt = mapToWidget(pt);
            path.lineTo(widgetPt);
        }
        
        // Revenir à la baseline du dernier point
        QPointF lastBaseline = mapToWidget(QPointF(currentSegment.last().x(), initialEquity));
        path.lineTo(lastBaseline);
        
        // Fermer le chemin (retour au point de départ)
        path.closeSubpath();
        
        // Dessiner avec la couleur appropriée
        if (isGainSegment) {
            painter.setBrush(QColor(16, 124, 16, 40));  // Vert léger
        } else {
            painter.setBrush(QColor(196, 43, 28, 40));  // Rouge léger
        }
        painter.drawPath(path);
        
        currentSegment.clear();
    };
    
    for (int i = 0; i < activePoints.size(); ++i) {
        const QPointF& pt = activePoints[i];
        bool isGain = (pt.y() >= initialEquity);
        
        if (i == 0) {
            // Premier point
            currentSegment.append(pt);
            isGainSegment = isGain;
        } else {
            const QPointF& prevPt = activePoints[i - 1];
            bool prevIsGain = (prevPt.y() >= initialEquity);
            
            if (isGain == prevIsGain) {
                // On reste du même côté, continuer le segment
                currentSegment.append(pt);
            } else {
                // On change de côté (crossing)
                // Calculer le point d'intersection
                double t = (initialEquity - prevPt.y()) / (pt.y() - prevPt.y());
                double intersectX = prevPt.x() + t * (pt.x() - prevPt.x());
                QPointF intersectPt(intersectX, initialEquity);
                
                // Finir le segment précédent avec le point d'intersection
                currentSegment.append(intersectPt);
                finishSegment();
                
                // Commencer un nouveau segment avec le point d'intersection
                currentSegment.append(intersectPt);
                currentSegment.append(pt);
                isGainSegment = isGain;
            }
        }
    }
    
    // Finir le dernier segment
    finishSegment();
    
    painter.setClipping(false);
}

void EquityWidget::mouseMoveEvent(QMouseEvent *event)
{    
    // Position de la souris dans les coordonnées du widget
    m_mousePos = event->pos();
    
    // Vérifier si la souris est dans la zone de tracé
    if (m_plotRect.contains(m_mousePos)) {
        m_showCrosshair = true;
    } else {
        m_showCrosshair = false;
    }
    
    update();
}

void EquityWidget::drawEquityMarkers(QPainter &painter)
{
    // Récupérer les valeurs automatiquement
    double initialEquity = getInitialEquity();
    double peakEquity = getPeakEquity();
    double finalEquity = getFinalEquity();
    
    bool isPercentMode = m_checkBox->isChecked();
    
    // Helper pour formater les valeurs
    auto formatValue = [isPercentMode](double value) -> QString {
        if (isPercentMode) {
            return QString("%1%").arg(value, 0, 'f', 2);
        } else {
            return QLocale().toString(value, 'f', 0);
        }
    };
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont labelFont = painter.font();
    labelFont.setPointSize(qMax(8, labelFont.pointSize() - 1));
    labelFont.setWeight(QFont::DemiBold);
    QFontMetrics fm(labelFont);
    
    // ==================== 1. Ligne horizontale INITIAL (sans label) ====================
    {
        QPointF leftPt = mapToWidget(QPointF(m_xmin, initialEquity));
        QPointF rightPt = mapToWidget(QPointF(m_xmax, initialEquity));
        
        // Ligne en pointillés
        QPen initialPen(QColor(100, 100, 100), 1, Qt::DashLine);
        painter.setPen(initialPen);
        painter.drawLine(leftPt, rightPt);
        
        // Note: Le label "Initial" sera affiché sur l'axe Y (voir drawAxes)
    }
    
    // ==================== 2. Ligne horizontale PEAK ====================
    {
        QPointF leftPt = mapToWidget(QPointF(m_xmin, peakEquity));
        QPointF rightPt = mapToWidget(QPointF(m_xmax, peakEquity));
        
        // Ligne en pointillés (verte)
        QPen peakPen(QColor(16, 124, 16), 1, Qt::DashLine);
        painter.setPen(peakPen);
        painter.drawLine(leftPt, rightPt);
        
        // Label sur la ligne (au milieu)
        QString labelText = "Peak: " + formatValue(peakEquity);
        painter.setFont(labelFont);
        
        int textWidth = fm.horizontalAdvance(labelText);
        int textHeight = fm.height();
        
        QRect textRect(m_plotRect.center().x() - textWidth / 2 - 4, 
                      leftPt.y() - textHeight / 2 - 2, 
                      textWidth + 8, textHeight + 4);
        
        // Fond semi-transparent
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255));
        painter.drawRoundedRect(textRect, 3, 3);
        
        // Texte en vert
        painter.setPen(QColor(16, 124, 16));
        painter.drawText(textRect, Qt::AlignCenter, labelText);
    }
    
    // ==================== 3. Highlight FINAL sur la légende ====================
    // Note: Le highlight sera dessiné dans drawAxes() directement sur le dernier label Y
    // On va juste stocker la valeur finale pour que drawAxes() puisse la highlighter
    // Pour l'instant, on dessine un petit indicateur visuel sur le graphique
    {
        // QPointF finalPt = mapToWidget(QPointF(m_xmax, finalEquity));
        
        // Petit cercle sur le point final
        // QPen finalPen(QColor(196, 43, 28), 2);
        // // painter.setPen(finalPen);
        // painter.setBrush(QColor(196, 43, 28, 100));
        // painter.drawEllipse(finalPt, 3, 3);
        
        // Flèche vers la légende à droite
        // QPen arrowPen(QColor(196, 43, 28), 2);
        // arrowPen.setStyle(Qt::DotLine);
        // painter.setPen(arrowPen);
        
        // QPointF arrowEnd(m_plotRect.right() + 3, finalPt.y());
        // painter.drawLine(finalPt, arrowEnd);
        
        // // Petite tête de flèche
        // painter.setPen(QPen(QColor(196, 43, 28), 2));
        // painter.drawLine(arrowEnd, arrowEnd + QPointF(-4, -3));
        // painter.drawLine(arrowEnd, arrowEnd + QPointF(-4, 3));
    }
    
    // Restaurer la police
    painter.setFont(font());
}

void EquityWidget::leaveEvent(QEvent * /*event*/)
{
    m_showCrosshair = false;
    update();
}
