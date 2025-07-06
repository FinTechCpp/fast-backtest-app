#pragma once

#include "dialog/indicatorDialog.h"
#include <QSpinBox>

/**
 * @brief Modal dialog to edit the parameters of an EMA (Exponential Moving Average)
 */
class EMADialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Constructor for one EMA
    EMADialog(QWidget* parent, ChartWidget* chartWidget, int emaId, const EMAInstance& ema);
    ~EMADialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onColorButtonClicked();
    
protected:
    // Virtual methods from IndicatorDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;
    
private:
    int m_emaId;
    EMAInstance m_originalEma;  // For restoring in case of cancellation
    EMAInstance m_currentEma;   // For ongoing modifications

    QSpinBox* m_periodSpinBox;
    QPushButton* m_colorButton;
};
