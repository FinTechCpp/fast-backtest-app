#ifndef RESULT_MANAGER_H
#define RESULT_MANAGER_H

#include <QObject>
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QString>
#include "baseview.h"

// Forward declarations
class StatsView;
class ChartView;
class HistogramView;

/**
 * @brief Gestionnaire des vues de résultats du backtest
 * 
 * Cette classe coordonne l'affichage des différentes vues de résultats:
 * - Statistiques (stats_view)
 * - Histogramme des profits/pertes (histogram_view)
 * - Graphique financier (financechart)
 */
class ResultManager : public QObject, public BaseView
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur - CORRIGER: QObject* au lieu de QWidget*
     * @param parent Pointeur vers l'objet parent
     */
    explicit ResultManager(QObject* parent = nullptr);
    
    /**
     * @brief Destructeur
     */
    ~ResultManager();
    
    /**
     * @brief Crée la zone d'affichage des résultats avec les différentes vues
     * @return Widget contenant l'ensemble des vues de résultats
     */
    QWidget* create() override;
    
    /**
     * @brief Met à jour toutes les vues avec les résultats du backtest
     * @param data Données utilisées pour le backtest
     * @param stats Statistiques résultantes du backtest
     */
    void update(void* data, void* stats) override;
    
    /**
     * @brief Efface les données des vues
     */
    void clear() override;
    
    /**
     * @brief Met à jour toutes les vues avec les résultats du backtest (méthode spécifique)
     * @param data Données utilisées pour le backtest
     * @param stats Statistiques résultantes du backtest
     */
    void updateAll(void* data, void* stats);
    
    /**
     * @brief Définit l'onglet actif
     * @param index Index de l'onglet à activer
     */
    void setCurrentTab(int index);
    
    /**
     * @brief Retourne le widget d'onglets
     * @return QTabWidget contenant les différentes vues
     */
    QTabWidget* getTabWidget() const;

private:
    // Widgets UI
    QWidget* m_resultsWidget;        // Widget principal des résultats
    QVBoxLayout* m_resultsLayout;    // Layout principal
    QTabWidget* m_tabWidget;         // Widget d'onglets
    
    // Vues - CORRECTION: Ordre des déclarations pour éviter l'avertissement
    StatsView* m_statsView;          // Vue des statistiques
    ChartView* m_chartView;          // Vue du graphique financier
    HistogramView* m_histogramView;  // Vue de l'histogramme des profits/pertes
    
    // Map pour accès facile aux vues
    QMap<QString, BaseView*> m_views;
};

#endif // RESULT_MANAGER_H