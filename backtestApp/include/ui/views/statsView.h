#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QHeaderView>
#include <QVariant>
#include <memory>
#include "ui/views/baseView.h"
#include "ui/metricWidget.h"
#include <cmath>

class App;

/**
 * @brief Définition d'une métrique qui sera affichée dans la vue des statistiques.
 * Contient l'identifiant, le libellé, l'infobulle, la section et les fonctions
 * d'évaluation et de formatage de la valeur.
 */
struct MetricDefinition {
    QString key;              // Identifiant unique
    QString label;            // Texte affiché
    QString tooltip;          // Info-bulle
    QString section;          // Section ("time", "performance", "risk", "general")
    
    // Fonction pour déterminer le statut (Good/Bad/Neutral) selon la valeur
    std::function<MetricStatus(const be::Stats&)> getStatus;
    
    // Fonction pour formatter la valeur
    std::function<QString(const be::Stats&)> formatValue;
};

/**
 * @brief Modèle de données pour la table des trades.
 * Gère l'affichage des transactions et leur formatage.
 */
class TradesTableModel : public QStandardItemModel {
    Q_OBJECT

public:
    /**
     * @brief Constructeur du modèle de table des trades
     * @param parent Le parent QObject
     */
    TradesTableModel(QObject* parent = nullptr);
    
    /**
     * @brief Met à jour les données du modèle avec un vecteur de trades
     * @param trades Les trades à afficher
     */
    void updateData(const std::vector<std::shared_ptr<be::Trade>>& trades);
    
    /**
     * @brief Efface toutes les lignes du modèle
     */
    void clear();
    
    // Méthodes utilitaires statiques
    static QString formatNumber(double value, int precision = 2);
    static QDateTime dateToQDateTime(const be::Date& date);
    static QString formatDateTime(const QDateTime& dateTime);
    
private:
    static QString formatDuration(const QString& duration);
};

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
    // ==================== Initialisation ====================
    
    /**
     * @brief Initialise les définitions de métriques
     */
    void initializeMetricDefinitions();
    
    /**
     * @brief Crée le scroll area et le conteneur principal
     */
    void createScrollAreaAndContent();
    
    /**
     * @brief Crée les groupes de métriques
     */
    void createGroupBoxes();
    
    /**
     * @brief Crée les widgets de métriques à partir des définitions
     */
    void createStatsWidgets();
    
    /**
     * @brief Crée la légende pour les couleurs
     */
    void createLegend();
    
    /**
     * @brief Arrange les panneaux en grille 2x2
     */
    void arrangePanels();

    // ==================== Création et gestion des tables ====================
    
    /**
     * @brief Crée la table des trades
     */
    void createTradesTable();
    
    /**
     * @brief Crée les contrôles pour la table des trades
     */
    void createTradesTableControls();
    
    /**
     * @brief Crée la vue de table des trades
     */
    void createTradesTableView();
    
    /**
     * @brief Configure les connexions pour la table des trades
     */
    void setupTradesConnections();
    
    /**
     * @brief Crée un widget métrique et l'ajoute au layout
     */
    MetricWidget* createMetricWidget(const QString& key, const QString& label, 
                                    const QString& value, QHBoxLayout* layout);

    // ==================== Mise à jour des données ====================
    
    /**
     * @brief Remplit les métriques avec les valeurs de stats
     */
    void populateMetrics(const be::Stats& stats);
    
    /**
     * @brief Remplit la table des trades
     */
    void populateTrades(const std::vector<std::shared_ptr<be::Trade>>& trades);
    
    /**
     * @brief Filtre les trades selon les paramètres actuels
     */
    std::vector<std::shared_ptr<be::Trade>> getFilteredTrades(
        const std::vector<std::shared_ptr<be::Trade>>& allTrades);

    // ==================== Membres privés ====================
    
    // --- Modèles de données ---
    TradesTableModel* m_tradesModel;
    TradesTableModel* m_equityModel;
    std::vector<MetricDefinition> m_metricDefinitions;
    QMap<QString, MetricWidget*> m_metricWidgets;
    
    // --- UI: Conteneurs principaux ---
    QScrollArea* m_scrollStats;
    QWidget* m_statsContent;
    QVBoxLayout* m_statsContentLayout;
    QGridLayout* m_statsGridLayout;
    QLabel* m_statsPlaceholder;
    
    // --- UI: Groupes de métriques ---
    QGroupBox* m_timeGroup;
    QVBoxLayout* m_timeLayout;
    QGroupBox* m_performanceGroup;
    QVBoxLayout* m_performanceLayout;
    QGroupBox* m_riskGroup;
    QVBoxLayout* m_riskLayout;
    QGroupBox* m_generalGroup;
    QVBoxLayout* m_generalLayout;
    
    // --- UI: Section trades ---
    QGroupBox* m_tradesGroup;
    QVBoxLayout* m_tradesLayout;
    QTableView* m_tradesTable;
    QComboBox* m_tradesLimitCombo;
    QPushButton* m_showAllTradesBtn;

    // --- État ---
    bool m_tablesCreated;
    BacktestResults* m_currentResults;
    App* m_app;  // Référence à l'application principale
};