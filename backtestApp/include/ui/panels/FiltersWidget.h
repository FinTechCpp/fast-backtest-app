#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QMap>
#include <vector>
#include "common.h"  // To access GenericFilter

// Structure to group the widgets of a filter
struct FilterWidgetGroup {
    QWidget* container;
    QCheckBox* enableCheckbox;
    QPushButton* editButton;
    QPushButton* deleteButton;
};

class FiltersWidget : public QWidget
{
    Q_OBJECT

public:
    // Optional groupTitle lets the widget display a custom QGroupBox title
    // (default: "Strategy Filters").
    FiltersWidget(QWidget* parent = nullptr, const QString& groupTitle = QStringLiteral("Strategy Filters"));
    
    // Methods to access the filters
    void setFilters(const std::vector<filter::GenericFilter>& filters);
    const std::vector<filter::GenericFilter>& getFilters() const;
    
signals:
    void filtersChanged();
    
private slots:
    void onAddFilterClicked();
    void onEditFilterClicked(size_t index);
    void onDeleteFilterClicked(size_t index);
    void onFilterEnabledChanged(size_t index, bool enabled);

private:
    void setupUI();
    void addFilterWidget(size_t index, filter::GenericFilter& filter);
    void updateFilterWidget(size_t index);
    QWidget* createFilterWidget(size_t index, const filter::GenericFilter& filter);
    void removeGaps();
    void setGroupTitle(const QString& title) { m_groupTitle = title; }
    
    QGroupBox* m_groupBox;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_filtersLayout;
    QPushButton* m_addFilterButton;
    
    std::vector<filter::GenericFilter> m_filters;
    std::vector<FilterWidgetGroup> m_filterWidgets;
    QString m_groupTitle;
};