#pragma once

#include <QGroupBox>
#include <QMap>
#include <QVariant>
#include <QWidget>

/**
 * @brief Classe de base abstraite pour tous les panels de l'application
 * 
 * Cette classe définit l'interface commune à tous les panels.
 * Chaque panel spécifique doit hériter de cette classe et implémenter 
 * la méthode create() qui retourne un widget contenant l'interface du panel.
 */
class BasePanel
{
public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    BasePanel(QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BasePanel();
    
    /**
     * @brief Crée et retourne l'interface graphique du panel
     * @return QGroupBox contenant les widgets du panel
     */
    virtual QGroupBox* create() = 0;
    
    /**
     * @brief Récupère les valeurs des widgets du panel
     * @return Map contenant les valeurs sous forme de QVariant
     */
    virtual QMap<QString, QVariant> getValues();
    
    /**
     * @brief Définit les valeurs des widgets du panel
     * @param values Map contenant les valeurs à affecter aux widgets
     */
    virtual void setValues(const QMap<QString, QVariant>& values);

protected:
    /** Parent widget */
    QWidget* m_parent;
    
    /** Map des widgets contenant les contrôles du panel */
    QMap<QString, QWidget*> m_widgets;
};
