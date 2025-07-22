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
class PivotPointsDialog : public IndicatorDialog<PivotPointsInstance>
{
    Q_OBJECT
    
public:
    PivotPointsDialog(QWidget* parent, ChartWidget* chartWidget, const PivotPointsInstance& pivotPoints);
    ~PivotPointsDialog() override;
    
private slots:
    void onPeriodTypeChanged(int index);
    void onCalculationMethodChanged(int index);
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
    void updateUIFromInstance() override;

private:
    // Méthodes utilitaires
    void setupLevelControls(QGridLayout* layout, int row, PivotPointsInstance::LevelType levelType, const QString& labelText);
    QComboBox* createLineStyleComboBox();
    void updateLevelControlsState();
    
    // Widgets de configuration générale
    QComboBox* m_periodTypeComboBox;
    QComboBox* m_calculationMethodComboBox;
    QCheckBox* m_showMidLevelsCheckBox;
    QCheckBox* m_showLabelsCheckBox;
    
    // Stockage des contrôles par niveau
    struct LevelControls {
        QCheckBox* visibilityCheckBox;
        QPushButton* colorButton;
        QSpinBox* thicknessSpinBox;
        QComboBox* lineStyleComboBox;
        QPushButton* syncButton; // Nouveau bouton de synchronisation
    };
    
    std::map<int, LevelControls> m_levelControls;  // Map des contrôles par type de niveau
    QTabWidget* m_tabWidget;  // Pour organiser les niveaux en onglets
    
    // Structure pour suivre l'état de synchronisation des groupes
    struct SyncGroup {
        bool synchronized = false;
        int color;  // Couleur représentative du groupe
        std::vector<int> levelTypes; // Les niveaux appartenant à ce groupe
    };

    std::map<std::string, SyncGroup> m_syncGroups; // Les groupes de synchronisation (R, S, mR, mS)
    
    // Méthodes pour la synchronisation
    void initSyncGroups();
    void onSyncButtonToggled(int levelType, bool checked);
    void updateSyncButtonsInGroup(const std::string& groupName);
    void syncGroupControls(const std::string& groupName, int sourceLevelType);
    std::string getLevelGroup(int levelType) const;
};