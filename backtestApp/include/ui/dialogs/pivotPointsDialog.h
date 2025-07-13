#pragma once

#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

#include "ui/dialogs/baseDialog.h"
#include "ui/chart/indicatorInstances.h"

/**
 * @brief Modal dialog to modify the parameters of Pivot Points indicator
 */
class PivotPointsDialog : public BaseDialog
{
    Q_OBJECT
    
public:
    // Constructor
    PivotPointsDialog(QWidget* parent, ChartWidget* chartWidget, int pivotId, const PivotPointsInstance& pivotPoints);
    ~PivotPointsDialog() override;
    
private slots:
    void onPeriodTypeChanged(int index);
    void onShowMidLevelsChanged(int state);
    void onShowLabelsChanged(int state);
    
    // Méthodes pour gérer les modifications de style des niveaux
    void onLevelVisibilityChanged(int levelType, bool checked);
    void onLevelColorChanged(int levelType);
    void onLevelThicknessChanged(int levelType, int value);
    void onLevelLineStyleChanged(int levelType, int index);
    
protected:
    // Méthodes virtuelles de BaseDialog
    void setupUI() override;
    void connectSignals() override;
    void applyChanges() override;
    void cancelChanges() override;

private:
    // Méthodes utilitaires
    void setupLevelControls(QGridLayout* layout, int row, PivotPointsInstance::LevelType levelType, const QString& labelText);
    QComboBox* createLineStyleComboBox();
    void updateLevelControlsState();
    
    int m_pivotId;
    PivotPointsInstance m_originalPivots;  // Pour restauration en cas d'annulation
    PivotPointsInstance m_currentPivots;   // Pour les modifications en cours
    
    // Widgets de configuration générale
    QComboBox* m_periodTypeComboBox;
    QCheckBox* m_showMidLevelsCheckBox;
    QCheckBox* m_showLabelsCheckBox;
    
    // Stockage des contrôles par niveau
    struct LevelControls {
        QCheckBox* visibilityCheckBox;
        QPushButton* colorButton;
        QSpinBox* thicknessSpinBox;
        QComboBox* lineStyleComboBox;
    };
    
    std::map<int, LevelControls> m_levelControls;  // Map des contrôles par type de niveau
    QTabWidget* m_tabWidget;  // Pour organiser les niveaux en onglets
};