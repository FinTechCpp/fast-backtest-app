#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QFormLayout>

#include "ui/chart/chartWidget.h"
#include "ui/chart/chartTypes.h"

/**
 * @brief Base class for all indicator configuration dialogs
 */
class BaseDialog : public QDialog
{
    Q_OBJECT
    
public:
    BaseDialog(QWidget* parent, const QString& title);
    virtual ~BaseDialog();

protected:
    QFormLayout* m_formLayout;
    
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
    virtual void resetToDefaults() = 0;
    
private:
    QVBoxLayout* m_mainLayout;
    QDialogButtonBox* m_buttonBox;
    QLabel* m_resetLink;

private slots:
    void onApply();
    void onCancel();
    void onReset();
};

/**
 * @brief Base template class for all indicator configuration dialogs
 */
template<typename IndicatorType>
class IndicatorDialog : public BaseDialog
{
public:
    IndicatorDialog(QWidget* parent, const QString& title, ChartWidget* chartWidget, const IndicatorType& originalIndicator)
        : BaseDialog(parent, title)
        , m_chartWidget(chartWidget)
        , m_originalIndicator(originalIndicator)
        , m_currentIndicator(originalIndicator)
    {
    }
    
protected:
    IndicatorType m_originalIndicator;
    IndicatorType m_currentIndicator;
    
    // Implementation of common methods from BaseDialog
    void applyChanges() override {
        m_chartWidget->updateIndicator(m_currentIndicator);
    }
    
    void cancelChanges() override {
        m_chartWidget->updateIndicator(m_originalIndicator);
    }
    
    virtual void updateUIFromInstance() = 0;
    
    void resetToDefaults() override {
        m_currentIndicator.setDefaults();
        updateUIFromInstance();
        applyChanges();
    }

    void initialize() {
        setupUI();
        connectSignals();
        updateUIFromInstance();
    }

private:
    ChartWidget* m_chartWidget;
};