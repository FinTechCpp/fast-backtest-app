#pragma once

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QColor>
#include <QEvent>
#include <QResizeEvent>
#include <QMap>

#include "ui/views/Stats/StatsBaseWidget.h"

class TradingHeatmapWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit TradingHeatmapWidget(QWidget* parent = nullptr);
    
    void updateContent(const be::Stats& stats) override;
    void clear() override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // Calcule le jour de la semaine à partir d'une date (0 = lundi, 6 = dimanche)
    int getDayOfWeek(const be::Date& date);
    
    // Analyse les trades pour extraire les performances par heure et jour
    void analyzeTradesByTimeAndDay(const std::vector<be::TradeData>& trades);
    
    // Construit la heatmap à partir des données analysées
    void buildHeatmap();
    
    // Retourne une couleur en fonction de la valeur (gradient rouge-blanc-vert)
    QColor getColorForValue(double value);

    // Composants d'interface
    QGroupBox* m_groupBox;
    QVBoxLayout* m_mainLayout;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    
    // Données pour la heatmap
    QVector<QVector<double>> m_performanceData; // [heure][jour] -> performance moyenne (%)
    QVector<QVector<int>> m_tradeCountData;     // [heure][jour] -> nombre de trades
    
    // Composants pour la légende
    QGraphicsScene* m_legendScene;
    QGraphicsView* m_legendView;
    
    // Valeurs min/max pour le gradient de couleur
    double m_minValue;
    double m_maxValue;
    int m_minHour;   // Première heure avec des trades
    int m_maxHour;   // Dernière heure avec des trades

    // Constantes
    static constexpr int HOURS_IN_DAY = 24;
    static constexpr int DAYS_IN_WEEK = 7;
    static constexpr int CELL_SIZE = 40;
    static constexpr int CELL_SPACING = 1;
    
    // Noms des jours pour l'affichage
    // QStringList m_dayNames{"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};
    QStringList m_dayNames{"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
};