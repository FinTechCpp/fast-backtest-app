#pragma once

#include "dialog/indicator_dialog.h"
#include <QSpinBox>

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur Stochastique
 */
class StochasticDialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Construction pour Stochastic
    StochasticDialog(QWidget* parent, ChartWidget* chartWidget, int stochasticId, const StochasticInstance& stochastic);
    ~StochasticDialog() override;
    
private slots:
    void onFastKPeriodChanged(int period);
    void onSlowKPeriodChanged(int period);
    void onSlowDPeriodChanged(int period);
    void onHeightChanged(int height);
    void onKColorButtonClicked();
    void onDColorButtonClicked();
    void onOverboughtLevelChanged(int level); // Nouveau slot
    void onOversoldLevelChanged(int level);   // Nouveau slot

protected:
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;

private:
    int m_stochasticId;
    StochasticInstance m_originalStochastic;  // Pour restaurer en cas d'annulation
    StochasticInstance m_currentStochastic;   // Pour les modifications en cours
    
    QSpinBox* m_fastKPeriodSpinBox;
    QSpinBox* m_slowKPeriodSpinBox;
    QSpinBox* m_slowDPeriodSpinBox;
    QSpinBox* m_heightSpinBox;
    QSpinBox* m_overboughtLevelSpinBox; // Nouveau contrôle
    QSpinBox* m_oversoldLevelSpinBox;   // Nouveau contrôle
    QPushButton* m_kColorButton;
    QPushButton* m_dColorButton;
};