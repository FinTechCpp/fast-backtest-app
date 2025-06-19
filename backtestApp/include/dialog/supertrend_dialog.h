#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QFrame>
#include <QFormLayout>

#include "chart_widget.h"

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur Supertrend
 */
class SupertrendDialog : public QDialog
{
    Q_OBJECT
    
public:
    SupertrendDialog(QWidget* parent, ChartWidget* chartWidget, int supertrendId, const SuperTrendInstance& supertrend);
    ~SupertrendDialog();
    
private slots:
    void onApply();
    void onCancel();
    void onPeriodChanged(int period);
    void onMultiplierChanged(double multiplier);
    void onUpColorButtonClicked();
    void onDownColorButtonClicked();
    
private:
    ChartWidget* m_chartWidget;
    int m_supertrendId;
    SuperTrendInstance m_originalSupertrend;  // Pour restaurer en cas d'annulation
    SuperTrendInstance m_currentSupertrend;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QDoubleSpinBox* m_multiplierSpinBox;
    QPushButton* m_upColorButton;
    QPushButton* m_downColorButton;
    QDialogButtonBox* m_buttonBox;
    
    void updateColorButtonStyle(QPushButton* button, int color);
};