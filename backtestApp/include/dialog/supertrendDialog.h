#pragma once

#include "dialog/indicatorDialog.h"
#include <QSpinBox>
#include <QDoubleSpinBox>

/**
 * @brief Modal dialog to edit the parameters of a Supertrend indicator
 */
class SupertrendDialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    SupertrendDialog(QWidget* parent, ChartWidget* chartWidget, int supertrendId, const SuperTrendInstance& supertrend);
    ~SupertrendDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onMultiplierChanged(double multiplier);
    void onUpColorButtonClicked();
    void onDownColorButtonClicked();
    
protected:
    // Virtual methods from IndicatorDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;

private:
    int m_supertrendId;
    SuperTrendInstance m_originalSupertrend;  // To restore in case of cancellation
    SuperTrendInstance m_currentSupertrend;   // For ongoing modifications

    QSpinBox* m_periodSpinBox;
    QDoubleSpinBox* m_multiplierSpinBox;
    QPushButton* m_upColorButton;
    QPushButton* m_downColorButton;
};