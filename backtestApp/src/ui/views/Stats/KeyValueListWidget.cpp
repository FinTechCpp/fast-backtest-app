#include "ui/views/Stats/KeyValueListWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>

KeyValueListWidget::KeyValueListWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent)
    , m_keyAlignment(Qt::AlignLeft | Qt::AlignVCenter)
    , m_valueAlignment(Qt::AlignLeft | Qt::AlignVCenter)
    , m_spacing(5)
    , m_keyTextWidth(0)
    , m_checkBoxString("Texte secondaire")
    , m_showBis(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_fontSize = 12;


    m_checkbox = new QCheckBox(m_checkBoxString);
    m_checkbox->setChecked(m_showBis);

    connect(m_checkbox, &QCheckBox::toggled, this, [this](bool checked) {
        m_showBis = checked;
        update();  // Redessiner le widget
    });

    setTitleCompanionWidget(m_checkbox);
    setTitleCompanionWidgetVisible(false);
}

void KeyValueListWidget::addItem(const QString& key, const QString& value, const QColor& color, const QString& valueBis)
{
    m_items.append(KeyValueItem(key, value, color, valueBis));
    
    // Activer la checkbox si au moins un item a une valeur alternative
    if (!valueBis.isEmpty()) {
        setTitleCompanionWidgetVisible(true);
    }
    
    update();
}

void KeyValueListWidget::addItems(const QList<KeyValueItem>& items)
{
    m_items.append(items);
    update();
}

void KeyValueListWidget::clear()
{
    m_items.clear();
    update();
}

bool KeyValueListWidget::updateValue(const QString& key, const QString& newValue)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].value = newValue;
            update();
            return true;
        }
    }
    return false;
}

bool KeyValueListWidget::updateValueBis(const QString& key, const QString& newValueBis)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].valueBis = newValueBis;
            
            // Activer la checkbox si au moins un item a une valeur alternative
            if (!newValueBis.isEmpty()) {
                setTitleCompanionWidgetVisible(true);
            }
            
            update();
            return true;
        }
    }
    return false;
}

bool KeyValueListWidget::updateValues(const QString& key, const QString& newValue, const QString& newValueBis)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].value = newValue;
            m_items[i].valueBis = newValueBis;
            
            // Activer la checkbox si au moins un item a une valeur alternative
            if (!newValueBis.isEmpty()) {
                setTitleCompanionWidgetVisible(true);
            }
            
            update();
            return true;
        }
    }
    return false;
}

bool KeyValueListWidget::updateColor(const QString& key, const QColor& newColor)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].key == key) {
            m_items[i].textColor = newColor;
            update();
            return true;
        }
    }
    return false;
}

void KeyValueListWidget::setCheckBoxBisString(const QString &text)
{
    if (m_checkBoxString != text) {
        m_checkBoxString = text;
        m_checkbox->setText(text);
        setTitleCompanionWidgetVisible(true);
        update();
    }
}

void KeyValueListWidget::setKeyAlignment(Qt::Alignment alignment)
{
    if (m_keyAlignment != alignment) {
        m_keyAlignment = alignment;
        update();
    }
}

void KeyValueListWidget::setValueAlignment(Qt::Alignment alignment)
{
    if (m_valueAlignment != alignment) {
        m_valueAlignment = alignment;
        update();
    }
}

void KeyValueListWidget::setSpacing(int spacing)
{
    if (m_spacing != spacing && spacing >= 0) {
        m_spacing = spacing;
        update();
    }
}

void KeyValueListWidget::setKeyTextWidth(int width)
{
    if (m_keyTextWidth != width && width >= 0) {
        m_keyTextWidth = width;
        update();
    }
}

QSize KeyValueListWidget::sizeHint() const
{
    if (m_items.isEmpty()) {
        return QSize(250, 150); // Taille par défaut si vide
    }
    
    // Recalculer la taille en fonction du contenu
    QFont font;
    font.setPointSize(m_fontSize);
    font.setWeight(m_fontWeight);

    QFontMetrics fm(font);
    
    int maxKeyWidth = calculateMaxKeyWidth(fm);
    int separatorWidth = fm.horizontalAdvance(": ");
    
    int maxValueWidth = 0;
    for (const auto& item : m_items) {
        // Considérer les deux valeurs pour le calcul de la taille
        int valueWidth = fm.horizontalAdvance(item.value);
        if (!item.valueBis.isEmpty()) {
            valueWidth = std::max(valueWidth, fm.horizontalAdvance(item.valueBis));
        }
        maxValueWidth = std::max(maxValueWidth, valueWidth);
    }
    
    int contentWidth = maxKeyWidth + separatorWidth + maxValueWidth;
    int contentHeight = m_items.count() * fm.height() + (m_items.count() - 1) * m_spacing;
    
    // Ajouter des marges autour du contenu
    contentWidth += 40;  // 20px de chaque côté
    contentHeight += 40; // 20px en haut et en bas
    
    return QSize(contentWidth, contentHeight);
}

QSize KeyValueListWidget::minimumSizeHint() const
{
    QSize hint = sizeHint();
    return QSize(hint.width() / 2, hint.height() / 2);
}

int KeyValueListWidget::calculateMaxKeyWidth(const QFontMetrics& fm) const
{
    // Si une largeur fixe est spécifiée, l'utiliser
    if (m_keyTextWidth > 0) {
        return m_keyTextWidth;
    }
    
    int maxWidth = 0;
    for (const auto& item : m_items) {
        int width = fm.horizontalAdvance(item.key);
        maxWidth = std::max(maxWidth, width);
    }
    return maxWidth;
}

void KeyValueListWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    // Rien à dessiner si la liste est vide
    if (m_items.isEmpty()) {
        return;
    }
    
    // Configuration du texte avec la taille personnalisée
    QFont font = painter.font();
    font.setPointSize(m_fontSize);
    font.setWeight(m_fontWeight);
    painter.setFont(font);
    
    QFontMetrics fm(font);
    int lineHeight = fm.height();
    int maxKeyWidth = calculateMaxKeyWidth(fm);
    
    // Largeur du séparateur ": "
    int separatorWidth = fm.horizontalAdvance(": ");
    
    // Calculer la largeur totale du bloc de contenu
    int contentWidth = maxKeyWidth + separatorWidth;
    
    // Trouver la valeur la plus large pour déterminer la largeur totale
    int maxValueWidth = 0;
    for (const auto& item : m_items) {
        // Vérifier quelle valeur afficher selon m_showBis
        QString displayValue = (m_showBis && !item.valueBis.isEmpty()) ? item.valueBis : item.value;
        maxValueWidth = std::max(maxValueWidth, fm.horizontalAdvance(displayValue));
    }
    contentWidth += maxValueWidth;
    
    // Calculer la hauteur totale du bloc de contenu
    int contentHeight = m_items.count() * lineHeight + (m_items.count() - 1) * m_spacing;
    
    // Calculer les positions de départ pour centrage avec marges minimales de sécurité
    int marginX = 10; // Marge minimale horizontale
    int marginY = 10; // Marge minimale verticale
    
    // Centrer le contenu, mais s'assurer qu'il ne dépasse pas les marges
    int startX = qMax(marginX, (contentRect.left() + contentRect.width() - contentWidth) / 2);
    int startY = qMax(marginY, (contentRect.top() + contentRect.height() - contentHeight) / 2);
    
    // Position y courante
    int y = startY + lineHeight/2; // Commencer avec un espace pour la première ligne
    
    // Dessiner chaque ligne
    for (const auto& item : m_items) {
        // Couleur du texte pour cette ligne
        painter.setPen(item.textColor);
        
        // Déterminer quelle valeur afficher
        QString displayValue = (m_showBis && !item.valueBis.isEmpty()) ? item.valueBis : item.value;
        
        // Rectangle pour la clé, assuré d'être dans les limites
        QRect keyRect(startX, y - lineHeight/2, maxKeyWidth, lineHeight);
        
        // Rectangle pour le séparateur
        QRect separatorRect(keyRect.right(), y - lineHeight/2, separatorWidth, lineHeight);
        
        // Rectangle pour la valeur
        QRect valueRect(separatorRect.right(), y - lineHeight/2, 
                       maxValueWidth, lineHeight);
        
        // Dessiner la clé alignée à droite pour meilleur alignement avec le séparateur
        painter.drawText(keyRect, Qt::AlignRight | Qt::AlignVCenter, item.key);
        
        // Dessiner le séparateur
        painter.drawText(separatorRect, Qt::AlignCenter, ": ");
        
        // Dessiner la valeur (primaire ou alternative selon m_showBis)
        painter.drawText(valueRect, m_valueAlignment, displayValue);
        
        // Passer à la ligne suivante
        y += lineHeight + m_spacing;
    }
}