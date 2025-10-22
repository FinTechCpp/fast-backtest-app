// filepath: /home/maxime/repos/fast-backtest-app/backtestApp/include/ui/dialogs/macdDialog.h
#pragma once

#include "ui/dialogs/indicators/baseDialog.h"
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>

/**
 * @brief Modal dialog to edit the parameters of a MACD indicator
 *
 * Mirrors configurable options in ThirdParty/Strategies/include/Indicators/macd.hpp:
 *  - fast period
 *  - slow period
 *  - signal period
 *  - optional signal smoothing period
 *  - source field (open/high/low/close)
 *  - oscillator MA type (EMA/SMA)
 *  - signal MA type (EMA/SMA)
 *
 * Also exposes color buttons for macd line, signal line and histogram.
 */
class MACDDialog : public IndicatorDialog<indicators::MACDInstance>
{
    Q_OBJECT

public:
    MACDDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::MACDInstance& macd);
    ~MACDDialog() override;

private slots:
    void onFastPeriodChanged(int period);
    void onSlowPeriodChanged(int period);
    void onSignalPeriodChanged(int period);
    void onSignalSmoothingChanged(int period);
    void onSourceChanged(int index);
    void onOscMATypeChanged(int index);
    void onSignalMATypeChanged(int index);
    void onMacdColorButtonClicked();
    void onSignalColorButtonClicked();
    void onHistColorButtonClicked();

protected:
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:
    QSpinBox* m_fastPeriodSpinBox;
    QSpinBox* m_slowPeriodSpinBox;
    QSpinBox* m_signalPeriodSpinBox;
    QSpinBox* m_signalSmoothingSpinBox; // 0 means "use signal period"

    QComboBox* m_sourceComboBox;      // open/high/low/close
    QComboBox* m_oscMATypeComboBox;   // EMA/SMA
    QComboBox* m_signalMATypeComboBox; // EMA/SMA

    QPushButton* m_macdColorButton;
    QPushButton* m_signalColorButton;
    QPushButton* m_histColorButton;
};