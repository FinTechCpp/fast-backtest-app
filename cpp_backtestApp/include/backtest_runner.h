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

#include "data_loader.h"
#include "baseview.h"  // Pour inclure la définition de BacktestResults

// Inclusions des fichiers d'en-tête du moteur de backtest C++
#include "backtest.hpp"
#include "broker.hpp"
#include "data.hpp"
#include "strategy.hpp"
#include "stats.hpp"
#include "trade.hpp"
#include "BuyHeikinGreen.hpp"  // Adaptateur pour la stratégie

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
    void finished(BacktestResults* results);
    void error(const QString& message);

private:
    App* m_mainWindow;
    
    // Méthodes d'aide pour le backtest C++
    std::shared_ptr<be::Strategy> createStrategy(std::shared_ptr<be::Broker> broker, 
                                               std::shared_ptr<be::Data> data,
                                               const QMap<QString, QVariant>& params);
    // Convertir les données OHLCBar en format be::Data
    std::shared_ptr<be::Data> convertToBeData(const std::vector<OHLCBar>& bars);
    
    // Maintenir les résultats en vie pendant l'exécution
    std::unique_ptr<BacktestResults> m_results;
};

