#include "ui/views/Stats/RatioGaugeWidget.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QStyleOption>
#include <QLinearGradient>
#include <QPen>
#include <QBrush>
#include <cmath>
#include <QDebug>
#include <QResizeEvent>

RatioGaugeWidget::RatioGaugeWidget(RatioType type, QWidget* parent)
    : QWidget(parent),
      m_value(0.0),
      m_minValue(0.0),
      m_maxValue(1.0),
      m_precision(2),
      m_addPercentageSign(false),
      m_currentType(RatioType::Custom),
      m_gaugeHeight(20),
      m_hasValue(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(80);
    // setMinimumWidth(m_gaugeWidth);
    setupUI();
    
    m_currentType = type;
    
    switch (type) {
        case RatioType::Sharpe:
            setupSharpeRatio();
            break;
        case RatioType::Sortino:
            setupSortinoRatio();
            break;
        case RatioType::Calmar:
            setupCalmarRatio();
            break;
        case RatioType::WinRate:
            setupWinRate();
            break;
        case RatioType::ProfitFactor:
            setupProfitFactor();
            break;
        case RatioType::Kelly:
            setupKelly();
            break;
        case RatioType::MaxDrawdown:
            setupMaxDrawdown();
            break;
        case RatioType::SQN:
            setupSQN();
            break;
        case RatioType::Custom:
        default:
            // Rien à faire pour le type personnalisé
            break;
    }
    
    updateGauge();
}

void RatioGaugeWidget::setupUI()
{
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Section supérieure avec nom de la métrique, valeur et zone actuelle
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    // Étiquette pour le nom de la métrique
    m_metricNameLabel = new QLabel("Ratio");
    QFont nameFont = m_metricNameLabel->font();
    nameFont.setPointSize(14);
    m_metricNameLabel->setFont(nameFont);
    m_metricNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // Étiquette pour la valeur actuelle
    m_valueLabel = new QLabel("N/A");
    QFont valueFont = m_valueLabel->font();
    valueFont.setPointSize(14);
    m_valueLabel->setFont(valueFont);
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // Widget personnalisé pour la jauge
    m_gaugeWidget = new GaugeRenderWidget(this);
    m_gaugeWidget->setFixedHeight(m_gaugeHeight);
    m_gaugeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    topLayout->addWidget(m_metricNameLabel);
    topLayout->addWidget(m_valueLabel);
    // topLayout->addStretch();
    topLayout->addWidget(m_gaugeWidget);

    mainLayout->addLayout(topLayout);
    
    // Connecter les signaux pour le redessinage
    connect(this, &RatioGaugeWidget::objectNameChanged, this, [this]() {
        updateGauge();
    });
}

void RatioGaugeWidget::setZones(const QVector<GaugeZone>& zones, const QString& title, const QString& explanation)
{
    if (zones.isEmpty()) {
        qWarning() << "Tentative de définition de zones vides pour la jauge";
        return;
    }
    
    m_zones = zones;
    m_metricTitle = title;
    m_baseExplanation = explanation;
    
    // Mettre à jour le nom de la métrique affiché
    m_metricNameLabel->setText(title + ": ");
    
    // Déterminer les valeurs min/max
    m_minValue = m_zones.first().minValue;
    m_maxValue = m_zones.last().maxValue;
    
    // Mettre à jour l'affichage
    updateGauge();
    updateExplanation();
}

void RatioGaugeWidget::setValue(double value)
{
    m_value = value;
    m_hasValue = true;
    updateGauge();
    updateExplanation();
}

double RatioGaugeWidget::value() const
{
    return m_value;
}

void RatioGaugeWidget::setValueFormat(unsigned int precision, bool addPercentageSign)
{
    m_precision = precision;
    m_addPercentageSign = addPercentageSign;
    
    updateGauge();
}

void RatioGaugeWidget::setGaugeHeight(int height)
{
    m_gaugeHeight = height;
    m_gaugeWidget->setMinimumHeight(height);
    updateGauge();
}

void RatioGaugeWidget::clear()
{
    m_value = 0.0;
    m_hasValue = false;
    
    updateGauge();
    updateExplanation();
}

void RatioGaugeWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateGauge();
}

void RatioGaugeWidget::paintEvent(QPaintEvent* event)
{
    // Dessiner le fond du widget
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    
    QWidget::paintEvent(event);
}

void RatioGaugeWidget::updateExplanation()
{
    // Trouver la zone actuelle pour ajouter une explication spécifique
    QString zoneExplanation = "";
    
    for (const auto& zone : m_zones) {
        if (m_value >= zone.minValue && m_value <= zone.maxValue) {
            zoneExplanation = QString("<br><b>Interprétation :</b> %1").arg(zone.description);
            break;
        }
    }

    QString tooltipText = m_baseExplanation + zoneExplanation;

    // Appliquer le tooltip aux widgets principaux
    this->setToolTip(tooltipText);
    m_gaugeWidget->setToolTip(tooltipText);
    m_metricNameLabel->setToolTip(tooltipText);
    m_valueLabel->setToolTip(tooltipText);
}

// Configuration des différents types de ratios

void RatioGaugeWidget::setupSharpeRatio()
{
    QVector<GaugeZone> sharpeZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Mauvais"},                          // Rouge
        { 0.0, 0.5, QColor(240, 173, 78), "Faible"},                         // Orange
        { 0.5, 1.0, QColor(240, 240, 80), "Moyen"},                         // Jaune
        { 1.0, 1.5, QColor(150, 200, 80), "Bon"},                           // Jaune-vert
        { 1.5, 2.5, QColor(92, 184, 92), "Très bon"},                       // Vert
        { 2.5, 4.0, QColor(32, 150, 80), "Excellent"}                      // Vert foncé
    };
    
    QString explanation = 
        "Le <b>ratio de Sharpe</b> mesure le rendement ajusté au risque. "
        "Il indique combien d'unités de rendement excédentaire vous obtenez pour chaque unité de volatilité. "
        "Un ratio plus élevé indique un meilleur rendement ajusté au risque.";
    
    setZones(sharpeZones, "Ratio de Sharpe", explanation);
}

void RatioGaugeWidget::setupSortinoRatio()
{
    QVector<GaugeZone> sortinoZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Mauvais"},                          // Rouge
        { 0.0, 0.75, QColor(240, 173, 78), "Faible"},                        // Orange
        { 0.75, 1.5, QColor(240, 240, 80), "Moyen"},                        // Jaune
        { 1.5, 2.5, QColor(150, 200, 80), "Bon"},                          // Jaune-vert
        { 2.5, 3.5, QColor(92, 184, 92), "Très bon"},                      // Vert
        { 3.5, 4.0, QColor(32, 150, 80), "Excellent"}                     // Vert foncé
    };
    
    QString explanation = 
        "Le <b>ratio de Sortino</b> est similaire au ratio de Sharpe, mais ne pénalise que la volatilité à la baisse. "
        "Il mesure le rendement excédentaire par unité de risque de baisse, ce qui est souvent plus pertinent pour les traders. "
        "Un ratio plus élevé indique une meilleure gestion du risque de perte.";
    
    setZones(sortinoZones, "Ratio de Sortino", explanation);
}

void RatioGaugeWidget::setupCalmarRatio()
{
    QVector<GaugeZone> calmarZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Mauvais"},                          // Rouge
        { 0.0, 0.5, QColor(240, 173, 78), "Faible"},                         // Orange
        { 0.5, 1.0, QColor(240, 240, 80), "Moyen"},                         // Jaune
        { 1.0, 2.0, QColor(150, 200, 80), "Bon"},                           // Jaune-vert
        { 2.0, 3.0, QColor(92, 184, 92), "Très bon"},                       // Vert
        { 3.0, 4.0, QColor(32, 150, 80), "Excellent"}                     // Vert foncé
    };
    
    QString explanation = 
        "Le <b>ratio de Calmar</b> mesure le rendement annualisé par rapport au drawdown maximal. "
        "Il indique le rendement obtenu par unité de risque de drawdown. "
        "Un ratio de Calmar supérieur à 1 signifie que le rendement annualisé est supérieur au drawdown maximal.";
    
    setZones(calmarZones, "Ratio de Calmar", explanation);
}

void RatioGaugeWidget::setupWinRate()
{
    QVector<GaugeZone> winRateZones = {
        {0.0, 0.3, QColor(217, 83, 79), "Très faible"},                       // Rouge
        {0.3, 0.4, QColor(240, 173, 78), "Faible"},                          // Orange
        {0.4, 0.5, QColor(240, 240, 80), "Moyen"},                          // Jaune
        {0.5, 0.6, QColor(150, 200, 80), "Bon"},                            // Jaune-vert
        {0.6, 0.7, QColor(92, 184, 92), "Très bon"},                        // Vert
        {0.7, 1.0, QColor(32, 150, 80), "Excellent"}                       // Vert foncé
    };
    
    QString explanation = 
        "Le <b>taux de réussite</b> (Win Rate) représente le pourcentage de trades gagnants. "
        "Bien qu'important, il doit être évalué en conjonction avec le ratio de profit/perte, "
        "car une stratégie avec un faible taux de réussite peut être profitable si les gains sont importants par rapport aux pertes.";
    
    setZones(winRateZones, "Taux de Réussite", explanation);
}

void RatioGaugeWidget::setupProfitFactor()
{
    QVector<GaugeZone> profitFactorZones = {
        {0.0, 1.0, QColor(217, 83, 79), "Non rentable"},                     // Rouge
        {1.0, 1.25, QColor(240, 173, 78), "Rentabilité marginale"},          // Orange
        {1.25, 1.5, QColor(240, 240, 80), "Rentabilité acceptable"},        // Jaune
        {1.5, 2.0, QColor(150, 200, 80), "Bonne rentabilité"},              // Jaune-vert
        {2.0, 3.0, QColor(92, 184, 92), "Très bonne rentabilité"},         // Vert
        {3.0, 4.0, QColor(32, 150, 80), "Excellente rentabilité"}         // Vert foncé
    };
    
    QString explanation = 
        "Le <b>facteur de profit</b> est le ratio entre les profits bruts et les pertes brutes. "
        "Un facteur de profit supérieur à 1 indique une stratégie rentable. "
        "Plus ce ratio est élevé, plus la stratégie est robuste face aux fluctuations du marché.";
    
    setZones(profitFactorZones, "Facteur de Profit", explanation);
}

void RatioGaugeWidget::setupKelly()
{
    QVector<GaugeZone> kellyZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Non viable"},                       // Rouge
        { 0.0, 0.05, QColor(240, 173, 78), "Taille minimale"},                // Orange
        { 0.05, 0.15, QColor(240, 240, 80), "Taille conservative"},            // Jaune
        { 0.15, 0.25, QColor(150, 200, 80), "Taille optimale"},                // Jaune-vert
        { 0.25, 0.4, QColor(92, 184, 92), "Taille agressive"},               // Vert
        { 0.4, 1.0, QColor(32, 150, 80), "Très agressive"}                  // Vert foncé
    };
    
    QString explanation = 
        "Le <b>critère de Kelly</b> détermine la taille optimale des positions pour maximiser la croissance du capital à long terme. "
        "En pratique, de nombreux traders utilisent une fraction de Kelly (25-50%) pour réduire la volatilité. "
        "Un critère négatif indique qu'on ne devrait pas trader cette stratégie.";
    
    setZones(kellyZones, "Critère de Kelly", explanation);
}

void RatioGaugeWidget::setupMaxDrawdown()
{
    QVector<GaugeZone> drawdownZones = {
        {100.0, 50.0, QColor(217, 83, 79), "Critique"},                      // Rouge
        {50.0, 30.0, QColor(240, 173, 78), "Sévère"},                       // Orange
        {30.0, 20.0, QColor(240, 240, 80), "Important"},                    // Jaune
        {20.0, 10.0, QColor(150, 200, 80), "Modéré"},                       // Jaune-vert
        {10.0, 5.0, QColor(92, 184, 92), "Faible"},                        // Vert
        {5.0, 0.0, QColor(32, 150, 80), "Très faible"}                   // Vert foncé
    };
    
    QString explanation = 
        "Le <b>drawdown maximal</b> mesure la perte maximale subie entre un pic et un creux de l'équité. "
        "C'est un indicateur clé de risque qui montre la pire perte qu'un trader aurait pu subir. "
        "Un drawdown plus faible est préférable et indique une meilleure gestion du risque.";
    
    setZones(drawdownZones, "Drawdown Maximal", explanation);
    setValueFormat(1, true); // Format pourcentage avec signe %
}

void RatioGaugeWidget::setupSQN()
{
    QVector<GaugeZone> sqnZones = {
        {-10.0, 1.6, QColor(217, 83, 79), "Médiocre"},                        // Rouge
        { 1.6, 2.0, QColor(240, 173, 78), "Moyen"},                          // Orange
        { 2.0, 2.5, QColor(240, 240, 80), "Bon"},                           // Jaune
        { 2.5, 3.0, QColor(150, 200, 80), "Très bon"},                      // Jaune-vert
        { 3.0, 5.0, QColor(92, 184, 92), "Excellent"},                      // Vert
        { 5.0, 10.0, QColor(32, 150, 80), "Extraordinaire"}                 // Vert foncé
    };
    
    QString explanation = 
        "Le <b>System Quality Number (SQN)</b> mesure la qualité globale d'un système de trading. "
        "Il prend en compte le rendement moyen par trade, l'écart-type des rendements et le nombre de trades. "
        "Un SQN plus élevé indique un système plus robuste et plus fiable.";
    
    setZones(sqnZones, "System Quality Number", explanation);
}

// Surcharge de la fonction de mise à jour de la jauge pour utiliser le widget spécialisé
void RatioGaugeWidget::updateGauge()
{
    // Vérifier si le widget de rendu existe et créer une instance si nécessaire
    GaugeRenderWidget* gaugeRenderer = dynamic_cast<GaugeRenderWidget*>(m_gaugeWidget);
    if (!gaugeRenderer) {
        // Supprimer l'ancien widget
        delete m_gaugeWidget;
        
        // Créer le nouveau widget de rendu
        gaugeRenderer = new GaugeRenderWidget(this);
        gaugeRenderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        
        // Remplacer dans le layout
        // QLayoutItem* item = m_groupBox->layout()->takeAt(1);
        // delete item;
        // static_cast<QVBoxLayout*>(m_groupBox->layout())->insertWidget(1, gaugeRenderer);
        
        // Mettre à jour la référence
        m_gaugeWidget = gaugeRenderer;
    }
    
    // Configurer le widget de rendu
    gaugeRenderer->setZones(m_zones);
    gaugeRenderer->setRange(m_minValue, m_maxValue);

    // Ne pas afficher le curseur s'il n'y a pas de valeur définie
    if (m_hasValue) {
        gaugeRenderer->setValue(m_value);
        // Mettre à jour uniquement la valeur, pas le nom de la métrique
        m_valueLabel->setText(QString::number(m_value, 'f', m_precision) + (m_addPercentageSign ? "%" : ""));
        
        // Trouver la zone actuelle
        QString currentZoneDesc = "Unknown";
        QColor currentColor = QColor(150, 150, 150); // Gris par défaut
        bool inRange = false;
        
        for (const auto& zone : m_zones) {
            if (m_value >= zone.minValue && m_value <= zone.maxValue ||
                (m_value >= zone.maxValue && m_value <= zone.minValue)) {
                currentZoneDesc = zone.description;
                currentColor = zone.color;
                inRange = true;
                break;
            }
        }

        if (!inRange && m_value < m_minValue) {
            currentZoneDesc = m_zones.first().description;
            currentColor = m_zones.first().color;
        } else if (!inRange && m_value > m_maxValue) {
            currentZoneDesc = m_zones.last().description;
            currentColor = m_zones.last().color;
        }

        // Mettre à jour l'étiquette de zone
        // m_currentZoneLabel->setText(currentZoneDesc);

        // // Calculer une couleur de fond éclaircie (luminance augmentée)
        // QColor lighterColor = currentColor.lighter(135); // 140 = +40% plus clair
        // const QString textColor = "#000000";

        // // Appliquer fond (la couleur de la zone éclaircie) et texte contrasté, avec un padding pour lisibilité
        // m_currentZoneLabel->setStyleSheet(QString(
        //     "color: %1; background-color: %2; padding: 4px 8px; border-radius: 4px;")
        //     .arg(textColor, lighterColor.name()));
        // m_currentZoneLabel->setVisible(true);
    } else {
        // Réinitialiser l'affichage
        // gaugeRenderer->setShowCursor(false);
        // m_valueLabel->setText("N/A");
        // m_currentZoneLabel->setVisible(false);
    }

    updateExplanation();
}