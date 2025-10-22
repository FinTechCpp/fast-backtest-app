#pragma once

#include "ui/dialogs/indicators/baseDialog.h"
#include <QSpinBox>
#include <QDoubleSpinBox>

/**
 * @brief Modal dialog to edit the parameters of a Supertrend indicator
 */
class SupertrendDialog : public IndicatorDialog<indicators::SuperTrendInstance>
{
    Q_OBJECT
    
public:
    SupertrendDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::SuperTrendInstance& supertrend);
    ~SupertrendDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onMultiplierChanged(double multiplier);
    void onUpColorButtonClicked();
    void onDownColorButtonClicked();
    
protected:
    // Virtual methods from BaseDialog
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:

    QSpinBox* m_periodSpinBox;
    QDoubleSpinBox* m_multiplierSpinBox;
    QPushButton* m_upColorButton;
    QPushButton* m_downColorButton;
};