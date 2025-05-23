#include "backtest_runner.h"
#include "../binding/pybinding.h"
#include "../config_manager.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include "../app.h"

BacktestRunner::BacktestRunner(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<App*>(parent))
    , m_buttonLayout(nullptr)
    , m_runButton(nullptr)
    , m_stopButton(nullptr)
    , m_loadingIndicator(nullptr)
    , m_statusLabel(nullptr)
    , m_logOutput(nullptr)
    , m_progressTimer(nullptr)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isRunning(false)
{
    // Créer les composants UI
    createUIComponents();
    
    // Connecter les signaux internes
    connect(this, &BacktestRunner::backtestCompleted, this, &BacktestRunner::onBacktestFinished);
    connect(this, &BacktestRunner::backtestError, this, &BacktestRunner::onBacktestError);
    
    qDebug() << "BacktestRunner initialisé";
}

BacktestRunner::~BacktestRunner()
{
    // Nettoyer le thread worker s'il existe
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(5000);
    }
    
    delete m_workerThread;
    // Ne pas supprimer m_worker, il est auto-détruit quand le thread se termine
}

void BacktestRunner::createUIComponents()
{
    // Créer le layout pour le bouton
    m_buttonLayout = new QHBoxLayout();
    
    // Créer le bouton d'exécution
    m_runButton = new QPushButton("Lancer le backtest");
    m_runButton->setMinimumHeight(40);
    m_runButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    
    // CORRECTION: Connecter le signal clicked au slot runBacktest
    connect(m_runButton, &QPushButton::clicked, this, &BacktestRunner::runBacktest);
    
    // Créer l'indicateur de chargement
    m_loadingIndicator = new QProgressBar();
    m_loadingIndicator->setMaximum(0);  // Mode indéterminé
    m_loadingIndicator->setMinimum(0);
    m_loadingIndicator->setTextVisible(false);
    m_loadingIndicator->setMaximumHeight(10);
    m_loadingIndicator->setVisible(false);
    
    // Ajouter les widgets au layout
    m_buttonLayout->addWidget(m_runButton);
    m_buttonLayout->addWidget(m_loadingIndicator);
    
    qDebug() << "Composants UI créés pour BacktestRunner";
}

QHBoxLayout* BacktestRunner::getLayout() const
{
    return m_buttonLayout;
}

void BacktestRunner::runBacktest()
{
    if (m_isRunning) {
        qWarning() << "Un backtest est déjà en cours";
        return;
    }
    
    if (!m_mainWindow) {
        emit backtestError("Référence à la fenêtre principale manquante");
        return;
    }
    
    // Récupérer la configuration depuis l'application
    QMap<QString, QVariant> generalConfig = m_mainWindow->getConfigManager()->getProfileFromUI();
    QMap<QString, QVariant> strategyConfig = m_mainWindow->getStrategyConfig();
    
    // CORRECTION: Adapter aux paramètres attendus par le code Python
    QString symbol = generalConfig.value("symbol", "NDX").toString();
    QString interval = generalConfig.value("interval", "20secs").toString();
    QString period = generalConfig.value("period", "10d").toString();
    QString endDate = generalConfig.value("end_date", QDate::currentDate()).toDate().toString("dd/MM/yyyy");
    QTime tradingFrom = generalConfig.value("trading_from", QTime(15, 30)).toTime();
    QTime tradingTo = generalConfig.value("trading_to", QTime(22, 0)).toTime();
    
    double cash = generalConfig.value("cash", 100000.0).toDouble();
    double spread = generalConfig.value("spread", 0.0001).toDouble();
    QString strategyClass = generalConfig.value("strategy", "BuyHeikinGreenBA").toString();
    
    // Utiliser PyBindingManager pour l'exécution
    PyBindingManager* pyManager = PyBindingManager::getInstance();
    
    if (!pyManager || !pyManager->isInitialized()) {
        emit backtestError("Interface Python non disponible");
        return;
    }
    
    // Mettre à jour l'UI
    m_isRunning = true;
    m_runButton->setEnabled(false);
    m_loadingIndicator->setVisible(true);
    emit backtestStarted();
    
    // Lancer le backtest dans un thread séparé
    m_workerThread = new QThread();
    m_worker = new BacktestWorker(nullptr, strategyClass, cash, spread, strategyConfig);
    m_worker->moveToThread(m_workerThread);
    
    // CORRECTION: Adapter la logique pour charger les données d'abord
    connect(m_workerThread, &QThread::started, [this, symbol, interval, period, endDate, tradingFrom, tradingTo]() {
        // Charger les données d'abord
        PyBindingManager* pyManager = PyBindingManager::getInstance();
        void* data = pyManager->loadData(symbol, interval, period, endDate, tradingFrom, tradingTo);
        
        if (!data) {
            emit backtestError("Impossible de charger les données");
            return;
        }
        
        // Puis exécuter le backtest
        m_worker->m_data = data;
        m_worker->run();
    });
    
    connect(m_worker, &BacktestWorker::finished, this, &BacktestRunner::onBacktestFinished);
    connect(m_worker, &BacktestWorker::error, this, &BacktestRunner::onBacktestError);
    
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);
    
    m_workerThread->start();
}

void BacktestRunner::onBacktestFinished(void* data, void* stats)
{
    qInfo() << "=== BACKTEST TERMINÉ ===";
    
    try {
        // Traiter les résultats
        if (data && stats) {
            qInfo() << "Données reçues, mise à jour des vues...";
            
            // Mettre à jour l'interface des résultats via l'application principale
            if (m_mainWindow) {
                m_mainWindow->updateResultViews(data, stats);
            }
        } else {
            qWarning() << "Aucune donnée reçue du backtest";
            
            // Afficher un message dans les vues indiquant qu'aucune donnée n'est disponible
            if (m_mainWindow) {
                m_mainWindow->updateResultViews(nullptr, nullptr);
            }
        }
        
        qInfo() << "Backtest exécuté avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors du traitement des résultats:" << e.what();
        emit backtestError(QString("Erreur de traitement: %1").arg(e.what()));
    }
    
    // Réinitialiser l'interface
    m_loadingIndicator->setVisible(false);
    m_runButton->setEnabled(true);
    m_runButton->setText("Lancer le backtest");
    m_isRunning = false;
    
    qInfo() << "Interface réinitialisée";
}

void BacktestRunner::onBacktestError(const QString& errorMessage)
{
    qCritical() << "=== ERREUR BACKTEST ===" << errorMessage;
    
    // Afficher une boîte de dialogue pour informer l'utilisateur
    QMessageBox::warning(
        qobject_cast<QWidget*>(m_mainWindow), 
        "Erreur de backtest",
        QString("Le backtest a rencontré une erreur:\n%1").arg(errorMessage)
    );
    
    // Réinitialiser l'interface
    m_loadingIndicator->setVisible(false);
    m_runButton->setEnabled(true);
    m_runButton->setText("Lancer le backtest");
    m_isRunning = false;
    
    qInfo() << "Interface réinitialisée après erreur";
}

// Méthodes non implémentées (stubs)
void BacktestRunner::stopBacktest() 
{
    qDebug() << "stopBacktest appelé - non implémenté";
}

void BacktestRunner::updateProgress() 
{
    // Pour l'instant, ne fait rien
}

// Implémentation de BacktestWorker

BacktestWorker::BacktestWorker(void* data, 
                             const QString& strategyClass, 
                             double cash, 
                             double spread, 
                             const QMap<QString, QVariant>& strategyParams,
                             QObject* parent)
    : QObject(parent)
    , m_data(data)
    , m_strategyClass(strategyClass)
    , m_cash(cash)
    , m_spread(spread)
    , m_strategyParams(strategyParams)
{
}

void BacktestWorker::run()
{
    qDebug() << "BacktestWorker::run() - non implémenté";
    
    // Pour l'instant, simuler un délai et retourner des résultats factices
    QThread::msleep(1000);
    
    // Simuler des résultats
    QMap<QString, QVariant> mockStats;
    mockStats["Return [%]"] = 12.3;
    mockStats["# Trades"] = 25;
    
    emit finished(nullptr, &mockStats);
}

