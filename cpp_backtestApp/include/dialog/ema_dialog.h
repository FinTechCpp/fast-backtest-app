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

#include "chart_widget.h"

/**
 * @brief Dialogue modal pour configurer plusieurs EMA
 */
class EMADialog : public QDialog
{
    Q_OBJECT
    
public:
    // Construction pour EMA
    EMADialog(QWidget* parent, ChartWidget* chartWidget);
    ~EMADialog();
    
private slots:
    void onApply();
    void onCancel();
    void onAddEMA();
    void onColorButtonClicked(int row);
    void onEnabledStateChanged(int row, bool enabled);
    void onPeriodChanged(int row, int period);
    
private:
    ChartWidget* m_chartWidget;
    std::vector<ChartWidget::EMAInstance> m_originalEMAs;  // Pour restaurer en cas d'annulation
    std::vector<ChartWidget::EMAInstance> m_currentEMAs;   // Pour les modifications en cours
    std::map<int, int> m_rowToEMAId;        // Mappage de la ligne de l'UI à l'ID de l'EMA
    
    QVBoxLayout* m_emaListLayout;
    QPushButton* m_addEMAButton;
    QDialogButtonBox* m_buttonBox;
    
    int m_nextRowId = 0;
    
    void updateColorButtonStyle(QPushButton* button, int color);
    QWidget* createEMARow(const ChartWidget::EMAInstance& ema, int row);
    void refreshEMAList();
};
