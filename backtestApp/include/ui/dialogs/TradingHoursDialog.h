#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimeEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <vector>
#include "common.h"

class TradingHoursDialog : public QDialog {
    Q_OBJECT

public:
    explicit TradingHoursDialog(QWidget* parent = nullptr);
    ~TradingHoursDialog() override = default;

    void setConfig(const StrategyConfig& config);
    void updateConfig(StrategyConfig& config) const;

private:
    void setupUI();
    
    QTimeEdit* m_tradingFromTime;
    QTimeEdit* m_tradingToTime;
    std::vector<QCheckBox*> m_tradingDayCheckboxes;
};
