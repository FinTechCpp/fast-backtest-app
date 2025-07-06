#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QFrame>

#include "dialog/indicatorDialog.h"

/**
 * @brief Modal dialog to modify the parameters of a technical indicator
 */
class RSIDialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Constructor for RSI

    RSIDialog(QWidget* parent, ChartWidget* chartWidget, int rsiId, const RSIInstance& rsi);
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
    void applyChanges() override;
    void cancelChanges() override;
    
private:
    int m_rsiId;
    RSIInstance m_originalRsi;  // To restore in case of cancellation
    RSIInstance m_currentRsi;   // For ongoing modifications
    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QSpinBox* m_overboughtLevelSpinBox;
    QSpinBox* m_oversoldLevelSpinBox;
    QPushButton* m_colorButton;
    QPushButton* m_upperColorButton;
    QPushButton* m_lowerColorButton;
};

