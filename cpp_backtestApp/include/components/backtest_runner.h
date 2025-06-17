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

#include "components/data_loader.h"
#include "components/backtest_results.h"
#include "strategies/strategy_registry.h"

// Inclusions des fichiers d'en-tête du moteur de backtest C++
#include "backtest.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "strategy.hpp"
#include "stats.hpp"
#include "trade.hpp"
#include "BuyHeikinGreen.hpp"

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
    // Membres dans l'ordre d'initialisation du constructeur
    App* m_mainWindow;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_runButton;
    QProgressBar* m_loadingIndicator;
    QLabel* m_statsLabel;
    class BacktestWorker* m_worker;
    bool m_isRunning;
    
    // Variables pour capturer les dernières statistiques
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
    // Méthode pour transférer la propriété des résultats
    std::unique_ptr<BacktestResults> takeResults() {
        return std::move(m_results);
    }

private:
    App* m_mainWindow;
    
    // Méthodes d'aide pour le backtest C++
    std::shared_ptr<be::Strategy> createStrategy(std::shared_ptr<be::Broker> broker, 
                                               std::shared_ptr<be::Data> data,
                                               const QMap<QString, QVariant>& params);
    // Convertir les données OHLCBar en format be::Data
    std::shared_ptr<be::Data> convertToBeData(const std::vector<OHLCBar>& bars);
    std::vector<StrategyIndicator> extractIndicatorsFromConfig(const QMap<QString, QVariant>& params);

    
    // Maintenir les résultats en vie pendant l'exécution
    std::unique_ptr<BacktestResults> m_results;
};

