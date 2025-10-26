#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QComboBox>
#include "common.h"

class MLConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit MLConfigDialog(QWidget* parent = nullptr);
    ~MLConfigDialog() override = default;

    // Set the strategy config to edit
    void setConfig(const StrategyConfig& config);
    
    // Update the config with ML settings
    void updateConfig(StrategyConfig& config);

private slots:
    void onAddFeature();
    void onRemoveFeature();
    void onMoveFeatureUp();
    void onMoveFeatureDown();
    void onUseMlEntryChanged(int state);
    void onBrowseModelPath();
    void updateButtonStates();

private:
    void setupUI();
    void refreshFeatureList();
    QString getFeatureDisplayName(const filter::IndicatorType& type, const std::string& params) const;
    
    // Configuration temporaire
    bool m_useMlEntry;
    std::string m_modelPath;
    int m_lookbackPeriods;
    float m_threshold;
    bool m_normalize;
    
    // Liste des features (indicateurs) à utiliser pour le modèle ML
    std::vector<StrategyConfig::MLFeatureConfig> m_features;
    
    // Widgets
    QCheckBox* m_useMlEntryCheck;
    QGroupBox* m_configGroup;
    QLineEdit* m_modelPathEdit;
    QPushButton* m_browseButton;
    QSpinBox* m_lookbackPeriodsSpinBox;
    QDoubleSpinBox* m_thresholdSpinBox;
    QCheckBox* m_normalizeCheck;
    
    QListWidget* m_featureList;
    QPushButton* m_addFeatureButton;
    QPushButton* m_removeFeatureButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;
    
    QComboBox* m_indicatorTypeCombo;
    QWidget* m_parameterWidget;
    QVBoxLayout* m_parameterLayout;
};
