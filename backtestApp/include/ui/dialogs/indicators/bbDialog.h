#pragma once

#include "ui/dialogs/indicators/baseDialog.h"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>

/**
 * @brief Modal dialog to edit the parameters of a Stochastic indicator
 */
class bbDialog : public IndicatorDialog<indicators::BBInstance>
{
    Q_OBJECT
    
public:
    bbDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::BBInstance& bollingerBands);
    ~bbDialog() override;

private slots:
    void onPeriodChanged(int period);
    void onStdDevMultiplierChanged(double multiplier);
    void onSourceChanged(int index);
    void onMATypeChanged(int index);
    void onMiddleBandColorClicked();
    void onUpperBandColorClicked();
    void onLowerBandColorClicked();

protected:
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:
    // UI controls
    QSpinBox* m_periodSpinBox;
    QDoubleSpinBox* m_stdDevMultiplierSpinBox;
    QComboBox* m_sourceCombo;
    QComboBox* m_maTypeCombo;
    QPushButton* m_middleBandColorButton;
    QPushButton* m_upperBandColorButton;
    QPushButton* m_lowerBandColorButton;

    // Helper used by color button handlers
    void onColorButtonClicked(QPushButton* button, int& colorField);
};