#include "baseview.h"
#include <QDebug>

BaseView::BaseView(QObject* parent)  
    : QObject(parent), m_parentWidget(nullptr)
{
    qDebug() << "BaseView créée avec parent:" << parent;
}

BaseView::~BaseView()
{
    qDebug() << "Destruction d'une vue";
}

void BaseView::clear()
{
    qDebug() << "BaseView::clear() appelé - à surcharger dans les classes dérivées";
}

void BaseView::clearLayout(QLayout* layout)
{
    if (!layout)
        return;
        
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