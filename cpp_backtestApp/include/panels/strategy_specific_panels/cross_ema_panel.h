#pragma once

#include <QObject>  // Required for Q_OBJECT
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QMap>
#include <QString>
#include <QVariant>
#include "panels/base_panel.h"

/**
 * @brief Panel spécifique à la stratégie CrossEMA
 * 
 * Ce panel contient les paramètres spécifiques à la stratégie CrossEMA comme
 * les périodes des EMA court et long pour les signaux de croisement.
 */
class CrossEMAPanel : public QObject, public BasePanel
{
    Q_OBJECT  // Required for signals/slots

public:
    /**
     * @brief Constructeur
     * @param parent Pointeur vers le widget parent
     */
    CrossEMAPanel(QWidget* parent = nullptr);
    
    /**
     * @brief Crée et retourne l'interface graphique du panel
     * @return QGroupBox contenant les widgets du panel
     */
    QGroupBox* create() override;
    
    /**
     * @brief Récupère les valeurs des widgets du panel
     * @return Map contenant les valeurs sous forme de QVariant
     */
    QMap<QString, QVariant> getValues() override;
    
    /**
     * @brief Définit les valeurs des widgets du panel
     * @param values Map contenant les valeurs à affecter aux widgets
     */
    void setValues(const QMap<QString, QVariant>& values) override;

private slots:
    /**
     * @brief Valide que l'EMA long est toujours supérieur à l'EMA court
     */
    void validateEmaPeriods();

private:
    // Pas de membres privés supplémentaires nécessaires
    // Les widgets sont stockés dans m_widgets hérité de BasePanel
};
