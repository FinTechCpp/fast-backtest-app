#include "ui/views/Stats/RatioGaugesContainerWidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

RatioGaugesContainerWidget::RatioGaugesContainerWidget(QWidget* parent)
    : StatsBaseWidget(parent)
{
    // Layout principal
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Widget de contenu
    m_contentWidget = new QWidget();
    m_mainLayout->addWidget(m_contentWidget);
    
    // Layout des jauges en grille (2 colonnes)
    QGridLayout* gridLayout = new QGridLayout(m_contentWidget);
    gridLayout->setSpacing(5);

    QVector<RatioGaugeWidget::GaugeZone> sharpeZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Mauvais"},                          // Rouge
        { 0.0, 0.5, QColor(240, 173, 78), "Faible"},                         // Orange
        { 0.5, 1.0, QColor(240, 240, 80), "Moyen"},                         // Jaune
        { 1.0, 1.5, QColor(150, 200, 80), "Bon"},                           // Jaune-vert
        { 1.5, 2.5, QColor(92, 184, 92), "Très bon"},                       // Vert
        { 2.5, 4.0, QColor(32, 150, 80), "Excellent"}                      // Vert foncé
    };
    QString sharpeExplanation =
        "Le <b>ratio de Sharpe</b> mesure le rendement ajusté au risque. "
        "Il indique combien d'unités de rendement excédentaire vous obtenez pour chaque unité de volatilité. "
        "Un ratio plus élevé indique un meilleur rendement ajusté au risque.";
    m_sharpeGauge = new RatioGaugeWidget(sharpeZones, "Ratio de Sharpe", sharpeExplanation);


    QVector<RatioGaugeWidget::GaugeZone> sortinoZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Mauvais"},                          // Rouge
        { 0.0, 0.75, QColor(240, 173, 78), "Faible"},                        // Orange
        { 0.75, 1.5, QColor(240, 240, 80), "Moyen"},                        // Jaune
        { 1.5, 2.5, QColor(150, 200, 80), "Bon"},                          // Jaune-vert
        { 2.5, 3.5, QColor(92, 184, 92), "Très bon"},                      // Vert
        { 3.5, 4.0, QColor(32, 150, 80), "Excellent"}                     // Vert foncé
    };
    QString sortinoExplanation = 
        "Le <b>ratio de Sortino</b> est similaire au ratio de Sharpe, mais ne pénalise que la volatilité à la baisse. "
        "Il mesure le rendement excédentaire par unité de risque de baisse, ce qui est souvent plus pertinent pour les traders. "
        "Un ratio plus élevé indique une meilleure gestion du risque de perte.";
    m_sortinoGauge = new RatioGaugeWidget(sortinoZones, "Ratio de Sortino", sortinoExplanation);


    QVector<RatioGaugeWidget::GaugeZone> calmarZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Mauvais"},                          // Rouge
        { 0.0, 0.5, QColor(240, 173, 78), "Faible"},                         // Orange
        { 0.5, 1.0, QColor(240, 240, 80), "Moyen"},                         // Jaune
        { 1.0, 2.0, QColor(150, 200, 80), "Bon"},                           // Jaune-vert
        { 2.0, 3.0, QColor(92, 184, 92), "Très bon"},                       // Vert
        { 3.0, 4.0, QColor(32, 150, 80), "Excellent"}                     // Vert foncé
    };
    QString calmarExplanation =
        "Le <b>ratio de Calmar</b> mesure le rendement annualisé par rapport au drawdown maximal. "
        "Il indique le rendement obtenu par unité de risque de drawdown. "
        "Un ratio de Calmar supérieur à 1 signifie que le rendement annualisé est supérieur au drawdown maximal.";
    m_calmarGauge = new RatioGaugeWidget(calmarZones, "Ratio de Calmar", calmarExplanation);


    QVector<RatioGaugeWidget::GaugeZone> winRateZones = {
        {0.0, 0.3, QColor(217, 83, 79), "Très faible"},                       // Rouge
        {0.3, 0.4, QColor(240, 173, 78), "Faible"},                          // Orange
        {0.4, 0.5, QColor(240, 240, 80), "Moyen"},                          // Jaune
        {0.5, 0.6, QColor(150, 200, 80), "Bon"},                            // Jaune-vert
        {0.6, 0.7, QColor(92, 184, 92), "Très bon"},                        // Vert
        {0.7, 1.0, QColor(32, 150, 80), "Excellent"}                       // Vert foncé
    };

    QString winRateExplanation =
        "Le <b>taux de réussite</b> (Win Rate) représente le pourcentage de trades gagnants. "
        "Bien qu'important, il doit être évalué en conjonction avec le ratio de profit/perte, "
        "car une stratégie avec un faible taux de réussite peut être profitable si les gains sont importants par rapport aux pertes.";
    m_winRateGauge = new RatioGaugeWidget(winRateZones, "Taux de Réussite", winRateExplanation);


    QVector<RatioGaugeWidget::GaugeZone> profitFactorZones = {
        {0.0, 1.0, QColor(217, 83, 79), "Non rentable"},                     // Rouge
        {1.0, 1.25, QColor(240, 173, 78), "Rentabilité marginale"},          // Orange
        {1.25, 1.5, QColor(240, 240, 80), "Rentabilité acceptable"},        // Jaune
        {1.5, 2.0, QColor(150, 200, 80), "Bonne rentabilité"},              // Jaune-vert
        {2.0, 3.0, QColor(92, 184, 92), "Très bonne rentabilité"},         // Vert
        {3.0, 4.0, QColor(32, 150, 80), "Excellente rentabilité"}         // Vert foncé
    };
    QString profitFactorExplanation = 
        "Le <b>facteur de profit</b> est le ratio entre les profits bruts et les pertes brutes. "
        "Un facteur de profit supérieur à 1 indique une stratégie rentable. "
        "Plus ce ratio est élevé, plus la stratégie est robuste face aux fluctuations du marché.";
    m_profitFactorGauge = new RatioGaugeWidget(profitFactorZones, "Facteur de Profit", profitFactorExplanation);


    QVector<RatioGaugeWidget::GaugeZone> kellyZones = {
        {-1.0, 0.0, QColor(217, 83, 79), "Non viable"},                       // Rouge
        { 0.0, 0.05, QColor(240, 173, 78), "Taille minimale"},                // Orange
        { 0.05, 0.15, QColor(240, 240, 80), "Taille conservative"},            // Jaune
        { 0.15, 0.25, QColor(150, 200, 80), "Taille optimale"},                // Jaune-vert
        { 0.25, 0.4, QColor(92, 184, 92), "Taille agressive"},               // Vert
        { 0.4, 1.0, QColor(32, 150, 80), "Très agressive"}                  // Vert foncé
    };
    QString kellyExplanation =
        "Le <b>critère de Kelly</b> détermine la taille optimale des positions pour maximiser la croissance du capital à long terme. "
        "En pratique, de nombreux traders utilisent une fraction de Kelly (25-50%) pour réduire la volatilité. "
        "Un critère négatif indique qu'on ne devrait pas trader cette stratégie.";
    m_kellyGauge = new RatioGaugeWidget(kellyZones, "Critère de Kelly", kellyExplanation);


    QVector<RatioGaugeWidget::GaugeZone> drawdownZones = {
        {100.0, 50.0, QColor(217, 83, 79), "Critique"},                      // Rouge
        {50.0, 30.0, QColor(240, 173, 78), "Sévère"},                       // Orange
        {30.0, 20.0, QColor(240, 240, 80), "Important"},                    // Jaune
        {20.0, 10.0, QColor(150, 200, 80), "Modéré"},                       // Jaune-vert
        {10.0, 5.0, QColor(92, 184, 92), "Faible"},                        // Vert
        {5.0, 0.0, QColor(32, 150, 80), "Très faible"}                   // Vert foncé
    };
    QString drawdownExplanation = 
        "Le <b>drawdown maximal</b> mesure la perte maximale subie entre un pic et un creux de l'équité. "
        "C'est un indicateur clé de risque qui montre la pire perte qu'un trader aurait pu subir. "
        "Un drawdown plus faible est préférable et indique une meilleure gestion du risque.";
    m_maxDrawdownGauge = new RatioGaugeWidget(drawdownZones, "Drawdown Max", drawdownExplanation);
    m_maxDrawdownGauge->setValueFormat(1, true); // Afficher en pourcentage avec 1 décimale


    QVector<RatioGaugeWidget::GaugeZone> sqnZones = {
        {-10.0, 1.6, QColor(217, 83, 79), "Médiocre"},                        // Rouge
        { 1.6, 2.0, QColor(240, 173, 78), "Moyen"},                          // Orange
        { 2.0, 2.5, QColor(240, 240, 80), "Bon"},                           // Jaune
        { 2.5, 3.0, QColor(150, 200, 80), "Très bon"},                      // Jaune-vert
        { 3.0, 5.0, QColor(92, 184, 92), "Excellent"},                      // Vert
        { 5.0, 10.0, QColor(32, 150, 80), "Extraordinaire"}                 // Vert foncé
    };
    QString sqnExplanation =
        "Le <b>System Quality Number (SQN)</b> mesure la qualité globale d'un système de trading. "
        "Il prend en compte le rendement moyen par trade, l'écart-type des rendements et le nombre de trades. "
        "Un SQN plus élevé indique un système plus robuste et plus fiable.";
    m_sqnGauge = new RatioGaugeWidget(sqnZones, "SQN", sqnExplanation);

    // Ajouter les jauges au layout en grille
    gridLayout->addWidget(m_profitFactorGauge, 0, 0);
    gridLayout->addWidget(m_winRateGauge, 1, 0);
    gridLayout->addWidget(m_maxDrawdownGauge, 2, 0);
    gridLayout->addWidget(m_sharpeGauge, 3, 0);
    gridLayout->addWidget(m_sortinoGauge, 0, 1);
    gridLayout->addWidget(m_calmarGauge, 1, 1);
    gridLayout->addWidget(m_sqnGauge, 2, 1);
    gridLayout->addWidget(m_kellyGauge, 3, 1);

    // Définir les tailles
    // setMinimumHeight(250);
}

void RatioGaugesContainerWidget::updateContent(const be::Stats& stats)
{
    // Mettre à jour chaque jauge avec les valeurs des statistiques
    m_sharpeGauge->setValue(stats.sharpeRatio);
    m_sortinoGauge->setValue(stats.sortinoRatio);
    m_calmarGauge->setValue(stats.calmarRatio);
    m_winRateGauge->setValue(stats.pctTPTrades * 0.01); // Convertir entre 0-1
    m_profitFactorGauge->setValue(stats.profitFactor);
    m_sqnGauge->setValue(stats.sqn);
    m_maxDrawdownGauge->setValue(std::abs(stats.maxDrawdownPct));
    m_kellyGauge->setValue(stats.kellyCriterion * 0.01); // Convertir entre 0-1
}

void RatioGaugesContainerWidget::clear()
{
    // Réinitialiser toutes les jauges à zéro
    m_sharpeGauge->clear();
    m_sortinoGauge->clear();
    m_calmarGauge->clear();
    m_winRateGauge->clear();
    m_profitFactorGauge->clear();
    m_sqnGauge->clear();
    m_maxDrawdownGauge->clear();
    m_kellyGauge->clear();
}