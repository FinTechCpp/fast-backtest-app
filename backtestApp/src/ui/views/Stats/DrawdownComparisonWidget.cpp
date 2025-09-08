#include "ui/views/Stats/DrawdownComparisonWidget.h"
#include <QVBoxLayout>
#include <QPainterPath>

DrawdownComparisonWidget::DrawdownComparisonWidget(QWidget* parent)
    : StatsBaseWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_groupBox = new QGroupBox("Analyse des Drawdowns");
    layout->addWidget(m_groupBox);
    
    // Hauteur minimale pour un bon rendu
    setMinimumHeight(200);
}

void DrawdownComparisonWidget::updateContent(const be::Stats& stats)
{
    m_maxDrawdownPct = stats.maxDrawdownPct;
    m_avgDrawdownPct = stats.avgDrawdownPct;
    m_maxDuration = QString::fromStdString(stats.maxDrawdownDuration.toString());
    m_avgDuration = QString::fromStdString(stats.avgDrawdownDuration.toString());
    
    // Convertir les durées en jours pour les barres
    m_maxDurationDays = static_cast<int>(stats.maxDrawdownDuration.toDays());
    m_avgDurationDays = static_cast<int>(stats.avgDrawdownDuration.toDays());

    update();
}

void DrawdownComparisonWidget::clear()
{
    m_maxDrawdownPct = 0.0;
    m_avgDrawdownPct = 0.0;
    m_maxDuration = "";
    m_avgDuration = "";
    m_maxDurationDays = 0;
    m_avgDurationDays = 0;
    
    update();
}

void DrawdownComparisonWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Dimensions
    const int width = m_groupBox->width() - 20;
    const int height = m_groupBox->height() - 40;
    const int leftMargin = 150;
    const int rightMargin = 80;
    const int topMargin = 40;
    const int barHeight = 25;
    const int barGap = 15;
    const int midGap = 30;
    
    // Position de départ (après le titre du groupe)
    int x = m_groupBox->x() + 10;
    int y = m_groupBox->y() + 30;
    
    // Zone de dessin
    int drawWidth = width - leftMargin - rightMargin;
    
    // Section Magnitude des Drawdowns (Pourcentage)
    // --------------------------------------------
    
    // Titre de section
    QFont sectionFont = painter.font();
    sectionFont.setBold(true);
    sectionFont.setPointSize(10);
    painter.setFont(sectionFont);
    painter.setPen(QColor(80, 80, 80));
    
    painter.drawText(x, y, "Magnitude (%)");
    
    // Sous-titre maximum
    QFont labelFont = painter.font();
    labelFont.setBold(false);
    labelFont.setPointSize(9);
    painter.setFont(labelFont);
    
    // Label pour Max DD
    painter.drawText(x, y + 25, leftMargin - 10, barHeight, Qt::AlignRight | Qt::AlignVCenter, "Maximum:");
    
    // Barre pour Max DD
    double maxDD = qAbs(m_maxDrawdownPct); // Utiliser la valeur absolue
    
    // Fond de la barre
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(230, 230, 230));
    QRect maxBarBg(x + leftMargin, y + 25, drawWidth, barHeight);
    painter.drawRoundedRect(maxBarBg, 4, 4);
    
    // Barre de valeur
    int maxBarWidth = drawWidth * (maxDD / 100.0); // Max théorique 100%
    maxBarWidth = qMin(maxBarWidth, drawWidth);
    
    painter.setBrush(QColor(80, 80, 80));
    QRect maxBar(x + leftMargin, y + 25, maxBarWidth, barHeight);
    painter.drawRoundedRect(maxBar, 4, 4);
    
    // Valeur textuelle
    painter.setPen(Qt::white);
    painter.drawText(maxBar, Qt::AlignCenter, QString("%1%").arg(maxDD, 0, 'f', 2));
    
    // Label pour Avg DD
    painter.setPen(QColor(80, 80, 80));
    painter.drawText(x, y + 25 + barHeight + barGap, leftMargin - 10, barHeight, Qt::AlignRight | Qt::AlignVCenter, "Moyenne:");
    
    // Barre pour Avg DD
    double avgDD = qAbs(m_avgDrawdownPct);
    
    // Fond de la barre
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(230, 230, 230));
    QRect avgBarBg(x + leftMargin, y + 25 + barHeight + barGap, drawWidth, barHeight);
    painter.drawRoundedRect(avgBarBg, 4, 4);
    
    // Barre de valeur
    int avgBarWidth = drawWidth * (avgDD / 100.0);
    avgBarWidth = qMin(avgBarWidth, drawWidth);
    
    painter.setBrush(QColor(150, 150, 150)); // Plus clair pour la moyenne
    QRect avgBar(x + leftMargin, y + 25 + barHeight + barGap, avgBarWidth, barHeight);
    painter.drawRoundedRect(avgBar, 4, 4);
    
    // Valeur textuelle
    painter.setPen(Qt::white);
    painter.drawText(avgBar, Qt::AlignCenter, QString("%1%").arg(avgDD, 0, 'f', 2));
    
    // Section Durée des Drawdowns
    // --------------------------
    int yDuration = y + 25 + 2 * (barHeight + barGap) + midGap;
    
    // Titre de section
    painter.setFont(sectionFont);
    painter.setPen(QColor(80, 80, 80));
    painter.drawText(x, yDuration, "Durée");
    
    // Déterminer l'échelle (max entre durée max et 30 jours)
    int maxScale = qMax(m_maxDurationDays, 30);
    
    // Label pour Max Duration
    painter.setFont(labelFont);
    painter.drawText(x, yDuration + 25, leftMargin - 10, barHeight, 
                     Qt::AlignRight | Qt::AlignVCenter, "Maximum:");
    
    // Barre pour Max Duration
    // Fond de la barre
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(230, 230, 230));
    QRect maxDurBg(x + leftMargin, yDuration + 25, drawWidth, barHeight);
    painter.drawRoundedRect(maxDurBg, 4, 4);
    
    // Barre de valeur
    int maxDurWidth = drawWidth * (m_maxDurationDays / static_cast<double>(maxScale));
    maxDurWidth = qMin(maxDurWidth, drawWidth);
    
    painter.setBrush(QColor(80, 80, 80));
    QRect maxDurBar(x + leftMargin, yDuration + 25, maxDurWidth, barHeight);
    painter.drawRoundedRect(maxDurBar, 4, 4);
    
    // Valeur textuelle
    painter.setPen(Qt::white);
    painter.drawText(maxDurBar, Qt::AlignCenter, m_maxDuration);
    
    // Label pour Avg Duration
    painter.setPen(QColor(80, 80, 80));
    painter.drawText(x, yDuration + 25 + barHeight + barGap, leftMargin - 10, barHeight, 
                     Qt::AlignRight | Qt::AlignVCenter, "Moyenne:");
    
    // Barre pour Avg Duration
    // Fond de la barre
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(230, 230, 230));
    QRect avgDurBg(x + leftMargin, yDuration + 25 + barHeight + barGap, drawWidth, barHeight);
    painter.drawRoundedRect(avgDurBg, 4, 4);
    
    // Barre de valeur
    int avgDurWidth = drawWidth * (m_avgDurationDays / static_cast<double>(maxScale));
    avgDurWidth = qMin(avgDurWidth, drawWidth);
    
    painter.setBrush(QColor(150, 150, 150)); // Plus clair pour la moyenne
    QRect avgDurBar(x + leftMargin, yDuration + 25 + barHeight + barGap, avgDurWidth, barHeight);
    painter.drawRoundedRect(avgDurBar, 4, 4);
    
    // Valeur textuelle
    painter.setPen(Qt::white);
    painter.drawText(avgDurBar, Qt::AlignCenter, m_avgDuration);
    
    // Échelle des barres
    painter.setPen(QColor(120, 120, 120));
    painter.setFont(QFont(painter.font().family(), 8));
    
    // Échelle des pourcentages (en haut)
    painter.drawText(x + leftMargin, y + 5, "0%");
    painter.drawText(x + leftMargin + drawWidth - 30, y + 5, "100%");
    
    // Échelle des durées (en bas)
    painter.drawText(x + leftMargin, yDuration + 5, "0j");
    painter.drawText(x + leftMargin + drawWidth - 30, yDuration + 5, 
                     QString("%1j").arg(maxScale));
}