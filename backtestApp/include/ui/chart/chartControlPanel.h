#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QSlider>
#include <QToolButton>
#include <QMap>
#include <vector>
#include <memory>

#include "ui/chart/chartWidget.h"

class ChartControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChartControlPanel(QWidget* parent = nullptr);
    ~ChartControlPanel();

    void refreshIndicatorsList();
    void configureIndicatorInstances(const std::vector<std::unique_ptr<indicators::IndicatorBase>>& indicators);
    void suggestIndicatorsFromStrategy(const std::vector<std::unique_ptr<indicators::IndicatorBase>>& indicators);
    void setChartWidget(ChartWidget* chartWidget);
    
    // method to update the comparison state in the UI
    void setComparisonMode(bool enabled);

signals:
    void chartTypeChanged(const QString& chartType);
    void rulerToolToggled(bool checked);
    void transferDataForComparison(); // signal to transfer data
    void exitComparisonMode();        // signal to exit comparison mode
    void aggregationValueChanged(int value);

private slots:
    void onChartTypeChanged(int index);
    void onAddIndicatorClicked();
    void onIndicatorTypeSelected(int index);
    void onEditIndicator(int id);
    void onRemoveIndicator(int id);
    void onHideIndicator(int id); // New slot
    void onRulerToolToggled(bool checked);
    void onTransferDataClicked();     // slot for the transfer button
    void onExitComparisonClicked();   // slot for the exit button
    void onAggregationSliderChanged(int value);

    void onCheckMarkerToggled(bool checked);
    void onErrorMarkerToggled(bool checked);
    void onClearMarkersClicked();

    void onIndicatorAdded(int id, const QString& name);
    void onIndicatorChanged(int id, const QString& name);
    void onIndicatorRemoved(int id);

    // slots for suggestions
    void onAddSuggestedIndicator(int index);
    void onRejectSuggestion(int index);
    void onClearAllSuggestions();
    void onAddAllSuggestions(); 

private:
    void setupUI();
    void setupIndicatorControls();
    void createIndicatorWidgets(int id, const QString& name, bool isVisible);
    void createSuggestionWidget(std::unique_ptr<indicators::IndicatorBase> indicator); // Helper method
    bool hasSimilarIndicator(const indicators::IndicatorBase* indicator) const; // Helper method

    QLabel* m_settingsTitle;
    QComboBox* m_chartTypeCombo;
    QToolButton* m_rulerToolButton;
    
    // Buttons for drawing tools
    QToolButton* m_checkMarkerButton;
    QToolButton* m_errorMarkerButton;
    QPushButton* m_clearMarkersButton;
    
    // New comparison buttons
    QPushButton* m_transferDataButton;
    QPushButton* m_exitComparisonButton;
    QHBoxLayout* m_comparisonButtonsLayout;
    
    // Components for indicators
    QPushButton* m_addIndicatorButton;
    QComboBox* m_indicatorTypeCombo;
    QGroupBox* m_indicatorsGroup;
    QVBoxLayout* m_indicatorsLayout;
    QMap<int, QPushButton*> m_editButtons;
    QMap<int, QPushButton*> m_removeButtons;
    QMap<int, QPushButton*> m_hideButtons; // New map
    QMap<int, QLabel*> m_indicatorLabels;

    QSlider* m_aggregationSlider;
    QLabel* m_aggregationLabel;

    ChartWidget* m_chartWidget;
    bool m_comparisonActive = false;

    template<typename IndicatorType, typename DialogType>
    bool tryOpenDialog(int id) {
        IndicatorType* indicator = m_chartWidget->findIndicator<IndicatorType>(id);
        if (indicator) {
            DialogType* dialog = new DialogType(this, m_chartWidget, *indicator);
            dialog->exec();
            delete dialog;
            return true;
        }
        return false;
    }

    // attributes for the suggestions section
    QGroupBox* m_suggestionsGroup;
    QVBoxLayout* m_suggestionsLayout;
    QPushButton* m_clearSuggestionsButton;
    QPushButton* m_addAllSuggestionsButton; // New button
    
    struct SuggestionItem {
        QWidget* widget;
        QLabel* label;
        QPushButton* addButton;
        QPushButton* rejectButton;
        std::unique_ptr<indicators::IndicatorBase> indicator;
    };
    std::vector<SuggestionItem> m_suggestions;
};