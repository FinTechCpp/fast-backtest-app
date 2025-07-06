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
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QSlider>
#include <vector>
#include <memory>
#include <map>

#include "views/baseView.h"
#include "chartWidget.h"

#include "dialog/rsiDialog.h"
#include "dialog/emaDialog.h"
#include "dialog/supertrendDialog.h"
#include "dialog/stochasticDialog.h"
#include "dialog/atrDialog.h"

class App;
class RSIDialog;

/**
 * @brief Vue pour afficher les graphiques de prix et d'indicateurs
 */
class ChartView : public BaseView
{
    Q_OBJECT

public:
    explicit ChartView(QWidget* parent = nullptr);
    ~ChartView();
    
    // Implémentation des méthodes virtuelles de BaseView
    void updateData(BacktestResults* results) override;
    void clear() override;

protected:
    void setupUI() override;

private slots:
    void onChartTypeChanged(int index);
    void onAddIndicatorClicked();
    void onIndicatorTypeSelected(int index);
    void onEditIndicator(int id);
    void onRemoveIndicator(int id);
    void onRulerToolToggled(bool checked); 

    void onIndicatorAdded(int id, const QString& name);
    void onIndicatorChanged(int id, const QString& name);
    void onIndicatorRemoved(int id);
    
    void refreshIndicatorsList();
    void onAggregationSliderChanged(int value);
    void onMaxDisplayPointsChanged(int value);

private:
    // Cache des données
    BacktestResults* m_cachedResults;
    bool m_dataExtracted;
    BacktestResults* m_currentResults;

    // UI Components 
    QWidget* m_leftPanel;         // Panneau de gauche (settings)
    QWidget* m_rightPanel;        // Panneau de droite (chart)
    QLabel* m_chartPlaceholder;
    QLabel* m_settingsTitle;      // Titre du panneau
    QComboBox* m_chartTypeCombo;  // Combo box pour le type de bougie
    QCheckBox* m_rulerToolCheckBox; // Checkbox pour l'outil règle
    
    // Composants pour les indicateurs
    QPushButton* m_addIndicatorButton;
    QComboBox* m_indicatorTypeCombo;
    QGroupBox* m_indicatorsGroup;
    QVBoxLayout* m_indicatorsLayout;
    QMap<int, QPushButton*> m_editButtons;    // Map des boutons d'édition par ID d'indicateur
    QMap<int, QPushButton*> m_removeButtons;  // Map des boutons de suppression par ID d'indicateur
    QMap<int, QLabel*> m_indicatorLabels;     // Map des libellés d'indicateurs par ID

    App* m_app;                   // Référence à l'application principale
    ChartWidget* m_chartWidget;   // Widget du graphique

    QSlider* m_aggregationSlider;
    QLabel* m_aggregationLabel;
    
    bool hasValidData() const;
    void showPlaceholder(const QString& message);
    void showChartWidget();
    void setupIndicatorControls();
    void createIndicatorWidgets(int id, const QString& name);
    void configureStrategyIndicators(const std::vector<StrategyIndicator>& indicators);
    
    template<typename IndicatorType, typename DialogType>
    bool tryOpenDialog(int id) {
        IndicatorType* indicator = m_chartWidget->findIndicator<IndicatorType>(id);
        if (indicator) {
            DialogType* dialog = new DialogType(this, m_chartWidget, id, *indicator);
            dialog->exec();
            delete dialog;
            return true;
        }
        return false;
    }
};
