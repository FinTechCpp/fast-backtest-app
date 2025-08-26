#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVector>
#include <QMap>
#include <QColor>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QGraphicsTextItem>
#include "stats.hpp"

class MonthlyPerformanceWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit MonthlyPerformanceWidget(QWidget* parent = nullptr);
    ~MonthlyPerformanceWidget();
    
    void updateData(const be::Stats& stats);
    void clear();
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    
private:
    void analyzeTradesByMonthAndYear(const std::vector<be::TradeData>& trades);
    QColor getColorForValue(double value);
    void buildHeatmap();

    // Constantes pour le dessin
    static constexpr int CELL_SIZE = 50;
    static constexpr int CELL_SPACING = 2;
    static constexpr int MONTHS_IN_YEAR = 12;
    
    // Noms des mois
    const QStringList m_monthNames = {
        "Jan", "Fév", "Mar", "Avr", "Mai", "Jun", 
        "Jui", "Aoû", "Sep", "Oct", "Nov", "Déc"
    };
    
    // Structures de données pour les performances
    QMap<int, QMap<int, double>> m_performanceData;  // [année][mois] -> performance
    QMap<int, QMap<int, int>> m_tradeCountData;      // [année][mois] -> nombre de trades
    
    // Années min et max pour déterminer la plage d'affichage
    int m_minYear;
    int m_maxYear;
    
    // Valeurs min et max pour l'échelle de couleur
    double m_minValue;
    double m_maxValue;
    
    // Interface graphique
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_groupBox;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
};