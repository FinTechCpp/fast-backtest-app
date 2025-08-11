#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QThread>
#include <QMap>
#include <QString>
#include <QVariant>

#include "components/Utils/dataLoader.h"
#include "components/backtestResults.h"
#include "components/strategyRegistry.h"

// Include necessary headers for the backtest components
#include "backtest.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "strategy.hpp"
#include "stats.hpp"
#include "trade.hpp"
#include "buy_heikin_green.hpp"

// Forward declarations
class App;

class BacktestRunner : public QObject
{
    Q_OBJECT

public:
    BacktestRunner(QObject* parent = nullptr);
    ~BacktestRunner();
    
    QHBoxLayout* getLayout() const;

signals:
    void backtestStarted();
    void backtestCompleted(BacktestResults* results);
    void backtestError(const QString& error);

public slots:
    void runBacktest();

private slots:
    void onBacktestFinished(BacktestResults* results);
    void onBacktestError(const QString& errorMessage);
    void onProgressUpdated(int current, int total, const QString& chrono);

private:
    // Members sorted by order of initialization in the constructor
    App* m_mainWindow;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_runButton;
    QProgressBar* m_loadingIndicator;
    QLabel* m_statsLabel;
    class BacktestWorker* m_worker;
    bool m_isRunning;
    
    // Variables to store latest stats while backtest execution
    int m_lastTotalCandles;
    QString m_lastChrono;
    
    void createUIComponents();
    void resetUI();
    void showError(const QString& error);
};

class BacktestWorker : public QThread
{
    Q_OBJECT

public:
    BacktestWorker(App* mainWindow, QObject* parent = nullptr);
    ~BacktestWorker();

protected:
    void run() override;

signals:
    void finished(BacktestResults* results);
    void error(const QString& message);
    void progressUpdated(int current, int total, const QString& chrono);

public:
    // Method to transfer ownership of the results
    std::unique_ptr<BacktestResults> takeResults() {
        return std::move(m_results);
    }

private:
    App* m_mainWindow;
    
    // Convert OHLCBar data to be::Data format
    std::shared_ptr<be::Data> convertToBeData(const std::vector<OHLCBar>& bars);


    // Keep the results alive during execution
    std::unique_ptr<BacktestResults> m_results;
};

