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
    // je n'aime pas dutout cette methode elle addIndicator et a chaque ajout on update le chart
    void configureStrategyIndicators(const std::vector<StrategyIndicator>& indicators);
    void setChartWidget(ChartWidget* chartWidget);
    
    // Nouvelle méthode pour mettre à jour l'état de comparaison dans l'UI
    void setComparisonMode(bool enabled);

signals:
    void chartTypeChanged(const QString& chartType);
    void rulerToolToggled(bool checked);
    void transferDataForComparison(); // Nouveau signal pour transférer les données
    void exitComparisonMode();        // Nouveau signal pour quitter le mode comparaison
    void aggregationValueChanged(int value);

private slots:
    void onChartTypeChanged(int index);
    void onAddIndicatorClicked();
    void onIndicatorTypeSelected(int index);
    void onEditIndicator(int id);
    void onRemoveIndicator(int id);
    void onRulerToolToggled(bool checked);
    void onTransferDataClicked();     // Nouveau slot pour le bouton de transfert
    void onExitComparisonClicked();   // Nouveau slot pour le bouton de sortie
    void onAggregationSliderChanged(int value);

    void onIndicatorAdded(int id, const QString& name);
    void onIndicatorChanged(int id, const QString& name);
    void onIndicatorRemoved(int id);

private:
    void setupUI();
    void setupIndicatorControls();
    void createIndicatorWidgets(int id, const QString& name);

    QLabel* m_settingsTitle;
    QComboBox* m_chartTypeCombo;
    QToolButton* m_rulerToolButton;
    
    // Nouveaux boutons de comparaison
    QPushButton* m_transferDataButton;
    QPushButton* m_exitComparisonButton;
    QHBoxLayout* m_comparisonButtonsLayout;
    
    // Composants pour les indicateurs
    QPushButton* m_addIndicatorButton;
    QComboBox* m_indicatorTypeCombo;
    QGroupBox* m_indicatorsGroup;
    QVBoxLayout* m_indicatorsLayout;
    QMap<int, QPushButton*> m_editButtons;
    QMap<int, QPushButton*> m_removeButtons;
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
};