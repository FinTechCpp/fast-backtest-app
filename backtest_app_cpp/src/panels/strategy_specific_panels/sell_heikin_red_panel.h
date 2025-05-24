#ifndef SELL_HEIKIN_RED_PANEL_H
#define SELL_HEIKIN_RED_PANEL_H

#include <QObject>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include "../base_panel.h"

class SellHeikinRedPanel : public QObject, public BasePanel
{
    Q_OBJECT

public:
    SellHeikinRedPanel(QWidget* parent = nullptr);
    QGroupBox* create() override;
    QMap<QString, QVariant> getValues() override;
    void setValues(const QMap<QString, QVariant>& values) override;

private:
    void _toggleWidgetGroup(const QStringList& widgets, bool enabled);
};

#endif // SELL_HEIKIN_RED_PANEL_H