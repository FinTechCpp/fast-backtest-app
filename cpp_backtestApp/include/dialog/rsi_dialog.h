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
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur technique
 */
class RSIDialog : public QDialog
{
    Q_OBJECT
    
public:
    // Construction pour RSI
    RSIDialog(QWidget* parent, ChartWidget* chartWidget, int rsiId, const RSIInstance& rsi);
    ~RSIDialog();
    
private slots:
    void onApply();
    void onCancel();
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onRangeChanged(double range);
    void onColorButtonClicked();
    void onUpperColorButtonClicked();
    void onLowerColorButtonClicked();
    
private:
    ChartWidget* m_chartWidget;
    int m_rsiId;
    RSIInstance m_originalRsi;  // Pour restaurer en cas d'annulation
    RSIInstance m_currentRsi;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QDoubleSpinBox* m_rangeSpinBox;
    QPushButton* m_colorButton;
    QPushButton* m_upperColorButton;
    QPushButton* m_lowerColorButton;
    QDialogButtonBox* m_buttonBox;
    
    void updateColorButtonStyle(QPushButton* button, int color);
    void updateRSI();
};

