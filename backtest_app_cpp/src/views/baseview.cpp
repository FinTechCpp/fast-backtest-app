#include "baseview.h"
#include <QDebug>

BaseView::BaseView(QWidget* parent)
    : m_parent(parent)
{
    // Initialisation simple de la vue de base
}

BaseView::~BaseView()
{
    // Les widgets enfants seront détruits automatiquement par le parent
    // Ne pas supprimer m_parent car il est géré ailleurs
    qDebug() << "Destruction d'une vue";
}

void BaseView::clear()
{
    // Implémentation par défaut : ne fait rien
    // Les classes dérivées doivent surcharger cette méthode pour nettoyer leurs ressources
    qDebug() << "BaseView::clear() appelé - à surcharger dans les classes dérivées";
}

void BaseView::clearLayout(QLayout* layout)
{
    if (!layout)
        return;
        
    // Supprimer tous les éléments du layout
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        } else if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}