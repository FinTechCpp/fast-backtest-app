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
#include "chart_widget.h" // Inclure le nouveau widget

class App;
class IndicatorDialog;

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
    void onRSIAdded(int id, int period);
    void onRSIChanged(int id, int period);
    void onRSIRemoved(int id);

    void onEMAAdded(int id, int period);
    void onEMAChanged(int id, int period);
    void onEMARemoved(int id);
    void onEditEMA();
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

/**
 * @brief Dialogue modal pour modifier les paramètres d'un indicateur technique
 */
class IndicatorDialog : public QDialog
{
    Q_OBJECT
    
public:
    // Construction pour RSI
    IndicatorDialog(QWidget* parent, ChartWidget* chartWidget, int rsiId, const ChartWidget::RSIInstance& rsi);
    ~IndicatorDialog();
    
private slots:
    void onApply();
    void onCancel();
    void onPeriodChanged(int period);
    void onHeightChanged(int height);
    void onRangeChanged(double range);
    void onColorButtonClicked();
    void onUpperColorButtonClicked();
    void onLowerColorButtonClicked();
    
private:
    ChartWidget* m_chartWidget;
    int m_rsiId;
    ChartWidget::RSIInstance m_originalRsi;  // Pour restaurer en cas d'annulation
    ChartWidget::RSIInstance m_currentRsi;   // Pour les modifications en cours
    
    QSpinBox* m_periodSpinBox;
    QSpinBox* m_heightSpinBox;
    QDoubleSpinBox* m_rangeSpinBox;
    QPushButton* m_colorButton;
    QPushButton* m_upperColorButton;
    QPushButton* m_lowerColorButton;
    QDialogButtonBox* m_buttonBox;
    
    void updateColorButtonStyle(QPushButton* button, int color);
    void updateRSI();
};

/**
 * @brief Dialogue modal pour configurer plusieurs EMA
 */
class EMADialog : public QDialog
{
    Q_OBJECT
    
public:
    // Construction pour EMA
    EMADialog(QWidget* parent, ChartWidget* chartWidget);
    ~EMADialog();
    
private slots:
    void onApply();
    void onCancel();
    void onAddEMA();
    void onRemoveEMA(int row);
    void onColorButtonClicked(int row);
    void onEnabledStateChanged(int row, bool enabled);
    void onPeriodChanged(int row, int period);
    
private:
    ChartWidget* m_chartWidget;
    std::vector<ChartWidget::EMAInstance> m_originalEMAs;  // Pour restaurer en cas d'annulation
    std::vector<ChartWidget::EMAInstance> m_currentEMAs;   // Pour les modifications en cours
    std::map<int, int> m_rowToEMAId;        // Mappage de la ligne de l'UI à l'ID de l'EMA
    
    QVBoxLayout* m_emaListLayout;
    QPushButton* m_addEMAButton;
    QDialogButtonBox* m_buttonBox;
    
    int m_nextRowId = 0;
    
    void updateColorButtonStyle(QPushButton* button, int color);
    QWidget* createEMARow(const ChartWidget::EMAInstance& ema, int row);
    void refreshEMAList();
};
