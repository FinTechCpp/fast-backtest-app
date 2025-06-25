#pragma once

#include <QSpinBox>
#include <QCheckBox>

#include "dialog/indicator_dialog.h"

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur ATR (Average True Range)
 */
class ATRDialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Construction pour ATR
    ATRDialog(QWidget* parent, ChartWidget* chartWidget, int atrId, const ATRInstance& atr);
    ~ATRDialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onLogScaleChanged(int state);
    void onColorButtonClicked();
    
protected:
    // Méthodes virtuelles de IndicatorDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;

private:
    int m_atrId;
    ATRInstance m_originalAtr;  // Pour restaurer en cas d'annulation
    ATRInstance m_currentAtr;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QCheckBox* m_useLogScaleCheckBox;  // Nouveau contrôle
    QPushButton* m_colorButton;
};