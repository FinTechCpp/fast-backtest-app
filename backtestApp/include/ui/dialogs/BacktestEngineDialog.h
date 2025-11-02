#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QWidget>
#include <QRadioButton>
#include <QPainter>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include "beTypes.h"

class CandleWidget : public QWidget {
    Q_OBJECT
public:
    enum class OrderType { STOP_BUY, STOP_SELL, LIMIT_BUY, LIMIT_SELL, MARKET };
    
    CandleWidget(OrderType type, QWidget* parent = nullptr);
    
    void setExecuteOnLevel(bool executeOnLevel);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    OrderType m_type;
    bool m_executeOnLevel = true;
    
    // Constants for drawing
    const int m_candleWidth = 20;
    const int m_candleBodyWidth = 8;
    const int m_candleSpacing = 40;
    const int m_levelLineLength = 80;
    const QColor m_bullishColor = QColor(0, 200, 0); // green
    const QColor m_bearishColor = QColor(200, 0, 0); // red
};

class BacktestEngineDialog : public QDialog {
    Q_OBJECT
    
public:
    BacktestEngineDialog(QWidget* parent = nullptr);
    
    bool executeStopOnOpen() const;
    bool executeLimitOnLimitPrice() const;
    
    void setExecuteStopOnOpen(bool value);
    void setExecuteLimitOnLimitPrice(bool value);

    bool tradeOnClose() const;
    void setTradeOnClose(bool value);
    
    double leverageLimit() const;
    void setLeverageLimit(double value);
    
    be::PositionMode positionMode() const;
    void setPositionMode(be::PositionMode mode);
    
    bool finalizeTrades() const;
    void setFinalizeTrades(bool value);

    double spread() const;
    void setSpread(double value);

    double commission() const;
    void setCommission(double value);

    double cash() const;
    void setCash(double value);

    double spreadEntryRatio() const;
    void setSpreadEntryRatio(double value);

    double minPositionStep() const;
    void setMinPositionStep(double value);
    
private:
    QRadioButton* m_stopOnOpenRadio;
    QRadioButton* m_stopOnLevelRadio;
    QRadioButton* m_limitOnLevelRadio;
    QRadioButton* m_limitOnOpenRadio;
    
    CandleWidget* m_stopBuyWidget;
    CandleWidget* m_stopSellWidget;
    CandleWidget* m_limitBuyWidget;
    CandleWidget* m_limitSellWidget;

    QRadioButton* m_tradeOnCloseRadio;
    QRadioButton* m_tradeOnOpenRadio;
    CandleWidget* m_marketBuyWidget;
    CandleWidget* m_marketSellWidget;
    
    QDoubleSpinBox* m_spreadSpin;
    QDoubleSpinBox* m_commissionSpin;
    QDoubleSpinBox* m_cashSpin;
    QDoubleSpinBox* m_leverageSpin;
    QDoubleSpinBox* m_spreadEntryRatioSpin;
    QComboBox* m_positionModeCombo;
    QDoubleSpinBox* m_minPositionStepSpin;
    QCheckBox* m_finalizeTradesCheck;
    
private slots:
    void onStopExecutionChanged();
    void onLimitExecutionChanged();

    void onTradeOnCloseChanged();
};