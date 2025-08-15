#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QHeaderView>
#include <QChartView>
#include <QVariant>
#include <memory>
#include "ui/views/baseView.h"
#include "ui/metricWidget.h"
#include "ui/views/Stats/TimeLineWidget.h"
#include "ui/views/Stats/TradeClosureWidget.h"
#include "ui/views/Stats/EquityCurveWidget.h"
#include "ui/views/Stats/TradesTableModel.h"
#include "ui/views/Stats/MetricsContainerWidget.h"

#include <cmath>

class App;

/**
 * @brief Définition d'une métrique qui sera affichée dans la vue des statistiques.
 * Contient l'identifiant, le libellé, l'infobulle, la section et les fonctions
 * d'évaluation et de formatage de la valeur.
 */
// struct MetricDefinition {
//     QString key;              // Identifiant unique
//     QString label;            // Texte affiché
//     QString tooltip;          // Info-bulle
//     QString section;          // Section ("time", "performance", "risk", "general")
    
//     // Fonction pour déterminer le statut (Good/Bad/Neutral) selon la valeur
//     std::function<MetricStatus(const be::Stats&)> getStatus;
    
//     // Fonction pour formatter la valeur
//     std::function<QString(const be::Stats&)> formatValue;
// };



/**
 * @brief Vue pour afficher les statistiques de backtest et les trades.
 * Organise les métriques en catégories et permet de visualiser les transactions.
 */
class StatsView : public BaseView {
    Q_OBJECT

public:
    /**
     * @brief Constructeur de la vue des statistiques
     * @param parent Le parent widget
     */
    StatsView(QWidget* parent = nullptr);
    
    /**
     * @brief Destructeur
     */
    ~StatsView();

    /**
     * @brief Met à jour l'affichage avec de nouveaux résultats de backtest
     * @param results Les résultats à afficher
     */
    void updateData(BacktestResults* results) override;
    
    /**
     * @brief Efface toutes les données et réinitialise l'affichage
     */
    void clear() override;

protected:
    /**
     * @brief Configure l'interface utilisateur
     */
    void setupUI() override;

private slots:
    /**
     * @brief Rafraîchit la table des trades avec les filtres actuels
     */
    void refreshTradesTable();

private:
    // ==================== Mise à jour des données ====================
    
    /**
     * @brief Remplit la table des trades
     */
    void populateTrades(const std::vector<be::TradeData>& trades);

    /**
     * @brief Filtre les trades selon les paramètres actuels
     */
    std::vector<be::TradeData> getFilteredTrades(const std::vector<be::TradeData>& allTrades);

    // ==================== Membres privés ====================
    // Graphiques
    // Trade closure
    TradeClosureWidget* m_tradeClosureWidget = nullptr;

    // Equity curve
    EquityCurveWidget* m_equityCurveWidget = nullptr;

    // Timeline
    TimelineWidget* m_timelineWidget = nullptr;

    // Modèle de données pour les trades
    MetricsContainerWidget* m_metricsWidget = nullptr;


    QWidget* m_legendWidget = nullptr;

    // --- Modèles de données ---
    TradesTableModel* m_tradesModel;
    TradesTableModel* m_equityModel;
    
    // --- UI: Conteneurs principaux ---
    QScrollArea* m_scrollStats;
    QWidget* m_statsContent;
    QVBoxLayout* m_statsContentLayout;
    QGridLayout* m_statsGridLayout;
    QLabel* m_statsPlaceholder;
    
    // --- UI: Section trades ---
    QGroupBox* m_tradesGroup;
    QVBoxLayout* m_tradesLayout;
    QTableView* m_tradesTable;
    QComboBox* m_tradesLimitCombo;
    QPushButton* m_showAllTradesBtn;

    // --- État ---
    App* m_app;  // Référence à l'application principale
};