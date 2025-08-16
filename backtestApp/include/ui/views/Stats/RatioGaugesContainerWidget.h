#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QVector>
#include "ui/views/Stats/RatioGaugeWidget.h"
#include "stats.hpp"

/**
 * @brief Widget conteneur pour afficher plusieurs jauges de ratios
 */
class RatioGaugesContainerWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param parent Widget parent
     */
    explicit RatioGaugesContainerWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Mettre à jour les données avec les statistiques du backtest
     * @param stats Statistiques calculées
     */
    void updateData(const be::Stats& stats);
    
    /**
     * @brief Effacer toutes les données et réinitialiser l'affichage
     */
    void clear();

private:
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_gaugesLayout;
    
    // Jauges individuelles
    RatioGaugeWidget* m_sharpeGauge;
    RatioGaugeWidget* m_sortinoGauge;
    RatioGaugeWidget* m_calmarGauge;
    RatioGaugeWidget* m_winRateGauge;
    RatioGaugeWidget* m_profitFactorGauge;
    RatioGaugeWidget* m_sqnGauge;
    RatioGaugeWidget* m_maxDrawdownGauge;
    RatioGaugeWidget* m_kellyGauge;
};