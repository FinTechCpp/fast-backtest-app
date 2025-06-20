#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QFormLayout>
#include "chart_widget.h"

/**
 * @brief Classe de base pour tous les dialogues d'indicateurs techniques
 */
class IndicatorDialog : public QDialog
{
    Q_OBJECT
    
public:
    IndicatorDialog(QWidget* parent, ChartWidget* chartWidget, const QString& title);
    virtual ~IndicatorDialog();

protected:
    ChartWidget* m_chartWidget;
    QVBoxLayout* m_mainLayout;
    QFormLayout* m_formLayout;
    QDialogButtonBox* m_buttonBox;
    
    // Méthodes utilitaires communes
    void updateColorButtonStyle(QPushButton* button, int color);
    QColor openColorDialog(int currentColor, const QString& title);
    int colorFromRGB(int r, int g, int b);
    void getRGBComponents(int color, int& r, int& g, int& b);
    
    // Méthodes virtuelles pour les classes dérivées
    virtual void setupUI() = 0;
    virtual void connectSignals() = 0;
    virtual void applyChanges() = 0;
    virtual void cancelChanges() = 0;

private slots:
    void onApply();
    void onCancel();
};