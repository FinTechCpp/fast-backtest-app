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
#include "chart_renderer.h"

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur ATR (Average True Range)
 */
class ATRDialog : public QDialog
{
    Q_OBJECT
    
public:
    // Construction pour ATR
    ATRDialog(QWidget* parent, ChartWidget* chartWidget, int atrId, const ATRInstance& atr);
    ~ATRDialog();
    
private slots:
    void onApply();
    void onCancel();
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onColorButtonClicked();
    
private:
    ChartWidget* m_chartWidget;
    int m_atrId;
    ATRInstance m_originalAtr;  // Pour restaurer en cas d'annulation
    ATRInstance m_currentAtr;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QPushButton* m_colorButton;
    QDialogButtonBox* m_buttonBox;
    
    void updateColorButtonStyle(QPushButton* button, int color);
    void updateATR();
};