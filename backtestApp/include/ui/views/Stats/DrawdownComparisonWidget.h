#pragma once

#include "ui/views/Stats/StatsBaseWidget.h"
#include <QGroupBox>
#include <QPainter>

class DrawdownComparisonWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit DrawdownComparisonWidget(QWidget* parent = nullptr);
    virtual void updateContent(const be::Stats& stats) override;
    virtual void clear() override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QGroupBox* m_groupBox;

    // Données
    double m_maxDrawdownPct = 0.0;
    double m_avgDrawdownPct = 0.0;
    QString m_maxDuration;
    QString m_avgDuration;
    
    // Pour les barres de durée
    int m_maxDurationDays = 0;
    int m_avgDurationDays = 0;
};