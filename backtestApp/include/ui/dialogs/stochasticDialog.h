#pragma once

#include "ui/dialogs/baseDialog.h"
#include <QSpinBox>

/**
 * @brief Modal dialog to edit the parameters of a Stochastic indicator
 */
class StochasticDialog : public IndicatorDialog<StochasticInstance>
{
    Q_OBJECT
    
public:
    StochasticDialog(QWidget* parent, ChartWidget* chartWidget, const StochasticInstance& stochastic);
    ~StochasticDialog() override;
    
private slots:
    void onFastKPeriodChanged(int period);
    void onSlowKPeriodChanged(int period);
    void onSlowDPeriodChanged(int period);
    void onHeightChanged(int height);
    void onKColorButtonClicked();
    void onDColorButtonClicked();
    void onOverboughtLevelChanged(int level); 
    void onOversoldLevelChanged(int level);  

protected:
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:
    QSpinBox* m_fastKPeriodSpinBox;
    QSpinBox* m_slowKPeriodSpinBox;
    QSpinBox* m_slowDPeriodSpinBox;
    QSpinBox* m_heightSpinBox;
    QSpinBox* m_overboughtLevelSpinBox; 
    QSpinBox* m_oversoldLevelSpinBox;   
    QPushButton* m_kColorButton;
    QPushButton* m_dColorButton;
};