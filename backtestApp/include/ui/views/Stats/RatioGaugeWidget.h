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
        QString description; ///< Description textuelle de la zone
    };

    /**
     * @brief Types de ratios financiers prédéfinis
     */
    enum class RatioType {
        Sharpe,     ///< Ratio de Sharpe
        Sortino,    ///< Ratio de Sortino
        Calmar,     ///< Ratio de Calmar
        WinRate,    ///< Pourcentage de trades gagnants
        ProfitFactor,///< Facteur de profit
        Kelly,      ///< Critère de Kelly
        MaxDrawdown,///< Drawdown maximal
        SQN,        ///< System Quality Number
        Custom      ///< Ratio personnalisé
    };

    /**
     * @brief Constructeur
     * @param parent Widget parent
     */
    explicit RatioGaugeWidget(QWidget* parent = nullptr);

    /**
     * @brief Définir les zones de la jauge manuellement
     * @param zones Liste des zones à afficher
     * @param title Titre du ratio
     * @param explanation Explication générale du ratio
     */
    void setZones(const QVector<GaugeZone>& zones, const QString& title, const QString& explanation);

    /**
     * @brief Configurer la jauge pour un type de ratio prédéfini
     * @param type Type de ratio à afficher
     */
    void setRatioType(RatioType type);

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


    /**
     * @brief Définir la largeur de la jauge
     * @param width Largeur souhaitée en pixels
     */
    void setGaugeWidth(int width);

    /**
     * @brief Définir la hauteur de la jauge
     * @param height Hauteur souhaitée en pixels
     */
    void setGaugeHeight(int height);

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

    /**
     * @brief Configurer les zones pour le ratio de Sharpe
     */
    void setupSharpeRatio();

    /**
     * @brief Configurer les zones pour le ratio de Sortino
     */
    void setupSortinoRatio();

    /**
     * @brief Configurer les zones pour le ratio de Calmar
     */
    void setupCalmarRatio();

    /**
     * @brief Configurer les zones pour le taux de réussite (Win Rate)
     */
    void setupWinRate();

    /**
     * @brief Configurer les zones pour le facteur de profit
     */
    void setupProfitFactor();

    /**
     * @brief Configurer les zones pour le critère de Kelly
     */
    void setupKelly();

    /**
     * @brief Configurer les zones pour le drawdown maximal
     */
    void setupMaxDrawdown();

    /**
     * @brief Configurer les zones pour le SQN (System Quality Number)
     */
    void setupSQN();

private:
    // Données de la jauge
    QVector<GaugeZone> m_zones;      ///< Zones de la jauge
    double m_value;                  ///< Valeur actuelle
    double m_minValue;               ///< Valeur minimale affichable
    double m_maxValue;               ///< Valeur maximale affichable
    unsigned int m_precision;        ///< Précision pour l'affichage des valeurs
    bool m_addPercentageSign;        ///< Indique si le signe % doit être ajouté à la valeur
    RatioType m_currentType;         ///< Type de ratio actuel

    // Widgets UI
    QGroupBox* m_groupBox;           ///< Boîte de groupe principale
    QWidget* m_gaugeWidget;          ///< Widget de la jauge
    QLabel* m_valueLabel;            ///< Étiquette pour la valeur
    QLabel* m_currentZoneLabel;      ///< Description de la zone actuelle
    QString m_baseExplanation;       ///< Texte explicatif de base
    int m_gaugeWidth;                ///< Largeur de la jauge
    int m_gaugeHeight;               ///< Hauteur de la jauge
};


// Classe dérivée pour le widget de jauge personnalisé
class GaugeRenderWidget : public QWidget {
    Q_OBJECT

public:
    GaugeRenderWidget(QWidget* parent = nullptr) : QWidget(parent), 
        m_value(0.0), m_minValue(0.0), m_maxValue(1.0) {}
    
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
            painter.fillRect(zoneRect, zone.color);

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
        
        // Dessiner la valeur actuelle (indicateur sobre)
        double valuePos = (m_value - m_minValue) / totalRange;
        valuePos = qBound(0.0, valuePos, 1.0); // Limiter aux bornes
        
        int markerX = padding + (int)(valuePos * (width - 2 * padding));
        
        // Ligne verticale du curseur (plus sobre)
        QPen cursorPen(QColor(0, 0, 0), 3);
        painter.setPen(cursorPen);
        painter.drawLine(markerX, 2, markerX, height - 2);
        
        // Indicateur circulaire simple et contrasté
        painter.setPen(QPen(Qt::black, 1.5));
        painter.setBrush(QColor(255, 50, 50)); // Rouge vif pour une bonne visibilité
        painter.drawEllipse(markerX - 5, height / 2 - 5, 10, 10);
        
        // Bordure globale de la jauge
        // painter.setPen(QPen(QColor(80, 80, 80), 1.5));
        // painter.setBrush(Qt::NoBrush);
        // painter.drawRect(0, 0, width - 1, height - 1);
    }

private:
    QVector<RatioGaugeWidget::GaugeZone> m_zones;
    double m_value;
    double m_minValue;
    double m_maxValue;
};
