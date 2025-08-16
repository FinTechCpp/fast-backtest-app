#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include <QMap>
#include <QColor>
#include <QTableView>
#include <QStandardItemModel>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <memory>

#include "stats.hpp"

/**
 * @brief Structure représentant une stratégie de trading pour l'analyse risque/rendement
 */
struct StrategyPoint {
    QString name;           // Nom de la stratégie
    double returnPct;       // Rendement annualisé (%)
    double volatilityPct;   // Volatilité annualisée (%)
    double sharpeRatio;     // Ratio de Sharpe
    double sortinoRatio;    // Ratio de Sortino
    double maxDrawdownPct;  // Drawdown maximal (%)
    QColor color;           // Couleur pour l'affichage
    bool isCurrentStrategy; // S'agit-il de la stratégie actuellement analysée ?
    bool isReferencePoint;  // S'agit-il d'un point de référence standard (comme SPY, etc.) ?

    // Constructeur par défaut
    StrategyPoint() 
        : name("Stratégie"), returnPct(0), volatilityPct(0), sharpeRatio(0), 
        sortinoRatio(0), maxDrawdownPct(0), color(Qt::blue), 
        isCurrentStrategy(false), isReferencePoint(false) {}
    
    // Constructeur complet
    StrategyPoint(const QString& n, double ret, double vol, double sharpe, 
                double sortino, double maxDD, const QColor& c = Qt::blue, 
                bool isCurrent = false, bool isRef = false)
        : name(n), returnPct(ret), volatilityPct(vol), sharpeRatio(sharpe),
        sortinoRatio(sortino), maxDrawdownPct(maxDD), color(c),
        isCurrentStrategy(isCurrent), isReferencePoint(isRef) {}
};

/**
 * @brief Widget affichant un graphique risque/rendement pour comparer les stratégies
 * 
 * Cette classe crée une visualisation avancée qui positionne différentes stratégies
 * dans un espace risque (volatilité) / rendement, permettant de comparer visuellement
 * leur efficacité selon la théorie moderne du portefeuille.
 */
class RiskReturnMapWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construit un nouveau widget de carte risque/rendement
     * @param parent Widget parent
     */
    explicit RiskReturnMapWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Met à jour les données avec les statistiques du backtest actuel
     * @param stats Statistiques calculées du backtest
     */
    void updateData(const be::Stats& stats);
    
    /**
     * @brief Effacer toutes les données et réinitialiser l'affichage
     */
    void clear();

public slots:
    /**
     * @brief Ajouter manuellement un point de stratégie au graphique
     */
    void addCustomStrategy();
    
    /**
     * @brief Supprimer la stratégie sélectionnée du graphique
     */
    void removeSelectedStrategy();
    
    /**
     * @brief Charger des stratégies à partir d'un fichier
     */
    void loadStrategiesFromFile();
    
    /**
     * @brief Sauvegarder les stratégies actuelles dans un fichier
     */
    void saveStrategiesToFile();
    
    /**
     * @brief Changer le mode d'affichage du graphique
     * @param index Index sélectionné dans le combobox
     */
    void changeDisplayMode(int index);
    
    /**
     * @brief Ajuster les lignes de référence de ratio de Sharpe
     * @param checked État de la case à cocher
     */
    void toggleSharpeLines(bool checked);

protected:
    /**
     * @brief Gère le redimensionnement du widget
     */
    void resizeEvent(QResizeEvent* event) override;
    
    /**
     * @brief Initialise les points de référence standards
     */
    void initReferencePoints();
    
    /**
     * @brief Recrée le graphique avec les données actuelles
     */
    void buildChart();
    
    /**
     * @brief Ajoute un point au graphique
     * @param point Données de la stratégie à visualiser
     */
    void addStrategyPoint(const StrategyPoint& point);
    
    /**
     * @brief Obtient la couleur pour une stratégie selon son ratio de Sharpe
     */
    QColor getColorForSharpe(double sharpe);

private:
    // Structure principale du widget
    QGroupBox* m_groupBox;
    QVBoxLayout* m_mainLayout;
    
    // Section du graphique
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    
    // Contrôles UI
    QHBoxLayout* m_controlsLayout;
    QComboBox* m_displayModeCombo;
    QCheckBox* m_showSharpeLinesCheck;
    QPushButton* m_addStrategyBtn;
    QPushButton* m_removeStrategyBtn;
    QPushButton* m_loadStrategiesBtn;
    QPushButton* m_saveStrategiesBtn;
    
    // Tableau des stratégies
    QTableView* m_strategiesTable;
    QStandardItemModel* m_strategiesModel;
    
    // Stockage des données
    QList<StrategyPoint> m_strategies;
    StrategyPoint m_currentStrategy;
    QMap<QString, StrategyPoint> m_referencePoints;
    
    // Paramètres d'affichage
    double m_minVolatility;
    double m_maxVolatility;
    double m_minReturn;
    double m_maxReturn;
    double m_volatilityRange;
    double m_returnRange;
    
    // Dimensions du graphique
    int m_chartMarginLeft;
    int m_chartMarginRight;
    int m_chartMarginTop;
    int m_chartMarginBottom;
    int m_chartWidth;
    int m_chartHeight;
    
    // Modes d'affichage
    enum DisplayMode {
        ReturnVsVolatility,
        ReturnVsDrawdown,
        SharpeVsSortino
    };
    
    DisplayMode m_currentMode;
    bool m_showSharpeLines;
    
    // Méthodes utilitaires
    void updateStrategiesTable();
    void updateAxisLabels();
    double pixelToVolatility(int x) const;
    double pixelToReturn(int y) const;
    int volatilityToPixel(double volatility) const;
    int returnToPixel(double returnValue) const;
    void calculateChartDimensions();
    void drawSharpeLines();
    void drawAxes();
    void drawGrid();
    QColor getPointColor(const StrategyPoint& point);
    void recalculateRanges();
    void saveStrategiesData(const QString& filePath);
    void loadStrategiesData(const QString& filePath);
    void showContextMenu(const QPoint& pos);
    void exportChartImage();
};