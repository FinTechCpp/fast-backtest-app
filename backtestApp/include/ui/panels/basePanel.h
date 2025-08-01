#pragma once

#include <QGroupBox>
#include <QMap>
#include <QVariant>
#include "ui/panels/PropertyBinderFactory.h"


/**
 * @brief Classe de base pour tous les panels de l'application
 */
class BasePanel : public QGroupBox
{
    Q_OBJECT
    
public:
    /**
     * @brief Constructeur
     * @param title Titre du groupe
     * @param parent Pointeur vers le widget parent
     */
    BasePanel(const QString& title, QWidget* parent = nullptr)
        : QGroupBox(title, parent) {

    }
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BasePanel() = default;
    

protected:
    /** Map des widgets contenant les contrôles du panel */
    QMap<QString, QWidget*> m_widgets;

    template<typename T>
    T convertToConfig(const QMap<QString, QVariant>& values);
};
