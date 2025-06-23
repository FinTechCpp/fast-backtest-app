#pragma once

#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QFrame>

#include "dialog/indicator_dialog.h"

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur technique
 */
class RSIDialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Construction pour RSI
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
    RSIInstance m_originalRsi;  // Pour restaurer en cas d'annulation
    RSIInstance m_currentRsi;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QSpinBox* m_overboughtLevelSpinBox;
    QSpinBox* m_oversoldLevelSpinBox;
    QPushButton* m_colorButton;
    QPushButton* m_upperColorButton;
    QPushButton* m_lowerColorButton;
};

