#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include "ui/views/Stats/StatsBaseWidget.h"

class VerticalGaugeRenderWidget;

class PnLGaugeWidget : public StatsBaseWidget {
    Q_OBJECT
    
public:
    explicit PnLGaugeWidget(QWidget* parent = nullptr);
    ~PnLGaugeWidget();

    void updateContent(const be::Stats& stats) override;
    void clear() override;
    
private:
    void setupUI();
    void updateLabels();
    
    // Widget de rendu de la jauge
    VerticalGaugeRenderWidget* m_gaugeWidget;
    
    // Widgets UI pour la légende
    QWidget* m_legendWidget;
    QLabel* m_titleLabel;
    
    // Labels pour les statistiques combinées
    QLabel* m_ratioLabel;
    QLabel* m_edgeLabel;
    
    // Valeurs calculées
    double m_tpAvg;     // Moyenne des TP
    double m_tpMax;     // Maximum des TP
    double m_tpMedian;  // Médiane des TP
    double m_slAvg;     // Moyenne des SL
    double m_slMin;     // Minimum des SL
    double m_slMedian;  // Médiane des SL
    int m_tpCount;      // Nombre de trades TP
    int m_slCount;      // Nombre de trades SL
    
    // Information sur les meilleurs/pires trades
    QString m_bestTradeInfo;
    QString m_worstTradeInfo;
    
    // Mise en page
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    QVBoxLayout* m_legendLayout;
};

class VerticalGaugeRenderWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit VerticalGaugeRenderWidget(QWidget* parent = nullptr);
    
    // Configuration des valeurs
    void setValues(double tpAvg, double tpMax, double tpMedian,
                  double slAvg, double slMin, double slMedian);
    void clear();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    // Conversion valeur -> position Y
    int valueToY(double value, double minValue, double maxValue, int height);
    
    // Valeurs à afficher
    double m_tpAvg;     // Moyenne des trades gagnants
    double m_tpMax;     // Maximum des trades gagnants
    double m_tpMedian;  // Médiane des trades gagnants
    double m_slAvg;     // Moyenne des trades perdants
    double m_slMin;     // Minimum des trades perdants
    double m_slMedian;  // Médiane des trades perdants
    
    // Couleurs
    QColor m_tpAvgColor;    // Vert foncé pour la moyenne des TP
    QColor m_tpMaxColor;    // Vert clair pour le max des TP
    QColor m_slAvgColor;    // Rouge foncé pour la moyenne des SL
    QColor m_slMinColor;    // Rouge clair pour le min des SL
    QColor m_medianTpColor; // Couleur de la ligne de médiane TP
    QColor m_medianSlColor; // Couleur de la ligne de médiane SL
    QColor m_lineColor;     // Couleur des lignes
    QColor m_textColor;     // Couleur du texte
};