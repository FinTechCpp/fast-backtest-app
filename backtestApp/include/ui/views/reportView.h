#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "ui/views/baseView.h"
#include "ui/views/Stats/ReportWidget.h"

/**
 * @brief View for AI-powered strategy analysis reports
 * 
 * This view provides a dedicated tab for AI-generated analysis of backtest results,
 * using the ReportWidget to generate comprehensive strategy reports.
 */
class ReportView : public BaseView
{
    Q_OBJECT

public:
    explicit ReportView(QWidget* parent = nullptr);
    ~ReportView() override;

    /**
     * @brief Update the view with new backtest results
     * @param results Backtest results to analyze
     */
    void updateData(BacktestResults* results) override;

    /**
     * @brief Clear the view and reset to initial state
     */
    void clear() override;

protected:
    /**
     * @brief Setup the user interface
     */
    void setupUI() override;

private:
    ReportWidget* m_reportWidget;
};
