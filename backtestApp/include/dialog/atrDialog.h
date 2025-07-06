#pragma once

#include <QSpinBox>
#include <QCheckBox>

#include "dialog/indicatorDialog.h"

/**
 * @brief Modal dialog to modify the parameters of an ATR (Average True Range) indicator
 */
class ATRDialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Constructor for ATR
    ATRDialog(QWidget* parent, ChartWidget* chartWidget, int atrId, const ATRInstance& atr);
    ~ATRDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onLogScaleChanged(int state);
    void onColorButtonClicked();
    
protected:
    //Virtual methods from IndicatorDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;

private:
    int m_atrId;
    ATRInstance m_originalAtr;  // For restoring in case of cancellation
    ATRInstance m_currentAtr;   // For ongoing modifications

    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QCheckBox* m_useLogScaleCheckBox;  // New control
    QPushButton* m_colorButton;
};