#ifndef BASEVIEW_H
#define BASEVIEW_H

#include <QWidget>  // Changé de QObject
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include <memory>

/**
 * @brief Classe de base abstraite pour toutes les vues de résultats du backtest
 */
class BaseView : public QWidget  // Changé de QObject à QWidget
{
    Q_OBJECT  

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'objet parent (maintenant un QWidget)
     */
    explicit BaseView(QWidget* parent = nullptr);  // Changé de QObject* à QWidget*
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BaseView();
    
    /**
     * @brief Crée et retourne le widget principal de la vue
     * @param parentWidget Widget parent pour les widgets créés
     * @return Widget contenant l'interface de la vue
     */
    virtual QWidget* create(QWidget* parentWidget = nullptr) = 0;
    
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
    /** Dictionnaire des widgets de la vue */
    QMap<QString, QWidget*> m_widgets;
    
    /** Widget parent pour la création des widgets enfants */
    QWidget* m_parentWidget;
    
    /**
     * @brief Utilitaire pour vider complètement un layout
     * @param layout Layout à vider
     */
    void clearLayout(QLayout* layout);
};

#endif // BASEVIEW_H