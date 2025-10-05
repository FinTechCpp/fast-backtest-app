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

#include "ui/views/Stats/TitledWidget.h"
#include "stats.hpp"

class TradingHeatmapWidget : public TitledWidget {
    Q_OBJECT

public:
    explicit TradingHeatmapWidget(const QString& title = QString(), QWidget* parent = nullptr);

    void updateContent(const std::vector<be::TradeData>& trades);

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // Calcule le jour de la semaine à partir d'une date (0 = lundi, 6 = dimanche)
    int getDayOfWeek(const be::Date& date);
    
    // Analyse les trades pour extraire les performances par heure et jour
    void analyzeTradesByTimeAndDay(const std::vector<be::TradeData>& trades);
    
    // Retourne une couleur en fonction de la valeur (gradient rouge-blanc-vert)
    QColor getColorForValue(double value);

    // Composants d'interface
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    
    // Données pour la heatmap
    QVector<QVector<double>> m_performanceData; // [heure][jour] -> performance moyenne (%)
    QVector<QVector<int>> m_tradeCountData;     // [heure][jour] -> nombre de trades
    QVector<QVector<double>> m_squaredSumData;   // [heure][jour] -> somme des carrés des performances
    
    // Valeurs min/max pour le gradient de couleur
    double m_minValue;
    double m_maxValue;
    int m_minHour;   // Première heure avec des trades
    int m_maxHour;   // Dernière heure avec des trades

    // Constantes
    static constexpr int HOURS_IN_DAY = 24;
    static constexpr int DAYS_IN_WEEK = 7;
    static constexpr int CELL_SIZE = 45;
    static constexpr int CELL_SPACING = 0;
    
    // Noms des jours pour l'affichage
    // QStringList m_dayNames{"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};
    QStringList m_dayNames{"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    QVector<bool> m_activeDays; // Indique quels jours ont des trades
    QVector<int> m_activeDayIndices; // Indices des jours actifs pour l'affichage

    // Variables pour le suivi de la souris
    QPoint m_mousePos;
    bool m_mouseOver = false;
    int m_activeCell_col = -1;
    int m_activeCell_row = -1;
    QRect m_cellsArea; // Rectangle englobant toute la zone des cellules
};