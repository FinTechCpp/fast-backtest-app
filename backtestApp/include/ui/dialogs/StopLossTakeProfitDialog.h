#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include "common.h"

class StopLossTakeProfitDialog : public QDialog {
    Q_OBJECT

public:
    explicit StopLossTakeProfitDialog(QWidget* parent = nullptr);
    ~StopLossTakeProfitDialog() override = default;

    void setConfig(const StrategyConfig& config);
    void updateConfig(StrategyConfig& config) const;

private:
    void setupUI();
    
    // Widgets for Stop Loss
    QComboBox* m_slMethodCombo;
    QLabel* m_slDistanceLabel;
    QDoubleSpinBox* m_stopLossDistanceSpin;
    QLabel* m_slAtrMultiplierLabel;
    QDoubleSpinBox* m_slAtrMultiplierSpin;
    QLabel* m_slMinmaxPeriodsLabel;
    QSpinBox* m_slMinmaxPeriodsSpin;
    QLabel* m_slMinmaxCoefLabel;
    QDoubleSpinBox* m_slMinmaxCoefAtr;
    QDoubleSpinBox* m_minStopLossDistanceSpin;
    
    // Widgets for Take Profit
    QComboBox* m_tpMethodCombo;
    QLabel* m_tpDistanceLabel;
    QDoubleSpinBox* m_takeProfitDistanceSpin;
    QLabel* m_tpAtrMultiplierLabel;
    QDoubleSpinBox* m_tpAtrMultiplierSpin;
    QLabel* m_tpSlRatioLabel;
    QDoubleSpinBox* m_tpSlRatioSpin;
    QLabel* m_rlLookbackLabel;
    QSpinBox* m_rlLookbackPeriodsSpin;
    QDoubleSpinBox* m_minTakeProfitDistanceSpin;
    
    // Common ATR period
    QLabel* m_atrLabel;
    QSpinBox* m_atrPeriodSpin;
};
