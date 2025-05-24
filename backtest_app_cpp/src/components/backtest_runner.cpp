#include "backtest_runner.h"
#include "../binding/pybinding.h"
#include "../config_manager.h"
#include "../data_loader.h"
#include "../app.h"
#include <QDebug>

BacktestRunner::BacktestRunner(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<App*>(parent))
    , m_buttonLayout(nullptr)
    , m_runButton(nullptr)
    , m_loadingIndicator(nullptr)
    , m_worker(nullptr)
    , m_isRunning(false)
{
    createUIComponents();
}

BacktestRunner::~BacktestRunner()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->quit();
        m_worker->wait();
    }
    delete m_worker;
}

void BacktestRunner::createUIComponents()
{
    m_buttonLayout = new QHBoxLayout();
    
    m_runButton = new QPushButton("Lancer le backtest");
    m_runButton->setMinimumHeight(40);
    m_runButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    
    connect(m_runButton, &QPushButton::clicked, this, &BacktestRunner::runBacktest);
    
    m_loadingIndicator = new QProgressBar();
    m_loadingIndicator->setMaximum(0);
    m_loadingIndicator->setMinimum(0);
    m_loadingIndicator->setTextVisible(false);
    m_loadingIndicator->setVisible(false);
    
    m_buttonLayout->addWidget(m_runButton);
    m_buttonLayout->addWidget(m_loadingIndicator);
}

void BacktestRunner::runBacktest()
{
    if (m_isRunning) {
        return;
    }
    
    // Récupérer la configuration depuis l'application
    if (!m_mainWindow) {
        showError("Référence à l'application principale non trouvée");
        return;
    }
    
    ConfigManager* configManager = m_mainWindow->getConfigManager();
    if (!configManager) {
        showError("Gestionnaire de configuration non trouvé");
        return;
    }
    
    // Interface utilisateur
    m_runButton->setEnabled(false);
    m_loadingIndicator->setVisible(true);
    m_isRunning = true;
    
    emit backtestStarted();
    
    // Récupérer les paramètres
    QMap<QString, QVariant> allParams = m_mainWindow->getStrategyConfig();
    
    // Récupérer la date de fin depuis la configuration
    QDateTime endDate;
    if (allParams.contains("end_date")) {
        QVariant dateVariant = allParams["end_date"];
        if (dateVariant.type() == QVariant::Date) {
            endDate = QDateTime(dateVariant.toDate());
        } else if (dateVariant.type() == QVariant::DateTime) {
            endDate = dateVariant.toDateTime();
        } else {
            // Tenter de parser une chaîne
            QString dateStr = dateVariant.toString();
            endDate = QDateTime::fromString(dateStr, "dd/MM/yyyy");
        }
    }
    
    // Charger les données avec la bonne date
    std::vector<OHLCBar> data = DataLoader::loadData(
        "NDX", "20secs", "10d", endDate, QTime(), QTime(), {}
    );
    
    if (data.empty()) {
        showError("Aucune donnée chargée");
        resetUI();
        return;
    }
    
    QString strategyClass = allParams.value("strategy", "BuyHeikinGreenBA").toString();
    double cash = allParams.value("cash", 100000.0).toDouble();
    double spread = allParams.value("spread", 0.0001).toDouble();
    
    // Créer et lancer le worker
    m_worker = new BacktestWorker(data, strategyClass, cash, spread, allParams, this);
    
    connect(m_worker, &BacktestWorker::finished, this, &BacktestRunner::onBacktestFinished);
    connect(m_worker, &BacktestWorker::error, this, &BacktestRunner::onBacktestError);
    connect(m_worker, &QThread::finished, m_worker, &QObject::deleteLater);
    
    m_worker->start();
}

void BacktestRunner::onBacktestFinished(void* data, void* stats)
{
    m_isRunning = false;
    resetUI();
    
    if (!stats) {
        qCritical() << "Pas de résultats de backtest reçus";
        showError("Aucun résultat de backtest reçu");
        return;
    }
    
    qInfo() << "Backtest terminé avec succès, transmission des résultats";
    
    // Transmettre les résultats au gestionnaire principal
    if (m_mainWindow) {
        m_mainWindow->updateResultViews(data, stats);
        emit backtestCompleted(data, stats);
    } else {
        qCritical() << "Impossible de transmettre les résultats : fenêtre principale non disponible";
    }
}

void BacktestRunner::onBacktestError(const QString& errorMessage)
{
    resetUI();
    showError(errorMessage);
    emit backtestError(errorMessage);
}

void BacktestRunner::resetUI()
{
    m_runButton->setEnabled(true);
    m_loadingIndicator->setVisible(false);
    m_isRunning = false;
}

void BacktestRunner::showError(const QString& error)
{
    qCritical() << "Erreur backtest:" << error;
}

QHBoxLayout* BacktestRunner::getLayout() const
{
    return m_buttonLayout;
}

// BacktestWorker - Version simplifiée
BacktestWorker::BacktestWorker(const std::vector<OHLCBar>& data,
                               const QString& strategyClass, 
                               double cash, 
                               double spread, 
                               const QMap<QString, QVariant>& strategyParams,
                               QObject* parent)
    : QThread(parent)
    , m_data(data)
    , m_strategyClass(strategyClass)
    , m_cash(cash)
    , m_spread(spread)
    , m_strategyParams(strategyParams)
{
}

// AJOUT: Implémentation du destructeur manquant
BacktestWorker::~BacktestWorker()
{
    // S'assurer que le thread est terminé avant destruction
    if (isRunning()) {
        quit();
        wait(5000); // Attendre maximum 5 secondes
        if (isRunning()) {
            terminate(); // Forcer l'arrêt si nécessaire
            wait(1000);
        }
    }
}

void BacktestWorker::run()
{
    try {
        qInfo() << "Démarrage du BacktestWorker dans le thread:" << QThread::currentThreadId();
        
        // Initialiser PyBindingManager si nécessaire
        PyBindingManager& pyManager = PyBindingManager::getInstance();
        if (!pyManager.isInitialized()) {
            if (!pyManager.initialize()) {
                emit error("Erreur d'initialisation de Python: " + pyManager.getLastError());
                return;
            }
        }
        
        // Exécuter le backtest Python
        QVariant result = pyManager.runPythonBacktest(
            m_data, m_strategyClass, m_cash, m_spread, m_strategyParams
        );
        
        if (result.isNull()) {
            emit error("Erreur lors de l'exécution du backtest: " + pyManager.getLastError());
            return;
        }
        
        // Pour l'instant, nous émettons des pointeurs nullptr car nous devons 
        // implémenter la conversion des résultats Python vers les structures C++
        // TODO: Implémenter la conversion des résultats
        void* data = nullptr;
        void* stats = reinterpret_cast<void*>(0x1); // Pointeur factice non-null pour indiquer le succès
        
        emit finished(data, stats);
        
    } catch (const std::exception& e) {
        emit error(QString("Exception dans BacktestWorker: %1").arg(e.what()));
    } catch (...) {
        emit error("Erreur inconnue dans BacktestWorker");
    }
}