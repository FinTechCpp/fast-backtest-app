#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>

enum class MetricStatus {
    Good,
    Neutral,
    Bad,
    NA
};

class MetricWidget : public QWidget
{
    Q_OBJECT

public:
    MetricWidget(const QString& label, const QString& value = "N/A", QWidget* parent = nullptr);
    void updateValues(const QString& value, MetricStatus status = MetricStatus::Neutral);
    void setTooltip(const QString& tooltip);

private:
    QLabel* m_labelWidget;
    QLabel* m_valueWidget;
};