#include "ui/dialogs/BacktestEngineDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QButtonGroup>
#include <QPaintEvent>

// Implémentation de CandleWidget
CandleWidget::CandleWidget(OrderType type, QWidget* parent)
    : QWidget(parent), m_type(type)
{
    setMinimumSize(160, 180);
}

void CandleWidget::setExecuteOnLevel(bool executeOnLevel)
{
    m_executeOnLevel = executeOnLevel;
    update(); // Redessine le widget
}

void CandleWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    int midY = h / 2;
    int offset = 10; // Marge du haut et du bas de chaque bougie


    // Position de la première bougie (précédente)
    int candle1X = (w - m_candleSpacing) / 2;
    
    // Position de la seconde bougie (actuelle)
    int candle2X = (w + m_candleSpacing) / 2;
    
    // Hauteur des bougies
    int candleHeight = h / 2 - 2 * offset;
    
    // Légende au-dessus des bougies
    painter.setPen(Qt::black);
    QFont legendFont = painter.font();
    legendFont.setBold(true);
    painter.setFont(legendFont);
    QString typeName;
    switch(m_type) {
        case OrderType::STOP_BUY: typeName = "STOP BUY"; break;
        case OrderType::STOP_SELL: typeName = "STOP SELL"; break;
        case OrderType::LIMIT_BUY: typeName = "LIMIT BUY"; break;
        case OrderType::LIMIT_SELL: typeName = "LIMIT SELL"; break;
    }

    painter.drawText(rect().adjusted(0, 10, 0, -h + 30), Qt::AlignLeft, typeName);

    // Dessiner les bougies
    QColor prevColor, curColor;
    int levelY = midY; // Position Y du niveau d'ordre
    bool isBuy = (m_type == OrderType::STOP_BUY || m_type == OrderType::LIMIT_BUY);
    bool isStop = (m_type == OrderType::STOP_BUY || m_type == OrderType::STOP_SELL);

    int candle1Top;
    int candle2Top;
    int candle2Open;
    
    // Déterminer les couleurs et positions selon le type d'ordre
    if (m_type == OrderType::STOP_BUY || m_type == OrderType::LIMIT_SELL) {
        // Bougies vertes montantes
        prevColor = m_bullishColor; // Bougie précédente verte
        curColor = m_bullishColor; // Bougie actuelle verte

        candle1Top = midY + offset;
        candle2Top = offset;
        candle2Open = candle2Top + 3 * candleHeight / 4;
    } 
    else { // STOP_SELL ou LIMIT_BUY
        // Bougies rouges descendantes
        prevColor = m_bearishColor; // Bougie précédente rouge
        curColor = m_bearishColor; // Bougie actuelle rouge

        candle1Top = offset;
        candle2Top = midY + offset;
        candle2Open = candle2Top + candleHeight / 4;
    }
    
    int candle1Bottom = candle1Top + candleHeight;
    int candle1BodyTop = candle1Top + candleHeight / 4;

    int candle2Bottom = candle2Top + candleHeight;
    int candle2BodyTop = candle2Top + candleHeight / 4;
    
    
    // Dessiner la première bougie (précédente)
    painter.setPen(QPen(prevColor, 1));
    painter.setBrush(prevColor);
    
    // Corps de la bougie
    painter.drawRect(candle1X - m_candleBodyWidth/2, candle1BodyTop, 
                    m_candleBodyWidth, candleHeight/2);
    
    // Mèche haute et basse
    painter.drawLine(candle1X, candle1Top, 
                     candle1X, candle1Bottom);
    
    // Dessiner la seconde bougie (actuelle)
    painter.setPen(QPen(curColor, 1));
    painter.setBrush(curColor);
    
    // Corps de la bougie
    painter.drawRect(candle2X - m_candleBodyWidth/2, candle2BodyTop, 
                    m_candleBodyWidth, candleHeight/2);
    
    // Mèche haute et basse
    painter.drawLine(candle2X, candle2Top, 
                     candle2X, candle2Bottom);
    
    // Dessiner le niveau d'ordre (STOP ou LIMIT) avec la couleur appropriée
    QColor levelColor = isStop ? m_bearishColor.lighter(110) : m_bullishColor.lighter(110);
    painter.setPen(QPen(levelColor, 2));
    painter.drawLine(candle1X - m_candleWidth, levelY, 
                     candle2X + m_candleWidth, levelY);

    
    // Dessiner le point d'exécution avec une flèche
    int execY = m_executeOnLevel ? levelY : candle2Open;
    int execX = m_executeOnLevel ? candle1X - m_candleWidth : candle2X;

    execX -= 5; // Décalage vers la gauche pour la flèche
    
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(Qt::black);
    
    // Flèche d'exécution
    QPolygon arrow;
    arrow << QPoint(execX, execY) 
          << QPoint(execX - 8, execY - 4) 
          << QPoint(execX - 8, execY + 4);
    painter.drawPolygon(arrow);
}

// Implémentation du dialogue
BacktestEngineDialog::BacktestEngineDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Configuration avancée du backtest");
    setMinimumSize(580, 700);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Section STOP Orders
    QGroupBox* stopGroupBox = new QGroupBox("Exécution des ordres STOP", this);
    QVBoxLayout* stopLayout = new QVBoxLayout(stopGroupBox);
    
    QLabel* stopLabel = new QLabel("Sélectionnez le niveau de prix auquel les ordres STOP seront exécutés:", this);
    stopLayout->addWidget(stopLabel);
    
    QButtonGroup* stopGroup = new QButtonGroup(this);
    m_stopOnOpenRadio = new QRadioButton("Exécuter au prix d'ouverture (pire cas)", this);
    m_stopOnLevelRadio = new QRadioButton("Exécuter au niveau du STOP", this);
    stopGroup->addButton(m_stopOnOpenRadio);
    stopGroup->addButton(m_stopOnLevelRadio);
    
    stopLayout->addWidget(m_stopOnOpenRadio);
    stopLayout->addWidget(m_stopOnLevelRadio);
    
    // Visualisations STOP
    QHBoxLayout* stopVisualsLayout = new QHBoxLayout();
    m_stopSellWidget = new CandleWidget(CandleWidget::OrderType::STOP_SELL, this);
    m_stopBuyWidget = new CandleWidget(CandleWidget::OrderType::STOP_BUY, this);
    
    stopVisualsLayout->addWidget(m_stopSellWidget);
    stopVisualsLayout->addWidget(m_stopBuyWidget);
    
    stopLayout->addLayout(stopVisualsLayout);
    mainLayout->addWidget(stopGroupBox);
    
    // Section LIMIT Orders
    QGroupBox* limitGroupBox = new QGroupBox("Exécution des ordres LIMIT", this);
    QVBoxLayout* limitLayout = new QVBoxLayout(limitGroupBox);
    
    QLabel* limitLabel = new QLabel("Sélectionnez le niveau de prix auquel les ordres LIMIT seront exécutés:", this);
    limitLayout->addWidget(limitLabel);
    
    QButtonGroup* limitGroup = new QButtonGroup(this);
    m_limitOnLevelRadio = new QRadioButton("Exécuter au niveau du LIMIT (pire cas)", this);
    m_limitOnOpenRadio = new QRadioButton("Exécuter au prix d'ouverture", this);
    limitGroup->addButton(m_limitOnLevelRadio);
    limitGroup->addButton(m_limitOnOpenRadio);
    
    limitLayout->addWidget(m_limitOnLevelRadio);
    limitLayout->addWidget(m_limitOnOpenRadio);
    
    // Visualisations LIMIT
    QHBoxLayout* limitVisualsLayout = new QHBoxLayout();
    m_limitSellWidget = new CandleWidget(CandleWidget::OrderType::LIMIT_SELL, this);
    m_limitBuyWidget = new CandleWidget(CandleWidget::OrderType::LIMIT_BUY, this);
    
    limitVisualsLayout->addWidget(m_limitSellWidget);
    limitVisualsLayout->addWidget(m_limitBuyWidget);
    
    limitLayout->addLayout(limitVisualsLayout);
    mainLayout->addWidget(limitGroupBox);
    
    // Boutons OK / Annuler
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", this);
    QPushButton* cancelButton = new QPushButton("Annuler", this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connexions
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(m_stopOnOpenRadio, &QRadioButton::toggled, this, &BacktestEngineDialog::onStopExecutionChanged);
    connect(m_limitOnLevelRadio, &QRadioButton::toggled, this, &BacktestEngineDialog::onLimitExecutionChanged);
    
    // Valeurs par défaut
    m_stopOnOpenRadio->setChecked(true);
    m_limitOnLevelRadio->setChecked(true);
    
    // Mettre à jour les visualisations
    onStopExecutionChanged();
    onLimitExecutionChanged();
}

void BacktestEngineDialog::onStopExecutionChanged()
{
    bool executeOnLevel = m_stopOnLevelRadio->isChecked();
    m_stopBuyWidget->setExecuteOnLevel(executeOnLevel);
    m_stopSellWidget->setExecuteOnLevel(executeOnLevel);
}

void BacktestEngineDialog::onLimitExecutionChanged()
{
    bool executeOnLevel = m_limitOnLevelRadio->isChecked();
    m_limitBuyWidget->setExecuteOnLevel(executeOnLevel);
    m_limitSellWidget->setExecuteOnLevel(executeOnLevel);
}

bool BacktestEngineDialog::executeStopOnOpen() const
{
    return m_stopOnOpenRadio->isChecked();
}

bool BacktestEngineDialog::executeLimitOnLimitPrice() const
{
    return m_limitOnLevelRadio->isChecked();
}

void BacktestEngineDialog::setExecuteStopOnOpen(bool value)
{
    m_stopOnOpenRadio->setChecked(value);
    m_stopOnLevelRadio->setChecked(!value);
}

void BacktestEngineDialog::setExecuteLimitOnLimitPrice(bool value)
{
    m_limitOnLevelRadio->setChecked(value);
    m_limitOnOpenRadio->setChecked(!value);
}