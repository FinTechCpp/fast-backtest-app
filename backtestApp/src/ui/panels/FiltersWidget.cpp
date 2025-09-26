#include "ui/panels/FiltersWidget.h"
#include "ui/dialogs/FilterEditDialog.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

FiltersWidget::FiltersWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void FiltersWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Création du groupe contenant les filtres
    m_groupBox = new QGroupBox("Filtres de stratégie", this);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    
    // Bouton d'ajout de filtre
    m_addFilterButton = new QPushButton("+ Ajouter un filtre", this);
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
    
    // Layout pour contenir les widgets des filtres
    m_filtersLayout = new QVBoxLayout();
    m_filtersLayout->setSpacing(8);
    groupLayout->addLayout(m_filtersLayout);
    
    // Ajouter un stretch pour pousser tout vers le haut
    groupLayout->addStretch();
    
    m_mainLayout->addWidget(m_groupBox);
}

void FiltersWidget::setFilters(const std::vector<filter::GenericFilter>& filters)
{
    m_filters = filters;
    updateFilterDisplay();
}

const std::vector<filter::GenericFilter>& FiltersWidget::getFilters() const
{
    return m_filters;
}

void FiltersWidget::updateFilterDisplay()
{
    clearFilterWidgets();
    
    // Créer un widget pour chaque filtre
    for (size_t i = 0; i < m_filters.size(); ++i) {
        createFilterWidgets(i, m_filters[i]);
    }
}

void FiltersWidget::clearFilterWidgets()
{
    // Nettoyer tous les widgets de filtres existants
    for (QWidget* widget : m_filterWidgets) {
        m_filtersLayout->removeWidget(widget);
        delete widget;
    }
    m_filterWidgets.clear();
}

void FiltersWidget::createFilterWidgets(int index, const filter::GenericFilter& filter)
{
    // Créer un widget conteneur pour ce filtre
    QWidget* filterWidget = new QWidget(this);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    
    // Checkbox pour activer/désactiver le filtre
    QCheckBox* enableCheckbox = new QCheckBox(filterWidget);
    enableCheckbox->setChecked(filter.enabled);
    connect(enableCheckbox, &QCheckBox::toggled, this, [this, index](bool checked) {
        onFilterEnabledChanged(index, checked);
    });
    filterLayout->addWidget(enableCheckbox);
    
    // Bouton de modification avec la description du filtre
    QPushButton* editButton = new QPushButton(QString::fromStdString(filter.description), filterWidget);
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
        "}"
    );
    if (!filter.enabled) {
        editButton->setStyleSheet(editButton->styleSheet() + "QPushButton { color: #888; }");
    }
    connect(editButton, &QPushButton::clicked, this, [this, index]() {
        onEditFilterClicked(index);
    });
    filterLayout->addWidget(editButton, 1); // Stretch factor 1 pour prendre l'espace disponible
    
    // Bouton de suppression
    QPushButton* deleteButton = new QPushButton("×", filterWidget);
    deleteButton->setFixedSize(24, 24);
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
    
    // Ajouter le widget au layout principal
    m_filtersLayout->addWidget(filterWidget);
    m_filterWidgets.push_back(filterWidget);
}

void FiltersWidget::onAddFilterClicked()
{
    FilterEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Récupérer le nouveau filtre et l'ajouter
        filter::GenericFilter newFilter = dialog.getFilter();
        m_filters.push_back(newFilter);
        updateFilterDisplay();
        emit filtersChanged();
    }
}

void FiltersWidget::onEditFilterClicked(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size())) return;
    
    FilterEditDialog dialog(this);
    dialog.setFilter(m_filters[index]);
    if (dialog.exec() == QDialog::Accepted) {
        // Mettre à jour le filtre
        m_filters[index] = dialog.getFilter();
        updateFilterDisplay();
        emit filtersChanged();
    }
}

void FiltersWidget::onDeleteFilterClicked(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size())) return;

    // Supprimer le filtre
    m_filters.erase(m_filters.begin() + index);
    updateFilterDisplay();
    emit filtersChanged();
}

void FiltersWidget::onFilterEnabledChanged(int index, bool enabled)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size())) return;
    
    m_filters[index].enabled = enabled;
    updateFilterDisplay();
    emit filtersChanged();
}