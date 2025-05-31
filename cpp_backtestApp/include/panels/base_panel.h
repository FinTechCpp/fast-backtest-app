#pragma once

#include <QGroupBox>
#include <QMap>
#include <QVariant>

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
    BasePanel(const QString& title, QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BasePanel();
    
    /**
     * @brief Initialise le contenu du panel
     * Cette méthode doit être appelée après la construction pour configurer l'UI
     */
    virtual void initialize() = 0;
    
    /**
     * @brief Récupère les valeurs des widgets du panel
     */
    virtual QMap<QString, QVariant> getValues();
    
    /**
     * @brief Définit les valeurs des widgets du panel
     */
    virtual void setValues(const QMap<QString, QVariant>& values);

protected:
    /** Map des widgets contenant les contrôles du panel */
    QMap<QString, QWidget*> m_widgets;
};
