#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QWidget>
#include <QRadioButton>
#include <QPainter>

class CandleWidget : public QWidget {
    Q_OBJECT
public:
    enum class OrderType { STOP_BUY, STOP_SELL, LIMIT_BUY, LIMIT_SELL };
    
    CandleWidget(OrderType type, QWidget* parent = nullptr);
    
    void setExecuteOnLevel(bool executeOnLevel);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    OrderType m_type;
    bool m_executeOnLevel = true;
    
    // Constantes pour le dessin
    const int m_candleWidth = 20;
    const int m_candleBodyWidth = 8;
    const int m_candleSpacing = 40;
    const int m_levelLineLength = 80;
    const QColor m_bullishColor = QColor(0, 200, 0); // Vert
    const QColor m_bearishColor = QColor(200, 0, 0); // Rouge
};

class BacktestEngineDialog : public QDialog {
    Q_OBJECT
    
public:
    BacktestEngineDialog(QWidget* parent = nullptr);
    
    bool executeStopOnOpen() const;
    bool executeLimitOnLimitPrice() const;
    
    void setExecuteStopOnOpen(bool value);
    void setExecuteLimitOnLimitPrice(bool value);
    
private:
    QRadioButton* m_stopOnOpenRadio;
    QRadioButton* m_stopOnLevelRadio;
    QRadioButton* m_limitOnLevelRadio;
    QRadioButton* m_limitOnOpenRadio;
    
    CandleWidget* m_stopBuyWidget;
    CandleWidget* m_stopSellWidget;
    CandleWidget* m_limitBuyWidget;
    CandleWidget* m_limitSellWidget;
    
private slots:
    void onStopExecutionChanged();
    void onLimitExecutionChanged();
};