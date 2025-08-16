#include "ui/views/Stats/RatioGaugesContainerWidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

RatioGaugesContainerWidget::RatioGaugesContainerWidget(QWidget* parent)
    : QWidget(parent)
{
    // Layout principal
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Widget de contenu
    m_contentWidget = new QWidget();
    m_mainLayout->addWidget(m_contentWidget);
    
    // Layout des jauges en grille (2 colonnes)
    QGridLayout* gridLayout = new QGridLayout(m_contentWidget);
    gridLayout->setSpacing(15);
    
    // Créer les jauges
    m_sharpeGauge = new RatioGaugeWidget();
    m_sharpeGauge->setRatioType(RatioGaugeWidget::RatioType::Sharpe);
    
    m_sortinoGauge = new RatioGaugeWidget();
    m_sortinoGauge->setRatioType(RatioGaugeWidget::RatioType::Sortino);
    
    m_calmarGauge = new RatioGaugeWidget();
    m_calmarGauge->setRatioType(RatioGaugeWidget::RatioType::Calmar);
    
    m_winRateGauge = new RatioGaugeWidget();
    m_winRateGauge->setRatioType(RatioGaugeWidget::RatioType::WinRate);
    
    m_profitFactorGauge = new RatioGaugeWidget();
    m_profitFactorGauge->setRatioType(RatioGaugeWidget::RatioType::ProfitFactor);
    
    m_sqnGauge = new RatioGaugeWidget();
    m_sqnGauge->setRatioType(RatioGaugeWidget::RatioType::SQN);
    
    m_maxDrawdownGauge = new RatioGaugeWidget();
    m_maxDrawdownGauge->setRatioType(RatioGaugeWidget::RatioType::MaxDrawdown);
    
    m_kellyGauge = new RatioGaugeWidget();
    m_kellyGauge->setRatioType(RatioGaugeWidget::RatioType::Kelly);
    
    // Ajouter les jauges au layout en grille
    gridLayout->addWidget(m_profitFactorGauge, 0, 0);
    gridLayout->addWidget(m_winRateGauge, 0, 1);
    gridLayout->addWidget(m_sharpeGauge, 1, 0);
    gridLayout->addWidget(m_sortinoGauge, 1, 1);
    gridLayout->addWidget(m_calmarGauge, 2, 0);
    gridLayout->addWidget(m_sqnGauge, 2, 1);
    gridLayout->addWidget(m_maxDrawdownGauge, 3, 0);
    gridLayout->addWidget(m_kellyGauge, 3, 1);

    // Définir les tailles
    setMinimumHeight(500);
}

void RatioGaugesContainerWidget::updateData(const be::Stats& stats)
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