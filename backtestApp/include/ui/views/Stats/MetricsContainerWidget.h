#pragma once

#include <QVBoxLayout>
#include <QGroupBox>
#include <QMap>
#include <QString>
#include <QHBoxLayout>
#include "ui/metricWidget.h"
#include "ui/views/Stats/StatsBaseWidget.h"

class MetricsContainerWidget : public StatsBaseWidget {
    Q_OBJECT

public:
    explicit MetricsContainerWidget(QWidget* parent = nullptr);

    void updateContent(const be::Stats& stats) override;
    void clear() override;

private:
    enum class Section {
        Performance,
        Risk,
        General
    };
    
    // Structure pour définir une métrique
    struct MetricDefinition {
        QString key;
        QString label;
        QString tooltip;
        Section section;
        std::function<MetricStatus(const be::Stats&)> getStatus;
        std::function<QString(const be::Stats&)> formatValue;
    };

    // Initialise les métriques selon la section (performance, risk, general)
    void initializeMetrics();

    // Crée les widgets pour une section spécifique
    void createMetricWidgets(Section section, QVBoxLayout* layout);

    QVBoxLayout* m_mainLayout;
    
    // Les trois groupes de métriques
    QGroupBox* m_performanceGroup;
    QVBoxLayout* m_performanceLayout;
    QGroupBox* m_riskGroup;
    QVBoxLayout* m_riskLayout;
    QGroupBox* m_generalGroup;
    QVBoxLayout* m_generalLayout;
    
    // Stockage des widgets
    QMap<QString, MetricWidget*> m_metricWidgets;
    
    // Liste des définitions de métriques pour cette section
    QVector<MetricDefinition> m_metricDefinitions;
};