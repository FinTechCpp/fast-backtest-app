#include "ui/views/Stats/TitledWidget.h"
#include <QPainter>
#include <QFontMetrics>

TitledWidget::TitledWidget(const QString& title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_titleColor(Qt::black)
    , m_titleBackgroundColor(Qt::transparent) // Transparent
    , m_titleVisible(!title.isEmpty())
    , m_backgroundColor(Qt::transparent) // Transparent
    , m_titleCompanionWidget(nullptr)
{
    // setMinimumSize(100, 100);
    // setMinimumSize(140, 140);
}

void TitledWidget::setTitle(const QString& title)
{
    if (m_title != title) {
        m_title = title;
        m_titleVisible = !title.isEmpty();
        update();
    }
}

void TitledWidget::setTitleColor(const QColor& color)
{
    if (m_titleColor != color) {
        m_titleColor = color;
        update();
    }
}

void TitledWidget::setTitleBackgroundColor(const QColor& color)
{
    if (m_titleBackgroundColor != color) {
        m_titleBackgroundColor = color;
        update();
    }
}

void TitledWidget::setTitleVisible(bool visible)
{
    if (m_titleVisible != visible) {
        m_titleVisible = visible;
        update();
    }
}

void TitledWidget::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
    }
}

void TitledWidget::setTitleCompanionWidget(QWidget* widget)
{
    // Si un widget existe déjà, le supprimer
    if (m_titleCompanionWidget) {
        m_titleCompanionWidget->setParent(nullptr);
        m_titleCompanionWidget->deleteLater();
    }
    
    m_titleCompanionWidget = widget;
    
    if (m_titleCompanionWidget) {
        m_titleCompanionWidget->setParent(this);
        m_titleCompanionWidget->show();
    }
    
    update();
}

QRect TitledWidget::contentRect() const
{
    QRect rect = this->rect();
    
    if (m_titleVisible && !m_title.isEmpty()) {
        // Réserver l'espace pour le titre en haut
        QFontMetrics fm(font());
        int titleHeight = fm.height() + 4; // Hauteur du texte + marge
        rect.setTop(rect.top() + titleHeight);
    }
    
    return rect;
}

void TitledWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    // Dessiner le fond (sans antialiasing)
    painter.fillRect(rect(), m_backgroundColor);

    // Dessiner le titre s'il est visible
    if (m_titleVisible && !m_title.isEmpty()) {
        QFont titleFont = font();
        titleFont.setPointSize(8); // Petite taille pour le titre
        painter.setFont(titleFont);

        QFontMetrics fm(titleFont);
        int textWidth = fm.horizontalAdvance(m_title) + 10; // Largeur du texte + marge
        int textHeight = fm.height() + 4; // Hauteur du texte + marge
        
        // Ajuster la largeur de la zone de titre si un widget compagnon est présent
        int titleBoxWidth = textWidth;
        if (m_titleCompanionWidget && m_titleCompanionWidget->isVisible()) {
            titleBoxWidth += m_titleCompanionWidget->width() + 15;  // Largeur du widget + marge
            m_titleCompanionWidget->setGeometry(
                textWidth + 1,  // Position X: après le titre avec une marge
                1,          // Position Y: centré verticalement dans la barre de titre
                m_titleCompanionWidget->sizeHint().width(),
                textHeight  // Hauteur légèrement réduite pour l'esthétique
            );
        }


        // Dessiner le fond du titre
        QRect titleRect(0, 0, textWidth, textHeight);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::black, 1)); // Utilise la couleur du titre pour le contour
        painter.drawRect(titleRect);

        // Réactiver l'antialiasing pour le texte
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.drawText(QRect(0, 0, textWidth, textHeight), Qt::AlignCenter, m_title);
    }

    // Dessiner le contour du widget
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::black, 1)); // Couleur et épaisseur du contour
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1)); // Ajuster pour que le contour soit à l'intérieur

    // Appeler la méthode virtuelle pour le contenu spécifique
    painter.setRenderHint(QPainter::Antialiasing, true);
    paintContent(painter, contentRect());
}