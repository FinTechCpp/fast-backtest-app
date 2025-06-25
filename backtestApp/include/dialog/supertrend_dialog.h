#pragma once

#include "dialog/indicator_dialog.h"
#include <QSpinBox>
#include <QDoubleSpinBox>

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur Supertrend
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
    // Méthodes virtuelles de IndicatorDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;

private:
    int m_supertrendId;
    SuperTrendInstance m_originalSupertrend;  // Pour restaurer en cas d'annulation
    SuperTrendInstance m_currentSupertrend;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QDoubleSpinBox* m_multiplierSpinBox;
    QPushButton* m_upColorButton;
    QPushButton* m_downColorButton;
};