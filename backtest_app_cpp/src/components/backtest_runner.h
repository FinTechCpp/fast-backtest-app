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

// Forward declarations
class App;

class BacktestRunner : public QObject
{
    Q_OBJECT

public:
    BacktestRunner(QObject* parent = nullptr);
    ~BacktestRunner();
    
    QHBoxLayout* getLayout() const; // CORRECTION: Déclaration seulement, pas de définition inline

signals:
    void backtestStarted();
    void backtestCompleted(void* data, void* stats);
    void backtestError(const QString& error);
    void progressUpdated(int percentage);

public slots:
    void runBacktest();
    void stopBacktest();

private slots:
    void updateProgress();
    void onBacktestFinished(void* data, void* stats);
    void onBacktestError(const QString& errorMessage);

private:
    // CORRECTION: Réorganiser l'ordre des membres pour éviter les warnings -Wreorder
    // L'ordre doit correspondre à l'ordre de déclaration dans le constructeur
    App* m_mainWindow;              // Premier dans l'ordre d'initialisation
    QHBoxLayout* m_buttonLayout;    // Deuxième
    QPushButton* m_runButton;
    QPushButton* m_stopButton;
    QProgressBar* m_loadingIndicator;
    QLabel* m_statusLabel;
    QTextEdit* m_logOutput;
    QTimer* m_progressTimer;
    QThread* m_workerThread;        // Avant-dernier
    class BacktestWorker* m_worker; // Dernier
    bool m_isRunning;
    
    void createUIComponents();
    void setupUI();
    void resetUI();
    void showError(const QString& error);
    void showSuccess();
};

// Classe BacktestWorker dans un fichier séparé ou en forward declaration
class BacktestWorker : public QObject
{
    Q_OBJECT

public:
    BacktestWorker(void* data, 
                   const QString& strategyClass, 
                   double cash, 
                   double spread, 
                   const QMap<QString, QVariant>& strategyParams,
                   QObject* parent = nullptr);

public slots:
    void run();

signals:
    void finished(void* data, void* stats);
    void error(const QString& errorMessage);

public:  // CORRECTION: Rendre les membres publics
    void* m_data;
    QString m_strategyClass;
    double m_cash;
    double m_spread;
    QMap<QString, QVariant> m_strategyParams;
};

#endif // BACKTEST_RUNNER_H