#pragma once

#include "ui/views/baseView.h"
#include "ui/views/Stats/TradesTableWidget.h"
#include <QVBoxLayout>

class TradesView : public BaseView
{
    Q_OBJECT

public:
    explicit TradesView(QWidget* parent = nullptr);
    ~TradesView() override;

    void updateData(BacktestResults* results) override;
    void clear() override;

signals:
    void tradeClicked(const be::TradeData& trade);

private:
    void setupUI();

    // Widgets
    TradesTableWidget* m_tradesTableWidget;
    
    // Résultats actuels
    BacktestResults* m_currentResults;
};
