#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include <memory>
#include "backtest.hpp" // Inclure les définitions de be::Data et be::Stats
#include "data.hpp"
#include "stats.hpp"
#include "components/backtest_results.h" // Inclure la classe BacktestResults pour les résultats du backtest

/**
 * @brief Classe de base abstraite pour toutes les vues de résultats du backtest
 */
class BaseView : public QWidget
{
    Q_OBJECT  

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers l'objet parent
     */
    explicit BaseView(QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~BaseView() = default;
    
    /**
     * @brief Met à jour la vue avec les nouvelles données du backtest
     * @param results Pointeur vers les résultats du backtest
     */
    virtual void updateData(BacktestResults* results) = 0;
    
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

