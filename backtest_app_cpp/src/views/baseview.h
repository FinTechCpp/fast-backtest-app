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
    explicit BaseView(QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BaseView() = default;
    
    /**
     * @brief Met à jour la vue avec les nouvelles données du backtest
     * @param data Pointeur vers les données utilisées pour le backtest
     * @param stats Pointeur vers les statistiques résultantes
     */
    virtual void updateData(void* data, void* stats) = 0;
    
    /**
     * @brief Réinitialise la vue à son état initial
     */
    virtual void clear() = 0;

    protected:
    /** Dictionnaire des widgets de la vue */
    QMap<QString, QWidget*> m_widgets;
    
    /** Layout principal de la vue */
    QVBoxLayout* m_mainLayout;
    
    /**
     * @brief Utilitaire pour vider complètement un layout
     * @param layout Layout à vider
     */
    void clearLayout(QLayout* layout);
    
    /**
     * @brief Méthode virtuelle pour construire l'interface
     * À implémenter dans les classes filles
     */
    virtual void setupUI() = 0;
};

#endif // BASEVIEW_H