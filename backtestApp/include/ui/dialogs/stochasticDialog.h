#pragma once

#include "ui/dialogs/baseDialog.h"
#include <QSpinBox>

/**
 * @brief Modal dialog to edit the parameters of a Stochastic indicator
 */
class StochasticDialog : public BaseDialog
{
    Q_OBJECT
    
public:
    // Stochastic constructor
    StochasticDialog(QWidget* parent, ChartWidget* chartWidget, int stochasticId, const StochasticInstance& stochastic);
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
    void applyChanges() override;
    void cancelChanges() override;

private:
    int m_stochasticId;
    StochasticInstance m_originalStochastic;  // To restore in case of cancellation
    StochasticInstance m_currentStochastic;   // For ongoing modifications

    QSpinBox* m_fastKPeriodSpinBox;
    QSpinBox* m_slowKPeriodSpinBox;
    QSpinBox* m_slowDPeriodSpinBox;
    QSpinBox* m_heightSpinBox;
    QSpinBox* m_overboughtLevelSpinBox; 
    QSpinBox* m_oversoldLevelSpinBox;   
    QPushButton* m_kColorButton;
    QPushButton* m_dColorButton;
};