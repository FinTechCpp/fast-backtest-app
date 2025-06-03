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
#include <vector>
#include <memory>
#include <map>
#include "views/baseview.h"
#include "chart_widget.h" // Inclure le nouveau widget

class App;

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

private:
    // Cache des données
    BacktestResults* m_cachedResults;
    bool m_dataExtracted;
    BacktestResults* m_currentResults;

    // UI Components 
    QWidget* m_leftPanel;  // Panneau de gauche (settings)
    QWidget* m_rightPanel; // Panneau de droite (chart)
    QLabel* m_chartPlaceholder;

    App* m_app;  // Référence à l'application principale

    // Contrôles dans le panneau de gauche
    QComboBox* m_chartTypeCombo; // Combo box pour le type de bougie
    QLabel* m_settingsTitle;     // Titre du panneau

    // Chart widget
    ChartWidget* m_chartWidget;
    
    bool hasValidData() const;
    void showPlaceholder(const QString& message);
    void showChartWidget();
};