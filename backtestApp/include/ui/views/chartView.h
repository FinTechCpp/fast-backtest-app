#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QMainWindow>
#include <QLineEdit>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QFormLayout>
#include <QStackedLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QSlider>
#include <vector>
#include <memory>
#include <map>

#include "ui/views/baseView.h"
#include "ui/chart/chartControlPanel.h"

#include "ui/dialogs/indicators/rsiDialog.h"
#include "ui/dialogs/indicators/emaDialog.h"
#include "ui/dialogs/indicators/supertrendDialog.h"
#include "ui/dialogs/indicators/stochasticDialog.h"
#include "ui/dialogs/indicators/atrDialog.h"
#include "ui/dialogs/indicators/cciDialog.h"
#include "ui/dialogs/indicators/macdDialog.h"
#include "ui/dialogs/indicators/pivotPointsDialog.h"
#include "ui/dialogs/indicators/bbDialog.h"

class App;
class RSIDialog;

/**
 * @brief View to display price and indicator charts
 */
class ChartView : public BaseView
{
    Q_OBJECT

public:
    explicit ChartView(QWidget* parent = nullptr);
    ~ChartView();
    
    // Implementations to save/load indicators in profile
    const std::vector<std::unique_ptr<indicators::IndicatorBase>>& getIndicators() const;
    void setIndicators(const std::vector<std::unique_ptr<indicators::IndicatorBase>>& indicators);

    // Implementation of BaseView virtual methods
    void updateData(BacktestResults* results) override;
    void clear() override;
    
    // Method to zoom in on a specific trade
    void zoomToTrade(const be::TradeData& trade);
    
    // Method to zoom in on a specific period
    void zoomToPeriod(const QDateTime& startDate, const QDateTime& endDate);

protected:
    void setupUI() override;

private:
    bool m_comparisonMode = false;

    // UI Components 
    ChartControlPanel* m_leftPanel;         // Left panel (settings)
    QWidget* m_rightPanel;        // Right panel (chart)
    QLabel* m_chartPlaceholder;
    QStackedLayout* m_rightPanelLayout; // Layout for the right panel
    QWidget* m_chartContainer; // Container for chart widgets

    ChartWidget* m_chartWidget;   // Chart widget
    ChartWidget* m_chartWidget2;   // Chart widget for vertical comparison

    
    void showPlaceholder(const QString& message);
    void showChartWidget();
    std::vector<std::unique_ptr<indicators::IndicatorBase>> extractIndicatorsFromFilters(const std::vector<StrategyConfig>& strategyConfigs);
};
