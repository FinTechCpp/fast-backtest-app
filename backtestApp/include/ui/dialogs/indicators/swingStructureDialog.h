#pragma once

#include "ui/dialogs/indicators/baseDialog.h"
#include <QSpinBox>
#include <QDoubleSpinBox>

/**
 * @brief Modal dialog to edit the parameters of a Swing Structure Trend indicator
 */
class SwingStructureDialog : public IndicatorDialog<indicators::SwingStructureInstance>
{
    Q_OBJECT

public:
    SwingStructureDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::SwingStructureInstance& swingStructure);
    ~SwingStructureDialog() override;

private slots:
    void onHighMoveChanged(double highMove);
    void onLowMoveChanged(double lowMove);
    void onMinPeriodsChanged(int minPeriods);
    void onMaxPeriodsChanged(int maxPeriods);
    void onResetOnNewDayChanged(int state);
    void onSwingHighColorButtonClicked();
    void onSwingLowColorButtonClicked();
    void onUpColorButtonClicked();
    void onDownColorButtonClicked();
    void onUncertainColorButtonClicked();

protected:
    // Virtual methods from BaseDialog
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:
    QDoubleSpinBox* m_highMoveSpinBox;
    QDoubleSpinBox* m_lowMoveSpinBox;
    QSpinBox* m_minPeriodsSpinBox;
    QSpinBox* m_maxPeriodsSpinBox;
    QCheckBox* m_resetOnNewDayCheckBox;
    QPushButton* m_swingHighColorButton;
    QPushButton* m_swingLowColorButton;
    QPushButton* m_upColorButton;
    QPushButton* m_downColorButton;
    QPushButton* m_uncertainColorButton;
};
