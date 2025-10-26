#pragma once

#include "ui/views/Stats/TitledWidget.h"
#include <QList>
#include <QCheckBox>

// Structure pour stocker les informations de chaque ligne
struct KeyValueItem {
    QString key;        // Le texte de gauche (clé)
    QString value;      // La valeur primaire de droite
    QString valueBis;   // La valeur alternative (optionnelle)
    QColor textColor;   // Couleur du texte
    
    KeyValueItem(const QString& k = QString(), 
                const QString& v = QString(), 
                const QColor& c = Qt::black,
                const QString& vBis = QString())
        : key(k), value(v), valueBis(vBis), textColor(c) {}
};

class KeyValueListWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit KeyValueListWidget(const QString& title = QString(), QWidget* parent = nullptr);
    
    // Ajouter une entrée à la liste
    void addItem(const QString& key, const QString& value, const QColor& color = Qt::black, const QString& valueBis = QString());
    
    // Ajouter plusieurs entrées d'un coup
    void addItems(const QList<KeyValueItem>& items);
    
    // Supprimer toutes les entrées
    void clear();
    
    // Mettre à jour une entrée existante (valeur primaire)
    bool updateValue(const QString& key, const QString& newValue);
    
    // Mettre à jour la valeur alternative
    bool updateValueBis(const QString& key, const QString& newValueBis);
    
    // Mettre à jour les deux valeurs en même temps
    bool updateValues(const QString& key, const QString& newValue, const QString& newValueBis);
    
    // Mettre à jour la couleur d'une entrée
    bool updateColor(const QString& key, const QColor& newColor);
    
    // Accès aux éléments
    QList<KeyValueItem> items() const { return m_items; }
    int count() const { return m_items.count(); }
    
    // Configuration visuelle
    void setCheckBoxBisString(const QString &text);
    void setKeyAlignment(Qt::Alignment alignment);
    void setValueAlignment(Qt::Alignment alignment);
    void setSpacing(int spacing);
    void setKeyTextWidth(int width); // Force une largeur spécifique (0 = auto)
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

private:
    QString m_checkBoxString;
    QCheckBox* m_checkbox;
    bool m_showBis;

    QList<KeyValueItem> m_items;
    Qt::Alignment m_keyAlignment;
    Qt::Alignment m_valueAlignment;
    int m_spacing;
    int m_keyTextWidth;  // Largeur fixe pour les clés (0 = auto)
    
    // Calcule la largeur maximale des clés pour l'alignement
    int calculateMaxKeyWidth(const QFontMetrics& fm) const;
};