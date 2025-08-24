#pragma once

#include <QVBoxLayout>
#include <QScrollArea>
#include <QVector>
#include "ui/views/Stats/RatioGaugeWidget.h"
#include "ui/views/Stats/StatsBaseWidget.h"

/**
 * @brief Widget conteneur pour afficher plusieurs jauges de ratios
 */
class RatioGaugesContainerWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit RatioGaugesContainerWidget(QWidget* parent = nullptr);
    
    void updateContent(const be::Stats& stats) override;
    void clear() override;

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