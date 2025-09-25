#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QScrollArea>
#include <QMessageBox>
#include "common.h"
#include "Strategies/generic_strategy.hpp"

class FilterConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilterConfigWidget(QWidget* parent = nullptr);
    explicit FilterConfigWidget(const GenericFilter& filter, QWidget* parent = nullptr);
    
    GenericFilter getFilter() const;
    void setFilter(const GenericFilter& filter);

private slots:
    void onLeftValueCategoryChanged();
    void onRightValueCategoryChanged();
    void onLeftIndicatorTypeChanged();
    void onRightIndicatorTypeChanged();

private:
    void setupUI();
    void setupValueSourceWidgets(QFormLayout* layout, const QString& prefix, 
                                QComboBox*& categoryCombo,
                                QComboBox*& typeCombo,
                                QWidget*& paramWidget,
                                QSpinBox*& offsetSpin,
                                QDoubleSpinBox*& constantSpin);
    
    void updateParameterWidgets(QComboBox* categoryCombo, QComboBox* typeCombo, 
                               QWidget*& paramWidget, bool isLeft);
    
    void updateIndicatorParameterWidgets(QComboBox* typeCombo, QWidget*& paramWidget, bool isLeft);
    
    ValueSource getValueSourceFromWidgets(QComboBox* categoryCombo, QComboBox* typeCombo,
                                         QWidget* paramWidget, QSpinBox* offsetSpin,
                                         QDoubleSpinBox* constantSpin) const;
    
    void setValueSourceToWidgets(const ValueSource& source, QComboBox* categoryCombo, 
                                QComboBox* typeCombo, QWidget*& paramWidget,
                                QSpinBox* offsetSpin, QDoubleSpinBox* constantSpin);

    // Left value widgets
    QComboBox* m_leftCategoryCombo;
    QComboBox* m_leftTypeCombo;
    QWidget* m_leftParamWidget;
    QSpinBox* m_leftOffsetSpin;
    QDoubleSpinBox* m_leftConstantSpin;

    // Right value widgets  
    QComboBox* m_rightCategoryCombo;
    QComboBox* m_rightTypeCombo;
    QWidget* m_rightParamWidget;
    QSpinBox* m_rightOffsetSpin;
    QDoubleSpinBox* m_rightConstantSpin;

    // Comparison widgets
    QComboBox* m_comparisonCombo;
    QComboBox* m_temporalLogicCombo;
    QSpinBox* m_lookbackPeriodsSpin;
    QCheckBox* m_enabledCheck;
    QLineEdit* m_descriptionEdit;
};

class StrategyCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StrategyCreationDialog(QWidget* parent = nullptr);
    explicit StrategyCreationDialog(const GenericStrategyConfig& existingConfig, QWidget* parent = nullptr);

    GenericStrategyConfig getStrategyConfig() const;

private slots:
    void onAddFilter();
    void onRemoveFilter();
    void onFilterSelectionChanged();
    void accept() override;

private:
    void setupUI();
    void addFilterToList(const GenericFilter& filter);
    void updateFilterList();

    // Strategy configuration
    QLineEdit* m_strategyNameEdit;
    QComboBox* m_directionCombo;
    
    // Filters list
    QListWidget* m_filtersList;
    QPushButton* m_addFilterBtn;
    QPushButton* m_removeFilterBtn;
    
    // Filter details
    QScrollArea* m_filterDetailsArea;
    FilterConfigWidget* m_currentFilterWidget;
    
    // Dialog buttons
    QPushButton* m_okBtn;
    QPushButton* m_cancelBtn;

    std::vector<GenericFilter> m_filters;
    bool m_editMode;
    int m_currentFilterIndex;
};