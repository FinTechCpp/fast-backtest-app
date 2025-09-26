#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <vector>
#include "common.h"  // Pour accéder à GenericFilter

class FiltersWidget : public QWidget
{
    Q_OBJECT

public:
    FiltersWidget(QWidget* parent = nullptr);
    
    // Méthodes pour accéder aux filtres
    void setFilters(const std::vector<GenericFilter>& filters);
    const std::vector<GenericFilter>& getFilters() const;
    
signals:
    void filtersChanged();
    
private slots:
    void onAddFilterClicked();
    void onEditFilterClicked(int index);
    void onDeleteFilterClicked(int index);
    void onFilterEnabledChanged(int index, bool enabled);
    
private:
    void setupUI();
    void updateFilterDisplay();
    void createFilterWidgets(int index, const GenericFilter& filter);
    void clearFilterWidgets();
    
    QGroupBox* m_groupBox;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_filtersLayout;
    QPushButton* m_addFilterButton;
    
    std::vector<GenericFilter> m_filters;
    std::vector<QWidget*> m_filterWidgets; // Pour faciliter le nettoyage
};