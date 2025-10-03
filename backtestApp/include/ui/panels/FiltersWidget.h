#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QMap>
#include <vector>
#include "common.h"  // Pour accéder à GenericFilter

// Structure pour regrouper les widgets d'un filtre
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
    FiltersWidget(QWidget* parent = nullptr);
    
    // Méthodes pour accéder aux filtres
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
    
    QGroupBox* m_groupBox;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_filtersLayout;
    QPushButton* m_addFilterButton;
    
    std::vector<filter::GenericFilter> m_filters;
    std::vector<FilterWidgetGroup> m_filterWidgets;
};