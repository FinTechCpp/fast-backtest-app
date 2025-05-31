#include "panels/base_panel.h"
#include <QDebug>

BasePanel::BasePanel(QWidget* parent)
    : m_parent(parent)
{
    // Initialisation simple du panel de base
}

BasePanel::~BasePanel()
{
    // Les widgets enfants seront détruits automatiquement par le parent
    // Ne pas supprimer m_parent car il est géré ailleurs
    qDebug() << "Destruction d'un panel";
}

QMap<QString, QVariant> BasePanel::getValues()
{
    // Implémentation par défaut : retourne une map vide
    // Les classes dérivées doivent surcharger cette méthode pour retourner les valeurs spécifiques
    qDebug() << "BasePanel::getValues() appelé - à surcharger dans les classes dérivées";
    return QMap<QString, QVariant>();
}

void BasePanel::setValues(const QMap<QString, QVariant>& values)
{
    // Implémentation par défaut : ne fait rien
    // Les classes dérivées doivent surcharger cette méthode pour définir les valeurs spécifiques
    Q_UNUSED(values);
    qDebug() << "BasePanel::setValues() appelé - à surcharger dans les classes dérivées";
}