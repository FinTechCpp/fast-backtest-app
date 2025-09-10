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

#include "ui/views/baseView.h"
#include "ui/chart/chartControlPanel.h"

#include "ui/dialogs/rsiDialog.h"
#include "ui/dialogs/emaDialog.h"
#include "ui/dialogs/supertrendDialog.h"
#include "ui/dialogs/stochasticDialog.h"
#include "ui/dialogs/atrDialog.h"
#include "ui/dialogs/pivotPointsDialog.h"

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
    // Cache des données
    bool m_dataExtracted;

    // UI Components 
    ChartControlPanel* m_leftPanel;         // Panneau de gauche (settings)
    QWidget* m_rightPanel;        // Panneau de droite (chart)
    QLabel* m_chartPlaceholder;

    App* m_app;                   // Référence à l'application principale
    ChartWidget* m_chartWidget;   // Widget du graphique
    ChartWidget* m_chartWidget2;   // Widget du graphique pour la comparaison verticale
    
    void showPlaceholder(const QString& message);
    void showChartWidget();
    
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
