#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QFormLayout>
#include "chartWidget.h"

/**
 * @brief Base class for all indicator configuration dialogs
 */
class IndicatorDialog : public QDialog
{
    Q_OBJECT
    
public:
    IndicatorDialog(QWidget* parent, ChartWidget* chartWidget, const QString& title);
    virtual ~IndicatorDialog();

protected:
    ChartWidget* m_chartWidget;
    QVBoxLayout* m_mainLayout;
    QFormLayout* m_formLayout;
    QDialogButtonBox* m_buttonBox;
    
    // Common utility methods
    void updateColorButtonStyle(QPushButton* button, int color);
    QColor openColorDialog(int currentColor, const QString& title);
    int colorFromRGB(int r, int g, int b);
    void getRGBComponents(int color, int& r, int& g, int& b);
    
    // Virtual methods for derived classes
    virtual void setupUI() = 0;
    virtual void connectSignals() = 0;
    virtual void applyChanges() = 0;
    virtual void cancelChanges() = 0;

private slots:
    void onApply();
    void onCancel();
};