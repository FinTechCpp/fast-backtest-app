#pragma once

#include "dialog/indicator_dialog.h"
#include <QSpinBox>

/**
 * @brief Dialogue modal pour modifier les paramètres d'un EMA (Exponential Moving Average)
 */
class EMADialog : public IndicatorDialog
{
    Q_OBJECT
    
public:
    // Construction pour un seul EMA
    EMADialog(QWidget* parent, ChartWidget* chartWidget, int emaId, const EMAInstance& ema);
    ~EMADialog() override;
    
private slots:
    void onPeriodChanged(int period);
    void onColorButtonClicked();
    
protected:
    // Méthodes virtuelles de IndicatorDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;
    
private:
    int m_emaId;
    EMAInstance m_originalEma;  // Pour restaurer en cas d'annulation
    EMAInstance m_currentEma;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QPushButton* m_colorButton;
};
