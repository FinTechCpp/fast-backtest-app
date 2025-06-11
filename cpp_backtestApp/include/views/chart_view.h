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
#include <vector>
#include <memory>
#include <map>

#include "views/baseview.h"
#include "chart_widget.h"

#include "dialog/rsi_dialog.h"
#include "dialog/ema_dialog.h"
#include "dialog/stochastic_dialog.h"
#include "dialog/atr_dialog.h"

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
    void onRSIAdded(int id, int period);
    void onRSIChanged(int id, int period);
    void onRSIRemoved(int id);

    void onEMAAdded(int id, int period);
    void onEMAChanged(int id, int period);
    void onEMARemoved(int id);
    void onEditEMA();

    // Slots pour le Stochastique
    void onStochasticAdded(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void onStochasticChanged(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void onStochasticRemoved(int id);

    // Slots pour l'ATR
    void onATRAdded(int id, int period);
    void onATRChanged(int id, int period);
    void onATRRemoved(int id);
    
    void refreshIndicatorsList();

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
    
    bool hasValidData() const;
    void showPlaceholder(const QString& message);
    void showChartWidget();
    void setupIndicatorControls();
    void createIndicatorWidgets(int id, const QString& name);
};
