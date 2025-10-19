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
    
    // Méthode pour zoomer sur un trade spécifique
    void zoomToTrade(const be::TradeData& trade);

protected:
    void setupUI() override;

private:
    bool m_comparisonMode = false;

    // UI Components 
    ChartControlPanel* m_leftPanel;         // Panneau de gauche (settings)
    QWidget* m_rightPanel;        // Panneau de droite (chart)
    QLabel* m_chartPlaceholder;
    QStackedLayout* m_rightPanelLayout; // Layout pour le panneau droit
    QWidget* m_chartContainer; // Conteneur pour les widgets de graphique

    ChartWidget* m_chartWidget;   // Widget du graphique
    ChartWidget* m_chartWidget2;   // Widget du graphique pour la comparaison verticale

    
    void showPlaceholder(const QString& message);
    void showChartWidget();
    std::vector<std::unique_ptr<indicators::IndicatorBase>> extractIndicatorsFromFilters(const StrategyConfig& strategyConfig);
};
