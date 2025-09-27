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
    // Nettoyer les widgets existants
    for (const auto& widgetGroup : m_filterWidgets) {
        m_filtersLayout->removeWidget(widgetGroup.container);
        delete widgetGroup.container;
    }
    m_filterWidgets.clear();
    
    // Enregistrer les nouveaux filtres
    m_filters = filters;
    
    // Créer les widgets pour chaque filtre
    for (size_t i = 0; i < m_filters.size(); ++i) {
        addFilterWidget(i, m_filters[i]);
    }
}

const std::vector<filter::GenericFilter>& FiltersWidget::getFilters() const
{
    return m_filters;
}

QWidget* FiltersWidget::createFilterWidget(int index, const filter::GenericFilter& filter)
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
    
    // Stocker le groupe de widgets
    FilterWidgetGroup widgetGroup;
    widgetGroup.container = filterWidget;
    widgetGroup.enableCheckbox = enableCheckbox;
    widgetGroup.editButton = editButton;
    widgetGroup.deleteButton = deleteButton;
    
    // Ajouter le groupe à notre collection
    if (index >= 0 && index < static_cast<int>(m_filterWidgets.size())) {
        m_filterWidgets.insert(m_filterWidgets.begin() + index, widgetGroup);
    } else {
        m_filterWidgets.push_back(widgetGroup);
    }
    
    return filterWidget;
}

void FiltersWidget::addFilterWidget(int index, const filter::GenericFilter& filter)
{
    QWidget* widget = createFilterWidget(index, filter);
    m_filtersLayout->insertWidget(index, widget);
}

void FiltersWidget::updateFilterWidget(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size()) || 
        index >= static_cast<int>(m_filterWidgets.size())) {
        return;
    }
    
    const filter::GenericFilter& filter = m_filters[index];
    FilterWidgetGroup& widgetGroup = m_filterWidgets[index];
    
    // Mettre à jour l'état de la checkbox
    widgetGroup.enableCheckbox->setChecked(filter.enabled);
    
    // Mettre à jour le texte et le style du bouton d'édition
    widgetGroup.editButton->setText(QString::fromStdString(filter.description));
    
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
    
    // Pas besoin de mettre à jour le bouton de suppression
}

void FiltersWidget::removeGaps()
{
    // Cette fonction réajuste les indices des connexions lambda après une suppression
    for (size_t i = 0; i < m_filterWidgets.size(); ++i) {
        FilterWidgetGroup& widgetGroup = m_filterWidgets[i];
        
        // Déconnecter les anciens signaux
        disconnect(widgetGroup.enableCheckbox, nullptr, this, nullptr);
        disconnect(widgetGroup.editButton, nullptr, this, nullptr);
        disconnect(widgetGroup.deleteButton, nullptr, this, nullptr);
        
        // Reconnecter avec les nouveaux indices
        int index = static_cast<int>(i);
        
        connect(widgetGroup.enableCheckbox, &QCheckBox::toggled, this, 
            [this, index](bool checked) { onFilterEnabledChanged(index, checked); });
        
        connect(widgetGroup.editButton, &QPushButton::clicked, this, 
            [this, index]() { onEditFilterClicked(index); });
        
        connect(widgetGroup.deleteButton, &QPushButton::clicked, this, 
            [this, index]() { onDeleteFilterClicked(index); });
    }
}

void FiltersWidget::onAddFilterClicked()
{
    FilterEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Récupérer le nouveau filtre et l'ajouter
        filter::GenericFilter newFilter = dialog.getFilter();
        int newIndex = static_cast<int>(m_filters.size());
        m_filters.push_back(newFilter);
        
        // Ajouter uniquement le nouveau widget
        addFilterWidget(newIndex, newFilter);
        
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
        
        // Mettre à jour uniquement le widget concerné
        updateFilterWidget(index);
        
        emit filtersChanged();
    }
}

void FiltersWidget::onDeleteFilterClicked(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size()) || 
        index >= static_cast<int>(m_filterWidgets.size())) {
        return;
    }

    // Supprimer le filtre des données
    m_filters.erase(m_filters.begin() + index);
    
    // Supprimer le widget correspondant
    QWidget* widget = m_filterWidgets[index].container;
    m_filtersLayout->removeWidget(widget);
    delete widget;
    m_filterWidgets.erase(m_filterWidgets.begin() + index);
    
    // Réajuster les indices des connexions
    removeGaps();
    
    emit filtersChanged();
}

void FiltersWidget::onFilterEnabledChanged(int index, bool enabled)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size())) return;
    
    // Mettre à jour l'état du filtre
    m_filters[index].enabled = enabled;
    m_filters[index].description = m_filters[index].autoGenerateDescription();
    
    // Mettre à jour uniquement le style du bouton d'édition
    updateFilterWidget(index);
    
    emit filtersChanged();
}