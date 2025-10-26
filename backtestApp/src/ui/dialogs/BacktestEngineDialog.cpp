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
    setMinimumSize(200, 200);
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
    int labelHeight = 30;
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
        case OrderType::MARKET: typeName = "MARKET"; break;
    }

    painter.drawText(rect().adjusted(0, 10, 0, -h + labelHeight), Qt::AlignLeft, typeName);

    // Dessiner les bougies
    QColor prevColor, curColor;
    int levelY = midY; // Position Y du niveau d'ordre
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
    else if (m_type == OrderType::STOP_SELL || m_type == OrderType::LIMIT_BUY) {
        // Bougies rouges descendantes
        prevColor = m_bearishColor; // Bougie précédente rouge
        curColor = m_bearishColor; // Bougie actuelle rouge

        candle1Top = offset;
        candle2Top = midY + offset;
        candle2Open = candle2Top + candleHeight / 4;
    }
    else if (m_type == OrderType::MARKET) {
        prevColor = m_bullishColor;
        curColor = m_bearishColor;

        candle1Top = midY - candleHeight / 4;
        candle2Top = midY - candleHeight / 2;
        candle2Open = candle2Top + candleHeight / 4;

        // Il faut que le niveau de cloture soit a midY
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


    // Dessiner le point d'exécution avec une flèche
    int execY = m_executeOnLevel ? levelY : candle2Open;
    int execX = m_executeOnLevel ? candle1X : candle2X;

    if (m_type != OrderType::MARKET) {
        // Dessiner le niveau d'ordre (STOP ou LIMIT) avec la couleur appropriée
        QColor levelColor = isStop ? m_bearishColor.lighter(110) : m_bullishColor.lighter(110);
        painter.setPen(QPen(levelColor, 2));
        painter.drawLine(candle1X - m_candleWidth, levelY, 
                        candle2X + m_candleWidth, levelY);

        if (m_executeOnLevel) {
            execX -= m_candleWidth; // Si exécution au niveau, la flèche est à gauche
        }
    }


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
    setMinimumSize(950, 750);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Grid pour la partie supérieure (2x2)
    QGridLayout* topGridLayout = new QGridLayout();
    topGridLayout->setColumnStretch(0, 1); // Colonne gauche prend 50%
    topGridLayout->setColumnStretch(1, 1); // Colonne droite prend 50%
    topGridLayout->setHorizontalSpacing(10);
    topGridLayout->setVerticalSpacing(10);

    // Section des paramètres supplémentaires du backtest (colonne gauche)
    QGroupBox* additionalParamsGroupBox = new QGroupBox("Paramètres additionnels", this);
    QFormLayout* additionalParamsLayout = new QFormLayout(additionalParamsGroupBox);

    // Spread (en pour mille)
    m_spreadSpin = new QDoubleSpinBox(this);
    m_spreadSpin->setDecimals(3);
    m_spreadSpin->setRange(0, 10);
    m_spreadSpin->setSingleStep(0.01);
    m_spreadSpin->setValue(0.100);
    additionalParamsLayout->addRow(new QLabel("Spread (‰):", this), m_spreadSpin);

    // Ratio du spread utilisé pour le prix d'entrée
    m_spreadEntryRatioSpin = new QDoubleSpinBox(this);
    m_spreadEntryRatioSpin->setDecimals(2);
    m_spreadEntryRatioSpin->setRange(0.0, 1.0);
    m_spreadEntryRatioSpin->setSingleStep(0.05);
    m_spreadEntryRatioSpin->setValue(0.5);
    additionalParamsLayout->addRow(new QLabel("Ratio du spread pour prix d'entrée (0.0 - 1.0):", this), m_spreadEntryRatioSpin);

    // Commission (en pourcentage)
    m_commissionSpin = new QDoubleSpinBox(this);
    m_commissionSpin->setDecimals(2);
    m_commissionSpin->setRange(0.0, 100.0);
    m_commissionSpin->setSingleStep(0.01);
    m_commissionSpin->setValue(0.0);
    additionalParamsLayout->addRow(new QLabel("Commission (%):", this), m_commissionSpin);
    
    // Cash initial
    m_cashSpin = new QDoubleSpinBox(this);
    m_cashSpin->setDecimals(2);
    m_cashSpin->setRange(10, 10000000);
    m_cashSpin->setSingleStep(1000);
    m_cashSpin->setValue(10000);
    additionalParamsLayout->addRow(new QLabel("Cash initial:", this), m_cashSpin);

    // Levier maximal
    m_leverageSpin = new QDoubleSpinBox(this);
    m_leverageSpin->setDecimals(2);
    m_leverageSpin->setRange(1, 10000);
    m_leverageSpin->setSingleStep(1);
    m_leverageSpin->setValue(20);
    additionalParamsLayout->addRow(new QLabel("Levier maximal:", this), m_leverageSpin);
    
    // Position Mode
    m_positionModeCombo = new QComboBox(this);
    m_positionModeCombo->addItems({"Hedging", "Netting"});
    additionalParamsLayout->addRow(new QLabel("Mode de position:", this), m_positionModeCombo);

    // Taille minimale de position (quantification)
    m_minPositionStepSpin = new QDoubleSpinBox(this);
    m_minPositionStepSpin->setDecimals(2);
    m_minPositionStepSpin->setRange(0.0, 10000);
    m_minPositionStepSpin->setSingleStep(0.1);
    m_minPositionStepSpin->setValue(0.5);
    additionalParamsLayout->addRow(new QLabel("Quantification de position:", this), m_minPositionStepSpin);
    
    // Finalize Trades
    m_finalizeTradesCheck = new QCheckBox("Finaliser les trades en fin de backtest", this);
    m_finalizeTradesCheck->setChecked(true);
    additionalParamsLayout->addRow(m_finalizeTradesCheck);
    
    topGridLayout->addWidget(additionalParamsGroupBox, 0, 0);
    
    // Section Trade on Close (colonne droite)
    QGroupBox* tradeOnCloseGroupBox = new QGroupBox("Trade on Close", this);
    QVBoxLayout* tradeOnCloseLayout = new QVBoxLayout(tradeOnCloseGroupBox);
    
    QLabel* tradeOnCloseLabel = new QLabel(
        "Si activé, les ordres MARKET sont exécutés au prix de clôture de la bougie précédente. "
        "Sinon, ils sont exécutés au prix d'ouverture de la bougie actuelle.", this);
    tradeOnCloseLabel->setWordWrap(true); // Important : permet le retour à la ligne automatique
    tradeOnCloseLayout->addWidget(tradeOnCloseLabel);
    
    QButtonGroup* tradeOnCloseGroup = new QButtonGroup(this);
    m_tradeOnCloseRadio = new QRadioButton("Activer Trade on Close", this);
    m_tradeOnOpenRadio = new QRadioButton("Exécuter au prix d'ouverture", this);
    tradeOnCloseGroup->addButton(m_tradeOnCloseRadio);
    tradeOnCloseGroup->addButton(m_tradeOnOpenRadio);
    
    tradeOnCloseLayout->addWidget(m_tradeOnCloseRadio);
    tradeOnCloseLayout->addWidget(m_tradeOnOpenRadio);
    
    // Visualisations Trade on Close
    m_marketBuyWidget = new CandleWidget(CandleWidget::OrderType::MARKET, this);
    // m_marketBuyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    tradeOnCloseLayout->addWidget(m_marketBuyWidget);
    tradeOnCloseLayout->addStretch();
    topGridLayout->addWidget(tradeOnCloseGroupBox, 0, 1);
    
    mainLayout->addLayout(topGridLayout);
    
    // Grid pour la partie inférieure (STOP et LIMIT côte à côte)
    QGridLayout* bottomGridLayout = new QGridLayout();
    bottomGridLayout->setColumnStretch(0, 1); // Colonne gauche prend 50%
    bottomGridLayout->setColumnStretch(1, 1); // Colonne droite prend 50%
    bottomGridLayout->setHorizontalSpacing(10);
    
    // Section STOP Orders (colonne gauche)
    QGroupBox* stopGroupBox = new QGroupBox("Exécution des ordres STOP lors d'un gap", this);
    QVBoxLayout* stopLayout = new QVBoxLayout(stopGroupBox);
    
    QLabel* stopLabel = new QLabel("Niveau de prix d'exécution:", this);
    stopLayout->addWidget(stopLabel);
    
    QButtonGroup* stopGroup = new QButtonGroup(this);
    m_stopOnOpenRadio = new QRadioButton("Prix d'ouverture (pire cas)", this);
    m_stopOnLevelRadio = new QRadioButton("Niveau du STOP", this);
    stopGroup->addButton(m_stopOnOpenRadio);
    stopGroup->addButton(m_stopOnLevelRadio);
    
    stopLayout->addWidget(m_stopOnOpenRadio);
    stopLayout->addWidget(m_stopOnLevelRadio);
    
    // Visualisations STOP
    QHBoxLayout* stopVisualsLayout = new QHBoxLayout();
    m_stopSellWidget = new CandleWidget(CandleWidget::OrderType::STOP_SELL, this);
    m_stopBuyWidget = new CandleWidget(CandleWidget::OrderType::STOP_BUY, this);
    // m_stopSellWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // m_stopBuyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    stopVisualsLayout->addWidget(m_stopSellWidget);
    stopVisualsLayout->addWidget(m_stopBuyWidget);
    
    stopLayout->addLayout(stopVisualsLayout);
    stopLayout->addStretch();
    bottomGridLayout->addWidget(stopGroupBox, 0, 0);
    
    // Section LIMIT Orders (colonne droite)
    QGroupBox* limitGroupBox = new QGroupBox("Exécution des ordres LIMIT lors d'un gap", this);
    QVBoxLayout* limitLayout = new QVBoxLayout(limitGroupBox);
    
    QLabel* limitLabel = new QLabel("Niveau de prix d'exécution:", this);
    limitLayout->addWidget(limitLabel);
    
    QButtonGroup* limitGroup = new QButtonGroup(this);
    m_limitOnLevelRadio = new QRadioButton("Niveau du LIMIT (pire cas)", this);
    m_limitOnOpenRadio = new QRadioButton("Prix d'ouverture", this);
    limitGroup->addButton(m_limitOnLevelRadio);
    limitGroup->addButton(m_limitOnOpenRadio);
    
    limitLayout->addWidget(m_limitOnLevelRadio);
    limitLayout->addWidget(m_limitOnOpenRadio);
    
    // Visualisations LIMIT
    QHBoxLayout* limitVisualsLayout = new QHBoxLayout();
    m_limitSellWidget = new CandleWidget(CandleWidget::OrderType::LIMIT_SELL, this);
    m_limitBuyWidget = new CandleWidget(CandleWidget::OrderType::LIMIT_BUY, this);
    // m_limitSellWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // m_limitBuyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    limitVisualsLayout->addWidget(m_limitSellWidget);
    limitVisualsLayout->addWidget(m_limitBuyWidget);
    
    limitLayout->addLayout(limitVisualsLayout);
    limitLayout->addStretch();
    bottomGridLayout->addWidget(limitGroupBox, 0, 1);
    
    mainLayout->addLayout(bottomGridLayout);
    
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
    connect(m_tradeOnCloseRadio, &QRadioButton::toggled, this, &BacktestEngineDialog::onTradeOnCloseChanged);

    
    // Valeurs par défaut
    m_stopOnOpenRadio->setChecked(true);
    m_limitOnLevelRadio->setChecked(true);
    m_tradeOnOpenRadio->setChecked(true);
    
    // Mettre à jour les visualisations
    onStopExecutionChanged();
    onLimitExecutionChanged();
    onTradeOnCloseChanged();
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

void BacktestEngineDialog::onTradeOnCloseChanged()
{
    bool tradeOnClose = m_tradeOnCloseRadio->isChecked();
    m_marketBuyWidget->setExecuteOnLevel(tradeOnClose);
    // m_marketSellWidget->setExecuteOnLevel(tradeOnClose);
}

bool BacktestEngineDialog::tradeOnClose() const
{
    return m_tradeOnCloseRadio->isChecked();
}

void BacktestEngineDialog::setTradeOnClose(bool value)
{
    m_tradeOnCloseRadio->setChecked(value);
    m_tradeOnOpenRadio->setChecked(!value);
}

double BacktestEngineDialog::leverageLimit() const
{
    return m_leverageSpin->value();
}

void BacktestEngineDialog::setLeverageLimit(double value)
{
    m_leverageSpin->setValue(value);
}

be::PositionMode BacktestEngineDialog::positionMode() const
{
    return static_cast<be::PositionMode>(m_positionModeCombo->currentIndex());
}

void BacktestEngineDialog::setPositionMode(be::PositionMode mode)
{
    m_positionModeCombo->setCurrentIndex(static_cast<int>(mode));
}

bool BacktestEngineDialog::finalizeTrades() const
{
    return m_finalizeTradesCheck->isChecked();
}

void BacktestEngineDialog::setFinalizeTrades(bool value)
{
    m_finalizeTradesCheck->setChecked(value);
}

double BacktestEngineDialog::spread() const
{
    return m_spreadSpin->value();
}

void BacktestEngineDialog::setSpread(double value)
{
    m_spreadSpin->setValue(value);
}

double BacktestEngineDialog::commission() const
{
    return m_commissionSpin->value();
}

void BacktestEngineDialog::setCommission(double value)
{
    m_commissionSpin->setValue(value);
}

double BacktestEngineDialog::cash() const
{
    return m_cashSpin->value();
}

void BacktestEngineDialog::setCash(double value)
{
    m_cashSpin->setValue(value);
}

double BacktestEngineDialog::spreadEntryRatio() const
{
    return m_spreadEntryRatioSpin->value();
}

void BacktestEngineDialog::setSpreadEntryRatio(double value)
{
    m_spreadEntryRatioSpin->setValue(value);
}

double BacktestEngineDialog::minPositionStep() const
{
    return m_minPositionStepSpin->value();
}

void BacktestEngineDialog::setMinPositionStep(double value)
{
    m_minPositionStepSpin->setValue(value);
}