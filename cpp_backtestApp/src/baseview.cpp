#include "baseview.h"
#include <QDebug>

BaseView::BaseView(QWidget *parent) : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
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