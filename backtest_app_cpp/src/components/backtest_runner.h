#ifndef BACKTEST_RUNNER_H
#define BACKTEST_RUNNER_H

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
#include "../data_loader.h"

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
    void backtestCompleted(void* data, void* stats);
    void backtestError(const QString& error);

public slots:
    void runBacktest();

private slots:
    void onBacktestFinished(void* data, void* stats);
    void onBacktestError(const QString& errorMessage);

private:
    // Membres dans l'ordre d'initialisation du constructeur
    App* m_mainWindow;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_runButton;
    QProgressBar* m_loadingIndicator;
    class BacktestWorker* m_worker;
    bool m_isRunning;
    
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
    void finished(void* data, void* stats);
    void error(const QString& message);

private:
    App* m_mainWindow;
};

#endif // BACKTEST_RUNNER_H