#include "ui/panels/FiltersWidget.h"
#include "ui/dialogs/FilterEditDialog.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMenu> // Added for context menu

FiltersWidget::FiltersWidget(QWidget* parent, const QString& groupTitle)
    : QWidget(parent)
{
    m_groupTitle = groupTitle;
    setupUI();
}

void FiltersWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create the group containing the filters
    m_groupBox = new QGroupBox(m_groupTitle.isEmpty() ? QStringLiteral("Strategy Filters") : m_groupTitle, this);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    
    // Add filter button
    m_addFilterButton = new QPushButton("+ Add Filter", this);
    m_addFilterButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  padding: 6px 12px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );
    connect(m_addFilterButton, &QPushButton::clicked, this, &FiltersWidget::onAddFilterClicked);
    groupLayout->addWidget(m_addFilterButton);
    
    // Layout to contain the filter widgets
    m_filtersLayout = new QVBoxLayout();
    m_filtersLayout->setSpacing(8);
    groupLayout->addLayout(m_filtersLayout);
    
    // Add a stretch to push everything to the top
    groupLayout->addStretch();
    
    m_mainLayout->addWidget(m_groupBox);
}

void FiltersWidget::addFilter(const filter::GenericFilter& filter)
{
    m_filters.push_back(filter);
    // Rebuild UI to ensure indices are correct
    setFilters(m_filters);
    emit filtersChanged();
}

void FiltersWidget::removeFilter(size_t index)
{
    if (index < m_filters.size()) {
        m_filters.erase(m_filters.begin() + index);
        // Rebuild UI to ensure indices are correct
        setFilters(m_filters);
        emit filtersChanged();
    }
}

void FiltersWidget::setFilters(const std::vector<filter::GenericFilter>& filters)
{
    // Clear existing widgets
    for (const auto& widgetGroup : m_filterWidgets) {
        m_filtersLayout->removeWidget(widgetGroup.container);
        delete widgetGroup.container;
    }
    m_filterWidgets.clear();
    
    // Store new filters
    m_filters = filters;
    
    // Create widgets for each filter
    for (size_t i = 0; i < m_filters.size(); ++i) {
        addFilterWidget(i, m_filters[i]);
    }
}

const std::vector<filter::GenericFilter>& FiltersWidget::getFilters() const
{
    return m_filters;
}

QWidget* FiltersWidget::createFilterWidget(size_t index, const filter::GenericFilter& filter)
{
    // Create a container widget for this filter
    QWidget* filterWidget = new QWidget(this);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    
    // Checkbox to enable/disable the filter
    QCheckBox* enableCheckbox = new QCheckBox(filterWidget);
    enableCheckbox->setChecked(filter.enabled);
    connect(enableCheckbox, &QCheckBox::toggled, this, [this, index](bool checked) {
        onFilterEnabledChanged(index, checked);
    });
    filterLayout->addWidget(enableCheckbox);
    
    // Edit button with the filter description
    std::string buttonText = filter.description;
    if (filter.offset > 0.0) buttonText += " | offset=" + std::to_string(filter.offset);
    QPushButton* editButton = new QPushButton(QString::fromStdString(buttonText), filterWidget);
    editButton->setStyleSheet(
        "QPushButton {"
        "  text-align: left;"
        "  padding: 5px 8px;"
        "  background-color: #f0f0f0;"
        "  border: 1px solid #ddd;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e0e0e0;"
        "  border-color: #bbb;"
        "}"
    );
    
    // Enable context menu
    editButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editButton, &QPushButton::customContextMenuRequested, this, &FiltersWidget::onCustomContextMenuRequested);
    
    connect(editButton, &QPushButton::clicked, this, [this, index]() {
        onEditFilterClicked(index);
    });
    filterLayout->addWidget(editButton, 1); // Stretch factor 1
    
    // Delete button
    QPushButton* deleteButton = new QPushButton("X", filterWidget);
    deleteButton->setFixedSize(18, 18);
    deleteButton->setStyleSheet(
        "QPushButton {"
        "  color: #fff;"
        "  background-color: #f44336;"
        "  border: none;"
        "  border-radius: 12px;"
        "  font-weight: bold;"
        "  font-size: 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #d32f2f;"
        "}"
    );
    connect(deleteButton, &QPushButton::clicked, this, [this, index]() {
        onDeleteFilterClicked(index);
    });
    filterLayout->addWidget(deleteButton);
    
    // Store the widget group
    FilterWidgetGroup widgetGroup;
    widgetGroup.container = filterWidget;
    widgetGroup.enableCheckbox = enableCheckbox;
    widgetGroup.editButton = editButton;
    widgetGroup.deleteButton = deleteButton;
    
    // Add the group to our collection
    if (index >= 0 && index < static_cast<int>(m_filterWidgets.size())) {
        m_filterWidgets.insert(m_filterWidgets.begin() + index, widgetGroup);
    } else {
        m_filterWidgets.push_back(widgetGroup);
    }
    
    return filterWidget;
}

void FiltersWidget::addFilterWidget(size_t index, filter::GenericFilter& filter)
{
    filter.description = filter.autoGenerateDescription();
    QWidget* widget = createFilterWidget(index, filter);
    m_filtersLayout->insertWidget(static_cast<int>(index), widget);
}

void FiltersWidget::updateFilterWidget(size_t index)
{
    if (index >= m_filters.size() || index >= m_filterWidgets.size())
        return;
    
    const filter::GenericFilter& filter = m_filters[index];
    FilterWidgetGroup& widgetGroup = m_filterWidgets[index];
    
    // Update checkbox state
    widgetGroup.enableCheckbox->setChecked(filter.enabled);
    
    // Update the text and style of the edit button
    std::string buttonText = filter.description;
    if (filter.offset > 0.0) buttonText += " | offset=" + std::to_string(filter.offset);
    widgetGroup.editButton->setText(QString::fromStdString(buttonText));
    
    QString baseStyle = 
        "QPushButton {"
        "  text-align: left;"
        "  padding: 5px 8px;"
        "  background-color: #f0f0f0;"
        "  border: 1px solid #ddd;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e0e0e0;"
        "}";
    
    if (!filter.enabled) {
        widgetGroup.editButton->setStyleSheet(baseStyle + "QPushButton { color: #888; }");
    } else {
        widgetGroup.editButton->setStyleSheet(baseStyle);
    }
    
    // No need to update the delete button
}

void FiltersWidget::onCustomContextMenuRequested(const QPoint& pos)
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    // Find index
    int index = -1;
    for (size_t i = 0; i < m_filterWidgets.size(); ++i) {
        if (m_filterWidgets[i].editButton == button) {
            index = static_cast<int>(i);
            break;
        }
    }
    if (index == -1) return;
    
    QMenu menu(this);
    
    // Duplicate action
    menu.addAction(QStringLiteral("Duplicate"), [this, index]() {
        if (index >= 0 && index < static_cast<int>(m_filters.size())) {
            filter::GenericFilter copy = m_filters[index];
            m_filters.insert(m_filters.begin() + index + 1, copy);
            setFilters(m_filters);
            emit filtersChanged();
        }
    });
    
    // Move to menu
    QMenu* moveMenu = menu.addMenu(QStringLiteral("Move to..."));
    const QStringList categories = {
        QStringLiteral("Buy Filters"), 
        QStringLiteral("Sell Filters"), 
        QStringLiteral("Resale Filters"), 
        QStringLiteral("Rebuy Filters")
    };
    
    for (const auto& cat : categories) {
        if (cat != m_groupTitle) {
            moveMenu->addAction(cat, [this, index, cat]() {
                emit filterMoveRequested(static_cast<size_t>(index), cat);
            });
        }
    }
    
    menu.exec(button->mapToGlobal(pos));
}

void FiltersWidget::removeGaps()
{
    // This function readjusts the indices of lambda connections after a removal
    for (size_t i = 0; i < m_filterWidgets.size(); ++i) {
        FilterWidgetGroup& widgetGroup = m_filterWidgets[i];
        
        // Disconnect old signals
        disconnect(widgetGroup.enableCheckbox, nullptr, this, nullptr);
        disconnect(widgetGroup.editButton, nullptr, this, nullptr);
        disconnect(widgetGroup.deleteButton, nullptr, this, nullptr);
        
        // Reconnect with the new indices
        size_t index = i;
        
        connect(widgetGroup.enableCheckbox, &QCheckBox::toggled, this, 
            [this, index](bool checked) { onFilterEnabledChanged(index, checked); });
        
        connect(widgetGroup.editButton, &QPushButton::clicked, this, 
            [this, index]() { onEditFilterClicked(index); });
        
        // Reconnect context menu signal
        connect(widgetGroup.editButton, &QPushButton::customContextMenuRequested, this, 
            &FiltersWidget::onCustomContextMenuRequested);
        
        connect(widgetGroup.deleteButton, &QPushButton::clicked, this, 
            [this, index]() { onDeleteFilterClicked(index); });
    }
}

void FiltersWidget::onAddFilterClicked()
{
    FilterEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Retrieve the new filter and add it
        addFilter(dialog.getFilter());
    }
}

void FiltersWidget::onEditFilterClicked(size_t index)
{
    if (index >= m_filters.size()) return;

    FilterEditDialog dialog(this);
    dialog.setFilter(m_filters[index]);
    if (dialog.exec() == QDialog::Accepted) {
        // Update the filter
        m_filters[index] = dialog.getFilter();
        
        // Update only the affected widget
        updateFilterWidget(index);
        
        emit filtersChanged();
    }
}

void FiltersWidget::onDeleteFilterClicked(size_t index)
{
    removeFilter(index);
}

void FiltersWidget::onFilterEnabledChanged(size_t index, bool enabled)
{
    if (index >= m_filters.size()) return;

    // Update the filter's enabled state
    m_filters[index].enabled = enabled;
    m_filters[index].description = m_filters[index].autoGenerateDescription();
    
    // Update only the edit button's style
    updateFilterWidget(index);
    
    emit filtersChanged();
}