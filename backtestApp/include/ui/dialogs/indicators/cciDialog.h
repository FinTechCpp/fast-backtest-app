#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QFrame>

#include "ui/dialogs/indicators/baseDialog.h"

/**
 * @brief Modal dialog to modify the parameters of CCI indicator
 */
class CCIDialog : public IndicatorDialog<indicators::CCIInstance>
{
    Q_OBJECT
    
public:
    CCIDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::CCIInstance& cci);
    ~CCIDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onResetOnNewDayChanged(int state);
    void onUpperLevelChanged(int level);
    void onLowerLevelChanged(int level);
    void onColorButtonClicked();
    void onUpperColorButtonClicked();
    void onLowerColorButtonClicked();

protected:
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;
    
private:    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QSpinBox* m_upperLevelSpinBox;
    QSpinBox* m_lowerLevelSpinBox;
    QPushButton* m_colorButton;
    QPushButton* m_upperColorButton;
    QPushButton* m_lowerColorButton;
    QCheckBox* m_resetOnNewDayCheckBox;
};
