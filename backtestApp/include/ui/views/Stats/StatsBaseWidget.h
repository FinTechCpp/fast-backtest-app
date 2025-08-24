#pragma once

#include <QWidget>
#include "stats.hpp"

class StatsBaseWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatsBaseWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~StatsBaseWidget() {}

    virtual void updateContent(const be::Stats& stats) = 0;
    virtual void clear() = 0;
};
