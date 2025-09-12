#pragma once

#include "ui/dialogs/baseDialog.h"
#include <QSpinBox>

/**
 * @brief Modal dialog to edit the parameters of an EMA (Exponential Moving Average)
 */
class EMADialog : public IndicatorDialog<indicators::EMAInstance>
{
    Q_OBJECT
    
public:
    EMADialog(QWidget* parent, ChartWidget* chartWidget, const indicators::EMAInstance& ema);
    ~EMADialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onColorButtonClicked();
    
protected:
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;
    
private:
    QSpinBox* m_periodSpinBox;
    QPushButton* m_colorButton;
};
