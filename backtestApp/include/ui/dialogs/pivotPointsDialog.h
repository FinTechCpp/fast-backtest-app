#pragma once

#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

#include "ui/dialogs/baseDialog.h"
#include "ui/chart/chartTypes.h"


/**
 * @brief Modal dialog to modify the parameters of Pivot Points indicator
 */
class PivotPointsDialog : public IndicatorDialog<indicators::PivotPointsInstance>
{
    Q_OBJECT
    
public:
    PivotPointsDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::PivotPointsInstance& pivotPoints);
    ~PivotPointsDialog() override;
    
private slots:
    void onPeriodTypeChanged(size_t index);
    void onCalculationMethodChanged(size_t index);
    void onShowMidLevelsChanged(int state);
    void onShowLabelsChanged(int state);
    
    // Méthodes pour gérer les modifications de style des niveaux
    void onLevelVisibilityChanged(indicators::PivotPointsInstance::LevelType levelType, bool checked);
    void onLevelColorChanged(indicators::PivotPointsInstance::LevelType levelType);
    void onLevelThicknessChanged(indicators::PivotPointsInstance::LevelType levelType, int value);
    void onLevelLineStyleChanged(indicators::PivotPointsInstance::LevelType levelType, int index);
    
protected:
    // Méthodes virtuelles de BaseDialog
    void setupUI() override;
    void connectSignals() override;
    void updateUIFromInstance() override;

private:
    // Méthodes utilitaires
    void setupLevelControls(QGridLayout* layout, int row, indicators::PivotPointsInstance::LevelType levelType, const QString& labelText);
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
    
    std::array<LevelControls, static_cast<size_t>(indicators::PivotPointsInstance::LevelType::Count)> m_levelControls;
    QTabWidget* m_tabWidget;  // Pour organiser les niveaux en onglets
    
    // Structure pour suivre l'état de synchronisation des groupes
    struct SyncGroup {
        bool synchronized = false;
        int color;  // Couleur représentative du groupe
        std::vector<indicators::PivotPointsInstance::LevelType> levelTypes; // Les niveaux appartenant à ce groupe
    };

    std::map<std::string, SyncGroup> m_syncGroups; // Les groupes de synchronisation (R, S, mR, mS)
    
    // Méthodes pour la synchronisation
    void initSyncGroups();
    void onSyncButtonToggled(indicators::PivotPointsInstance::LevelType levelType, bool checked);
    void updateSyncButtonsInGroup(const std::string& groupName);
    void syncGroupControls(const std::string& groupName, indicators::PivotPointsInstance::LevelType sourceLevelType);
    std::string getLevelGroup(indicators::PivotPointsInstance::LevelType levelType) const;
};