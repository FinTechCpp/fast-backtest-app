#pragma once

#include <QSpinBox>
#include <QCheckBox>

#include "ui/dialogs/baseDialog.h"

/**
 * @brief Modal dialog to modify the parameters of an ATR (Average True Range) indicator
 */
class ATRDialog : public IndicatorDialog<indicators::ATRInstance>
{
    Q_OBJECT
    
public:
    ATRDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::ATRInstance& atr);
    ~ATRDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onLogScaleChanged(int state);
    void onColorButtonClicked();
    
protected:
    //Virtual methods from BaseDialog
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QCheckBox* m_useLogScaleCheckBox;
    QPushButton* m_colorButton;
};