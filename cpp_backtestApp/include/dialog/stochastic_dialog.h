#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QFrame>
#include <QFormLayout>

#include "chart_widget.h"

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur Stochastique
 */
class StochasticDialog : public QDialog
{
    Q_OBJECT
    
public:
    // Construction pour Stochastic
    StochasticDialog(QWidget* parent, ChartWidget* chartWidget, int stochasticId, const ChartWidget::StochasticInstance& stochastic);
    ~StochasticDialog();
    
private slots:
    void onApply();
    void onCancel();
    void onFastKPeriodChanged(int period);
    void onSlowKPeriodChanged(int period);
    void onSlowDPeriodChanged(int period);
    void onHeightChanged(int height);
    void onKColorButtonClicked();
    void onDColorButtonClicked();
    
private:
    ChartWidget* m_chartWidget;
    int m_stochasticId;
    ChartWidget::StochasticInstance m_originalStochastic;  // Pour restaurer en cas d'annulation
    ChartWidget::StochasticInstance m_currentStochastic;   // Pour les modifications en cours
    
    QSpinBox* m_fastKPeriodSpinBox;
    QSpinBox* m_slowKPeriodSpinBox;
    QSpinBox* m_slowDPeriodSpinBox;
    QSpinBox* m_heightSpinBox;
    QPushButton* m_kColorButton;
    QPushButton* m_dColorButton;
    QDialogButtonBox* m_buttonBox;
    
    void updateColorButtonStyle(QPushButton* button, int color);
};