#include "backtest_runner.h"
#include "../binding/pybinding.h"
#include "../config_manager.h"
#include "../data_loader.h"
#include "../app.h"
#include "../panels/general_params_panel.h"
#include "../panels/strategy_base_panel.h"
#include "../panels/base_panel.h"
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
    
    // Récupérer TOUS les paramètres depuis les panels
    QMap<QString, QVariant> allParams;
    
    // Paramètres généraux
    if (m_mainWindow->getGeneralParamsPanel()) {
        QMap<QString, QVariant> generalParams = m_mainWindow->getGeneralParamsPanel()->getValues();
        for (auto it = generalParams.begin(); it != generalParams.end(); ++it) {
            allParams[it.key()] = it.value();
        }
    }
    
    // Paramètres de base de stratégie
    if (m_mainWindow->getStrategyBasePanel()) {
        QMap<QString, QVariant> baseParams = m_mainWindow->getStrategyBasePanel()->getValues();
        for (auto it = baseParams.begin(); it != baseParams.end(); ++it) {
            allParams[it.key()] = it.value();
        }
    }
    
    // Paramètres spécifiques à la stratégie
    if (m_mainWindow->getStrategySpecificPanel()) {
        QMap<QString, QVariant> specificParams = m_mainWindow->getStrategySpecificPanel()->getValues();
        for (auto it = specificParams.begin(); it != specificParams.end(); ++it) {
            allParams[it.key()] = it.value();
        }
    }
    
    qDebug() << "Tous les paramètres récupérés:" << allParams;
    
    // Extraire les paramètres pour le chargement des données
    QString symbol = allParams.value("symbol", "NDX").toString();
    QString interval = allParams.value("interval", "20secs").toString();
    QString period = allParams.value("period", "10d").toString();
    
    // Récupérer la date de fin et la convertir correctement
    QDateTime endDate;
    if (allParams.contains("end_date")) {
        QVariant dateVariant = allParams["end_date"];
        if (dateVariant.type() == QVariant::Date) {
            endDate = QDateTime(dateVariant.toDate(), QTime(23, 59, 59));
        } else if (dateVariant.type() == QVariant::DateTime) {
            endDate = dateVariant.toDateTime();
        } else if (dateVariant.type() == QVariant::String) {
            QString dateStr = dateVariant.toString();
            QStringList dateFormats = {"dd/MM/yyyy", "yyyy-MM-dd", "dd-MM-yyyy"};
            
            for (const QString& format : dateFormats) {
                QDate parsedDate = QDate::fromString(dateStr, format);
                if (parsedDate.isValid()) {
                    endDate = QDateTime(parsedDate, QTime(23, 59, 59));
                    break;
                }
            }
        }
    }
    
    if (!endDate.isValid()) {
        endDate = QDateTime::currentDateTime();
    }
    
    qDebug() << "Paramètres de chargement des données:";
    qDebug() << "- Symbole:" << symbol;
    qDebug() << "- Intervalle:" << interval;
    qDebug() << "- Période:" << period;
    qDebug() << "- Date de fin:" << endDate.toString("dd/MM/yyyy hh:mm:ss");
    
    // Récupérer les heures de trading
    QTime tradingFrom = QTime::fromString(allParams.value("trading_from", "07:00:00").toString(), "hh:mm:ss");
    QTime tradingTo = QTime::fromString(allParams.value("trading_to", "23:00:00").toString(), "hh:mm:ss");
    
    // Récupérer les jours de trading
    std::vector<int> tradingDays;
    if (allParams.contains("trading_days")) {
        QVariantList daysList = allParams["trading_days"].toList();
        for (const QVariant& day : daysList) {
            tradingDays.push_back(day.toInt());
        }
    }
    
    // Charger les données avec tous les paramètres
    std::vector<OHLCBar> data = DataLoader::loadData(
        symbol, interval, period, endDate);
    
    if (data.empty()) {
        showError("Aucune donnée chargée");
        resetUI();
        return;
    }
    
    QString strategyClass = allParams.value("strategy", "BuyHeikinGreenBA").toString();
    double cash = allParams.value("cash", 100000.0).toDouble();
    double spread = allParams.value("spread", 0.0001).toDouble();
    
    // Créer et lancer le worker avec TOUS les paramètres
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
    
    if (!data || !stats) {
        qCritical() << "Pas de résultats de backtest reçus - data:" << data << "stats:" << stats;
        showError("Aucun résultat de backtest reçu");
        return;
    }
    
    qInfo() << "Backtest terminé avec succès, transmission des résultats";
    qDebug() << "Données à transmettre - data:" << data << "stats:" << stats;
    
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
    qDebug() << "BacktestWorker::run() - Début de l'exécution";
    
    try {
        // Vérification de l'initialisation de Python
        PyBindingManager& pyManager = PyBindingManager::getInstance();
        if (!pyManager.isInitialized()) {
            qCritical() << "PyBindingManager non initialisé";
            emit error("Python n'est pas initialisé");
            return;
        }
        
        qDebug() << "Données disponibles:" << m_data.size() << "barres";
        qDebug() << "Stratégie:" << m_strategyClass;
        qDebug() << "Cash:" << m_cash;
        qDebug() << "Paramètres:" << m_strategyParams;
        
        qDebug() << "Démarrage du backtest Python...";
        
        QVariant result = pyManager.runPythonBacktest(
            m_data, 
            m_strategyClass, 
            m_cash, 
            m_spread, 
            m_strategyParams
        );
        
        qDebug() << "Backtest Python terminé";
        
        if (result.isNull()) {
            QString error = pyManager.getLastError();
            qCritical() << "Erreur du backtest:" << error;
            emit this->error(error.isEmpty() ? "Erreur inconnue du backtest" : error);
            return;
        }
        
        // CORRECTION: Extraire les bonnes données du QVariantMap
        QVariantMap resultMap = result.toMap();
        
        if (!resultMap.contains("data") || !resultMap.contains("stats")) {
            qCritical() << "Format de résultat invalide - manque data ou stats";
            emit error("Format de résultat invalide");
            return;
        }
        
        void* data = resultMap["data"].value<void*>();
        void* stats = resultMap["stats"].value<void*>();
        
        qDebug() << "Données extraites - data:" << data << "stats:" << stats;
        
        if (!data || !stats) {
            qCritical() << "Pointeurs de données invalides";
            emit error("Données invalides reçues du backtest");
            return;
        }
        
        qDebug() << "Émission du signal finished avec les bonnes données";
        emit finished(data, stats);  
        
    } catch (const std::exception& e) {
        qCritical() << "Exception dans BacktestWorker::run():" << e.what();
        emit error(QString("Erreur d'exécution: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "Exception inconnue dans BacktestWorker::run()";
        emit error("Erreur d'exécution inconnue");
    }
    
    qDebug() << "BacktestWorker::run() - Fin de l'exécution";
}