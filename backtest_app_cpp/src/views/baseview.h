#ifndef BASEVIEW_H
#define BASEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>  // AJOUT MANQUANT
#include <QString>  // AJOUT MANQUANT
#include <memory>

/**
 * @brief Classe de base abstraite pour toutes les vues de résultats du backtest
 * 
 * Cette classe définit l'interface commune à toutes les vues de résultats.
 * Chaque vue spécifique doit hériter de cette classe et implémenter 
 * les méthodes abstraites create() et update().
 */
class BaseView
{
public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    BaseView(QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BaseView();
    
    /**
     * @brief Crée et retourne le widget principal de la vue
     * @return Widget contenant l'interface de la vue
     */
    virtual QWidget* create() = 0;
    
    /**
     * @brief Met à jour la vue avec les nouvelles données du backtest
     * @param data Pointeur vers les données utilisées pour le backtest
     * @param stats Pointeur vers les statistiques résultantes
     */
    virtual void update(void* data, void* stats) = 0;
    
    /**
     * @brief Réinitialise la vue à son état initial
     */
    virtual void clear();

protected:
    /** Parent widget */
    QWidget* m_parent;
    
    /** Dictionnaire des widgets de la vue */
    QMap<QString, QWidget*> m_widgets;  // Maintenant défini correctement
    
    /**
     * @brief Utilitaire pour vider complètement un layout
     * @param layout Layout à vider
     */
    void clearLayout(QLayout* layout);
};

#endif // BASEVIEW_H