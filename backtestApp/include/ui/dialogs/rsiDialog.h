#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QFrame>

#include "ui/dialogs/baseDialog.h"

/**
 * @brief Modal dialog to modify the parameters of a technical indicator
 */
class RSIDialog : public IndicatorDialog<indicators::RSIInstance>
{
    Q_OBJECT
    
public:
    RSIDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::RSIInstance& rsi);
    ~RSIDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    // void onRangeChanged(double range);
    void onOverboughtLevelChanged(int level);
    void onOversoldLevelChanged(int level);
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
    QSpinBox* m_overboughtLevelSpinBox;
    QSpinBox* m_oversoldLevelSpinBox;
    QPushButton* m_colorButton;
    QPushButton* m_upperColorButton;
    QPushButton* m_lowerColorButton;
};

