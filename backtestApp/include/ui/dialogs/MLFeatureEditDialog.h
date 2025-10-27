#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include "common.h"

class MLFeatureEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit MLFeatureEditDialog(QWidget* parent = nullptr);
    ~MLFeatureEditDialog() override = default;

    // Set the feature to edit
    void setFeature(const StrategyConfig::MLFeatureConfig& feature);
    
    // Get the configured feature
    StrategyConfig::MLFeatureConfig getFeature() const;

private slots:
    void onIndicatorTypeChanged(int index);
    void onTransformChanged(int index);
    void onCompositeTypeChanged(int index);
    void updatePreview();

private:
    void setupUI();
    void setupParameterWidgets();
    void clearParameterWidgets();
    void populateParametersFromFeature(const StrategyConfig::MLFeatureConfig& feature);
    
    // Helper to create ValueSource from current UI state
    filter::ValueSource getConfiguredValueSource() const;
    
    // Current feature being edited
    StrategyConfig::MLFeatureConfig m_feature;
    
    // Widgets - Mode selection
    QComboBox* m_featureModeCombo;  // Simple indicator vs Composite operation
    
    // Widgets - Simple indicator mode
    QComboBox* m_indicatorTypeCombo;
    QComboBox* m_transformCombo;
    QGroupBox* m_parametersGroup;
    QGridLayout* m_parametersLayout;
    
    // Widgets - Composite mode (e.g., BB_WIDTH = distance between BB_UPPER and BB_LOWER)
    QGroupBox* m_compositeGroup;
    QComboBox* m_compositeTypeCombo;  // DISTANCE, RATIO, etc.
    QComboBox* m_leftIndicatorCombo;
    QComboBox* m_rightIndicatorCombo;
    QGridLayout* m_leftParamsLayout;
    QGridLayout* m_rightParamsLayout;
    
    // Preview
    QLabel* m_previewLabel;
    
    // Parameter widgets (dynamically created based on indicator type)
    QSpinBox* m_periodSpinBox;
    QDoubleSpinBox* m_multiplierSpinBox;
    QSpinBox* m_kPeriodSpinBox;
    QSpinBox* m_dPeriodSpinBox;
    QSpinBox* m_smoothSpinBox;
    QSpinBox* m_fastPeriodSpinBox;
    QSpinBox* m_slowPeriodSpinBox;
    QSpinBox* m_signalPeriodSpinBox;
    
    // Left side parameters (for composite mode)
    QSpinBox* m_leftPeriodSpinBox;
    QDoubleSpinBox* m_leftMultiplierSpinBox;
    QSpinBox* m_leftKPeriodSpinBox;
    QSpinBox* m_leftDPeriodSpinBox;
    QSpinBox* m_leftSmoothSpinBox;
    QSpinBox* m_leftFastPeriodSpinBox;
    QSpinBox* m_leftSlowPeriodSpinBox;
    QSpinBox* m_leftSignalPeriodSpinBox;
    
    // Right side parameters (for composite mode)
    QSpinBox* m_rightPeriodSpinBox;
    QDoubleSpinBox* m_rightMultiplierSpinBox;
    QSpinBox* m_rightKPeriodSpinBox;
    QSpinBox* m_rightDPeriodSpinBox;
    QSpinBox* m_rightSmoothSpinBox;
    QSpinBox* m_rightFastPeriodSpinBox;
    QSpinBox* m_rightSlowPeriodSpinBox;
    QSpinBox* m_rightSignalPeriodSpinBox;
    
    // Custom name
    QLineEdit* m_customNameEdit;
};
