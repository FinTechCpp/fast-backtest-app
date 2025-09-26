#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QTabWidget>
#include "common.h"

class FilterEditDialog : public QDialog
{
    Q_OBJECT

public:
    FilterEditDialog(QWidget* parent = nullptr);
    
    // Setter/Getter pour le filtre
    void setFilter(const filter::GenericFilter& filter);
    filter::GenericFilter getFilter() const;

private slots:
    void onLeftValueCategoryChanged(int index);
    void onRightValueCategoryChanged(int index);
    void onLeftIndicatorTypeChanged(int index);
    void onRightIndicatorTypeChanged(int index);
    void onOkButtonClicked();
    void onCancelButtonClicked();
    void updatePreview();

private:
    void setupUI();
    void setupLeftValueUI(QWidget* parent);
    void setupRightValueUI(QWidget* parent);
    void setupOperatorUI(QWidget* parent);
    void setupTemporalLogicUI(QWidget* parent);
    void updateIndicatorParamsVisibility(QWidget* container, filter::IndicatorType type);
    
    filter::ValueSource getLeftValueSource() const;
    filter::ValueSource getRightValueSource() const;
    
    // Widgets pour le côté gauche
    QComboBox* m_leftCategoryCombo;
    QWidget* m_leftPriceWidget;
    QWidget* m_leftIndicatorWidget;
    QWidget* m_leftCandlePropertyWidget;
    QComboBox* m_leftPriceTypeCombo;
    QComboBox* m_leftIndicatorTypeCombo;
    QComboBox* m_leftCandlePropertyCombo;
    QSpinBox* m_leftHistoricalOffsetSpin;
    
    // Widgets spécifiques aux indicateurs de gauche
    QWidget* m_leftEMAWidget;
    QWidget* m_leftRSIWidget;
    QWidget* m_leftStochasticWidget;
    QWidget* m_leftATRWidget;
    QWidget* m_leftSuperTrendWidget;
    QSpinBox* m_leftEMAPeriodSpin;
    QSpinBox* m_leftRSIPeriodSpin;
    QSpinBox* m_leftStochFastKSpin;
    QSpinBox* m_leftStochSlowKSpin;
    QSpinBox* m_leftStochSlowDSpin;
    QSpinBox* m_leftATRPeriodSpin;
    QCheckBox* m_leftATRUseLogCheck;
    QSpinBox* m_leftSuperTrendPeriodSpin;
    QDoubleSpinBox* m_leftSuperTrendMultiplierSpin;
    
    // Widgets pour le côté droit
    QComboBox* m_rightCategoryCombo;
    QWidget* m_rightPriceWidget;
    QWidget* m_rightIndicatorWidget;
    QWidget* m_rightCandlePropertyWidget;
    QWidget* m_rightConstantWidget;
    QComboBox* m_rightPriceTypeCombo;
    QComboBox* m_rightIndicatorTypeCombo;
    QComboBox* m_rightCandlePropertyCombo;
    QDoubleSpinBox* m_rightConstantValueSpin;
    QSpinBox* m_rightHistoricalOffsetSpin;
    
    // Widgets spécifiques aux indicateurs de droite (similaires à gauche)
    QWidget* m_rightEMAWidget;
    QWidget* m_rightRSIWidget;
    QWidget* m_rightStochasticWidget;
    QWidget* m_rightATRWidget;
    QWidget* m_rightSuperTrendWidget;
    QSpinBox* m_rightEMAPeriodSpin;
    QSpinBox* m_rightRSIPeriodSpin;
    QSpinBox* m_rightStochFastKSpin;
    QSpinBox* m_rightStochSlowKSpin;
    QSpinBox* m_rightStochSlowDSpin;
    QSpinBox* m_rightATRPeriodSpin;
    QCheckBox* m_rightATRUseLogCheck;
    QSpinBox* m_rightSuperTrendPeriodSpin;
    QDoubleSpinBox* m_rightSuperTrendMultiplierSpin;
    
    // Widgets pour l'opérateur
    QComboBox* m_operatorCombo;
    
    // Widgets pour la logique temporelle
    QComboBox* m_temporalLogicCombo;
    QSpinBox* m_lookbackPeriodsSpin;
    
    // Widget pour l'aperçu du filtre
    QLabel* m_previewLabel;
    
    // Le filtre en cours d'édition
    filter::GenericFilter m_filter;
};