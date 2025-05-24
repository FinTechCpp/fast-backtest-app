#ifndef METRIC_WIDGET_H
#define METRIC_WIDGET_H

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QString>

/**
 * @brief Widget pour afficher une métrique avec un titre, une valeur et une variation optionnelle
 * 
 * Ce widget est utilisé dans les vues de résultats pour afficher les statistiques du backtest
 * de manière visuellement cohérente et attrayante.
 */
class MetricWidget : public QFrame
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur
     * @param title Titre de la métrique
     * @param value Valeur initiale (optionnelle)
     * @param delta Variation (optionnelle)
     * @param delta_color Couleur de la variation ("normal" = vert, "inverse" = rouge)
     * @param parent Widget parent
     */
    MetricWidget(const QString& title, const QString& value = "",
                 const QString& delta = "", const QString& delta_color = "normal",
                 QWidget* parent = nullptr);
    
    /**
     * @brief Met à jour les valeurs affichées dans le widget
     * @param value Nouvelle valeur à afficher
     * @param delta Nouvelle variation (optionnelle)
     * @param delta_color Couleur de la variation ("normal" = vert, "inverse" = rouge)
     */
    void updateValues(const QString& value, const QString& delta = "",
                      const QString& delta_color = "normal");

private:
    QLabel* m_titleLabel;    // Étiquette pour le titre
    QLabel* m_valueLabel;    // Étiquette pour la valeur
    QLabel* m_deltaLabel;    // Étiquette pour la variation
};

#endif // METRIC_WIDGET_H