#pragma once

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QPair>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QString>

class GaugeRenderWidget;
class FillGaugeRenderWidget;

enum class FillDirection {
    LeftToRight,
    RightToLeft
};

/**
 * @brief Widget affichant une jauge pour visualiser un ratio financier
 * 
 * Ce widget affiche une jauge colorée horizontale permettant de contextualiser
 * visuellement la valeur d'un ratio financier (Sharpe, Sortino, Calmar, etc.)
 * avec des zones de couleurs différentes selon la qualité du ratio.
 */
class RatioGaugeWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Structure définissant une zone de la jauge
     */
    struct GaugeZone {
        double minValue;     ///< Valeur minimale de la zone
        double maxValue;     ///< Valeur maximale de la zone
        QColor color;        ///< Couleur de la zone
        QColor labelColor;   ///< Couleur du texte dans cette zone
        QString description; ///< Description textuelle de la zone
    };

    /**
     * @brief Constructeur
     * @param parent Widget parent
     */
    explicit RatioGaugeWidget(const QVector<GaugeZone>& zones, const QString& title, const QString& explanation, QWidget* parent = nullptr);

    /**
     * @brief Définir la valeur actuelle du ratio
     * @param value Valeur à afficher
     */
    void setValue(double value);

    /**
     * @brief Obtenir la valeur actuelle du ratio
     * @return Valeur actuelle
     */
    double value() const;

    /**
     * @brief Définir le format d'affichage de la valeur
     * @param precision Précision pour l'affichage (nombre de décimales)
     * @param addPercentageSign Indique si le signe % doit être ajouté
     */
    void setValueFormat(unsigned int precision, bool addPercentageSign);

    // Nouveau: Définir la valeur de référence
    void setReferenceValue(double referenceValue);

    // Nouveau: Activer/désactiver la barre de référence
    void showReference(bool show);

    // Nouveau: Définir la direction de remplissage
    void setFillDirection(FillDirection direction);

    /**
     * @brief Réinitialise le widget et cache le curseur
     */
    void clear();

protected:
    /**
     * @brief Événement de redimensionnement
     * @param event Informations sur le redimensionnement
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Événement de peinture personnalisé
     * @param event Informations sur l'événement de peinture
     */
    void paintEvent(QPaintEvent* event) override;

private:
    /**
     * @brief Initialiser l'interface utilisateur
     */
    void setupUI();

    /**
     * @brief Mettre à jour l'affichage de la jauge
     */
    void updateGauge();

    /**
     * @brief Mettre à jour le texte explicatif selon la valeur actuelle
     */
    void updateExplanation();

private:
    // Données de la jauge
    QVector<GaugeZone> m_zones;      ///< Zones de la jauge
    bool m_hasValue;
    double m_value;                  ///< Valeur actuelle
    double m_minValue;               ///< Valeur minimale affichable
    double m_maxValue;               ///< Valeur maximale affichable
    unsigned int m_precision;        ///< Précision pour l'affichage des valeurs
    bool m_addPercentageSign;        ///< Indique si le signe % doit être ajouté à la valeur
    double m_referenceValue;   // NOUVEAU: Valeur de référence
    bool m_showReference;      // NOUVEAU: Afficher la ligne de référence?
    FillDirection m_fillDirection; // NOUVEAU: Direction de remplissage

    // Widgets UI
    QGridLayout* m_mainLayout;  ///< Layout principal
    QLabel* m_metricNameLabel;       ///< Étiquette pour le nom de la métrique
    QString m_metricTitle;           ///< Titre de la métrique
    FillGaugeRenderWidget* m_gaugeWidget;          ///< Widget de la jauge
    QLabel* m_valueLabel;            ///< Étiquette pour la valeur
    QString m_baseExplanation;       ///< Texte explicatif de base
    int m_gaugeHeight;               ///< Hauteur de la jauge
};


// Classe dérivée pour le widget de jauge personnalisé
class GaugeRenderWidget : public QWidget {
    Q_OBJECT

public:
    GaugeRenderWidget(QWidget* parent = nullptr) : QWidget(parent), 
        m_value(0.0), m_minValue(0.0), m_maxValue(1.0), m_showCursor(false) {}

    void setZones(const QVector<RatioGaugeWidget::GaugeZone>& zones) {
        m_zones = zones;
        update();
    }
    
    void setValue(double value) {
        m_value = value;
        m_showCursor = true;
        update();
    }
    
    void setRange(double min, double max) {
        m_minValue = min;
        m_maxValue = max;
        update();
    }

    void setShowCursor(bool show) {
        m_showCursor = show;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        int width = this->width();
        int height = this->height();
        
        // Dessiner le fond de la jauge avec des dégradés pour chaque zone
        QRect gaugeRect(0, 0, width, height);
        
        if (m_zones.isEmpty()) return;
        
        // Calculer la largeur totale de la plage
        double totalRange = m_maxValue - m_minValue;

        int startX = qRound(qBound(0.0, (m_zones[0].minValue - m_minValue) / totalRange, 1.0) * width);
        
        // Dessiner chaque zone
        for (const auto& zone : m_zones) {
            double endPos = (zone.maxValue - m_minValue) / totalRange;
            endPos = qBound(0.0, endPos, 1.0);
            int endX = qRound(endPos * width);
            
            // Dessiner le rectangle de la zone
            QRect zoneRect(startX, 0, endX - startX, height);
            QColor lighterColor = zone.color.lighter(130);
            painter.fillRect(zoneRect, lighterColor);

            startX = endX; // Mettre à jour le début pour la prochaine zone
        }
        
        // Dessiner des graduations
        painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
        
        // Police pour les graduations - AMÉLIORÉE
        QFont tickFont = painter.font();
        tickFont.setPointSizeF(tickFont.pointSizeF() * 1.2); // Police 20% plus grande
        painter.setFont(tickFont);
        
        // Nombre de graduations
        const int numTicks = 10;
        int padding = 5; // Marge pour éviter que les graduations ne débordent
        
        for (int i = 0; i <= numTicks; i++) {
            // Calculer la position ajustée pour éviter le dépassement aux extrémités
            double ratio = (double)i / numTicks;
            int x = padding + (int)(ratio * (width - 2 * padding));
            
            // Lignes de graduation plus visibles
            // painter.setPen(QPen(QColor(40, 40, 40), 1.5, Qt::DotLine));
            // painter.drawLine(x, 0, x, height / 4);
            // painter.drawLine(x, height * 3 / 4, x, height);
            
            // Ajouter les valeurs aux graduations principales avec meilleure visibilité
            if (i % 2 != 0)
                continue;
                
            // Calculer la valeur correspondante
            double tickValue = m_minValue + (ratio * totalRange);
            QString valueStr = QString::number(tickValue, 'f', 1);
            QFontMetrics fm = painter.fontMetrics();
            int textWidth = fm.horizontalAdvance(valueStr);
            int textHeight = fm.height();
            
            // Position X adaptée selon la position (premier, dernier ou intermédiaire)
            int textX;
            if (i == 0) {
                // Premier label aligné à gauche avec un léger décalage
                textX = padding + 2;
            } else if (i == numTicks) {
                // Dernier label aligné à droite avec un léger décalage
                textX = width - padding - textWidth - 2;
            } else {
                // Labels intermédiaires centrés
                textX = x - textWidth / 2;
            }
            
            // Position Y centrée verticalement
            int textY = height / 2 + textHeight / 3;
            
            // Texte plus foncé et plus visible
            painter.setPen(QColor(0, 0, 0));
            painter.drawText(textX, textY, valueStr);
        }
        
        if (!m_showCursor || m_zones.isEmpty())
            return; // Ne pas dessiner le curseur si désactivé ou pas de zones
        
        // Calculer la position horizontale du curseur
        double valuePos = (m_value - m_minValue) / totalRange;
        valuePos = qBound(0.0, valuePos, 1.0);
        int markerX = padding + (int)(valuePos * (width - 2 * padding));
        
        // Paramètres du sablier
        int sablierWidth = qMin(height / 3, 12); // Largeur du sablier (max 12px)
        int middleHeight = height / 2;

        // Créer le chemin du sablier
        QPainterPath sablierPath;

        // Triangle supérieur (base en haut, pointe vers le bas)
        sablierPath.moveTo(markerX - sablierWidth, 0);  // Coin supérieur gauche
        sablierPath.lineTo(markerX + sablierWidth, 0);  // Coin supérieur droit
        sablierPath.lineTo(markerX, middleHeight);                    // Pointe au milieu
        sablierPath.closeSubpath();

        // Triangle inférieur (base en bas, pointe vers le haut)
        sablierPath.moveTo(markerX - sablierWidth, height);  // Coin inférieur gauche
        sablierPath.lineTo(markerX + sablierWidth, height);  // Coin inférieur droit
        sablierPath.lineTo(markerX, middleHeight);                            // Pointe au milieu
        sablierPath.closeSubpath();

        // Couleurs neutres qui se détachent bien
        QColor fillColor = QColor(55, 55, 55, 230);     // Gris foncé avec légère transparence
        QColor strokeColor = QColor(0, 0, 0);           // Noir pour le contour

        // Dessiner le sablier avec une bordure contrastée
        painter.setPen(QPen(strokeColor, 2));
        painter.setBrush(fillColor);
        painter.drawPath(sablierPath);

        // Ligne verticale centrale pour plus de précision (en blanc pour contraste)
        // painter.setPen(QPen(Qt::white, 1));
        // painter.drawLine(markerX, 0, markerX, height);

        // Point central pour l'indication précise de la valeur (en blanc)
        // painter.setBrush(Qt::white);
        // painter.setPen(Qt::NoPen);
        // painter.drawEllipse(markerX - 3, middleHeight - 3, 6, 6);

        // Contour du point central pour plus de netteté
        // painter.setPen(QPen(Qt::black, 1));
        // painter.setBrush(Qt::NoBrush);
        // painter.drawEllipse(markerX - 4, middleHeight - 4, 8, 8);

    }

private:
    QVector<RatioGaugeWidget::GaugeZone> m_zones;
    double m_value;
    double m_minValue;
    double m_maxValue;
    bool m_showCursor;
};

// FillGaugeRenderWidget.h - à ajouter dans le fichier d'en-tête RatioGaugeWidget.h ou dans son propre fichier

class FillGaugeRenderWidget : public QWidget {
    Q_OBJECT

public:
    FillGaugeRenderWidget(QWidget* parent = nullptr)
        : QWidget(parent), 
          m_value(0.0), 
          m_minValue(0.0), 
          m_maxValue(1.0),
          m_referenceValue(std::numeric_limits<double>::quiet_NaN()), // Valeur de référence (NaN = pas de référence)
          m_showReference(false),
          m_fillDirection(FillDirection::LeftToRight) // Par défaut: remplissage de gauche à droite
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(65); // Hauteur minimale pour accommoder les labels en dessous
    }

    void setZones(const QVector<RatioGaugeWidget::GaugeZone>& zones) {
        m_zones = zones;
        update();
    }
    
    void setValue(double value) {
        m_value = value;
        update();
    }
    
    void setRange(double min, double max) {
        m_minValue = min;
        m_maxValue = max;
        update();
    }

    // Nouveau: Définir la valeur de référence
    void setReferenceValue(double referenceValue) {
        m_referenceValue = referenceValue;
        m_showReference = !std::isnan(referenceValue);
        update();
    }

    // Nouveau: Activer/désactiver la barre de référence
    void showReference(bool show) {
        m_showReference = show;
        update();
    }

    // Nouveau: Définir la direction de remplissage
    void setFillDirection(FillDirection direction) {
        m_fillDirection = direction;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        
        // Définir les dimensions et positions principales
        int gaugeHeight = h * 0.55;         // 55% de la hauteur pour la gauge et les tirets
        int barHeight = gaugeHeight * 0.3;  // Barre de remplissage = 35% de la hauteur du fond
        int labelHeight = h - gaugeHeight;  // 45% pour les labels

        int gaugeY = 0;                     // Gauge commence en haut
        int barY = gaugeY + (gaugeHeight - barHeight) / 2; // Barre centrée dans la gauge
        int tickY = gaugeY + gaugeHeight;    // Position Y des tirets (sous la gauge)
        int labelY = tickY;              // Position Y des labels (sous les tirets)
        
        // Rectangle pour la gauge complète
        QRect gaugeRect(0, gaugeY, w, gaugeHeight);
        
        if (m_zones.isEmpty()) return;
        
        // 1. DESSINER LE FOND DE ZONES COLORÉES
        double totalRange = m_maxValue - m_minValue;
        int startX = 0;
        
        for (const auto& zone : m_zones) {
            double zoneStart = (zone.minValue - m_minValue) / totalRange;
            double zoneEnd = (zone.maxValue - m_minValue) / totalRange;
            zoneStart = qBound(0.0, zoneStart, 1.0);
            zoneEnd = qBound(0.0, zoneEnd, 1.0);
            
            int zoneStartX = qRound(zoneStart * w);
            int zoneEndX = qRound(zoneEnd * w);
            int zoneWidth = zoneEndX - zoneStartX;
            
            if (zoneWidth > 0) {
                QRect zoneRect(zoneStartX, gaugeY, zoneWidth, gaugeHeight);
                QColor zoneColor = zone.color.lighter(115); // Légèrement plus clair
                painter.fillRect(zoneRect, zoneColor);
                
                // Bordure fine entre les zones
                painter.setPen(QPen(QColor(220, 220, 220), 1));
                painter.drawLine(zoneEndX, gaugeY, zoneEndX, gaugeY + gaugeHeight);
            }
            
            startX = zoneEndX;
        }
        
        // 2. DESSINER LA BARRE DE REMPLISSAGE NOIRE
        double fillRatio = (m_value - m_minValue) / totalRange;
        
        QRect fillRect;
        
        // Dessiner la barre selon la direction choisie
        if (m_fillDirection == FillDirection::LeftToRight) {
            fillRatio = qBound(0.0, fillRatio, 1.0);
            int fillWidth = qRound(fillRatio * w);
            fillRect = QRect(0, barY, fillWidth, barHeight);
        } else {
            fillRatio = qBound(0.0, 1 - fillRatio, 1.0);
            int fillWidth = qRound(fillRatio * w);
            fillRect = QRect(w - fillWidth, barY, fillWidth, barHeight);
        }
        
        QColor fillColor(30, 30, 30);  // Noir semi-transparent
        painter.fillRect(fillRect, fillColor);
        
        // 3. DESSINER LA BARRE DE RÉFÉRENCE (NOUVEAU)
        if (m_showReference && m_referenceValue >= m_minValue && m_referenceValue <= m_maxValue) {
            double refRatio = (m_referenceValue - m_minValue) / totalRange;
            refRatio = qBound(0.0, refRatio, 1.0);
            int refX = qRound(refRatio * w);
            
            // Dessiner une ligne verticale pour la référence
            painter.setPen(QPen(QColor(0, 0, 0), 2)); // Ligne noire de 2px
            painter.fillRect(QRect(refX - 1, gaugeY, 3, gaugeHeight), QColor(0, 0, 0));
        }
        
        // 4. DESSINER LES GRADUATIONS ET LABELS
        painter.setPen(QPen(QColor(20, 20, 20), 1));
        
        QFont tickFont = painter.font();
        tickFont.setPointSizeF(tickFont.pointSizeF());
        painter.setFont(tickFont);
        
        // Nombre de graduations principales
        const int numTicks = 5; // Réduit pour plus de lisibilité
        int tickLength = 5;
        
        for (int i = 0; i <= numTicks; i++) {
            double ratio = (double)i / numTicks;
            int x = qRound(ratio * w);
            
            // Tiret de graduation
            painter.drawLine(x, tickY, x, tickY + tickLength);
            
            // Label de valeur
            double tickValue = m_minValue + (ratio * totalRange);
            QString valueStr = QString::number(tickValue, 'f', 1);
            QFontMetrics fm = painter.fontMetrics();
            int textWidth = fm.horizontalAdvance(valueStr);
            
            // Ajuster position X pour éviter débordement
            int textX;
            if (i == 0) {
                textX = x;
            } else if (i == numTicks) {
                textX = x - textWidth;
            } else {
                textX = x - textWidth/2;
            }
            
            painter.drawText(textX, labelY + fm.height(), valueStr);
        }
        
        // 5. AJOUTER DES GRADUATIONS INTERMÉDIAIRES (optionnel)
        painter.setPen(QPen(QColor(120, 120, 120), 0.5, Qt::DotLine));
        for (int i = 1; i < numTicks * 2; i += 2) {
            double ratio = (double)i / (numTicks * 2);
            int x = qRound(ratio * w);
            
            // Tiret intermédiaire plus court
            painter.drawLine(x, tickY - tickLength/2, x, tickY);
        }
        
        // 6. BORDURE DE LA JAUGE COMPLÈTE
        painter.setPen(QPen(QColor(100, 100, 100), 1));
        painter.drawRect(0, gaugeY, w, gaugeHeight);
    }

private:
    QVector<RatioGaugeWidget::GaugeZone> m_zones;
    double m_value;
    double m_minValue;
    double m_maxValue;
    double m_referenceValue;   // NOUVEAU: Valeur de référence
    bool m_showReference;      // NOUVEAU: Afficher la ligne de référence?
    FillDirection m_fillDirection; // NOUVEAU: Direction de remplissage
};