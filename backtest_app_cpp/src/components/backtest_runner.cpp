#include "backtest_runner.h"
#include "../binding/pybinding.h"
#include "../config_manager.h"
#include "../data_loader.h"
#include "../app.h"
#include "../panels/general_params_panel.h"
#include "../panels/strategy_base_panel.h"
#include "../panels/base_panel.h"
#include <QDebug>
#include <QApplication>  

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
    
    m_runButton->setEnabled(false);
    m_loadingIndicator->setVisible(true);
    m_isRunning = true;
    
    emit backtestStarted();
    
    QApplication::processEvents();
    
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
    
    // Créer le worker immédiatement et lui passer la responsabilité de tout (recup paramètres, données, et run du backtest)
    m_worker = new BacktestWorker(m_mainWindow, this);
    
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

// SUPPRESSION du constructeur non déclaré
// BacktestWorker - Version modifiée pour gérer tous les paramètres
BacktestWorker::BacktestWorker(App* mainWindow, QObject* parent)
    : QThread(parent)
    , m_mainWindow(mainWindow)
{
}

BacktestWorker::~BacktestWorker()
{
    // Destructor implementation
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
        
        // DÉPLACÉ: Récupération des paramètres dans le worker thread
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
        
        // DÉPLACÉ: Extraction des paramètres pour le chargement des données
        QString symbol = allParams.value("symbol", "NDX").toString();
        QString interval = allParams.value("interval", "20secs").toString();
        QString period = allParams.value("period", "10d").toString();
        
        // DÉPLACÉ: Récupération et conversion de la date de fin
        QDateTime endDate;
        if (allParams.contains("end_date")) {
            QVariant dateVariant = allParams["end_date"];
            if (dateVariant.typeId() == QVariant::Date) {
                endDate = QDateTime(dateVariant.toDate(), QTime(23, 59, 59));
            } else if (dateVariant.typeId() == QVariant::DateTime) {
                endDate = dateVariant.toDateTime();
            } else if (dateVariant.typeId() == QVariant::String) {
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
        
        // DÉPLACÉ: Chargement des données dans le worker thread
        std::vector<OHLCBar> data = DataLoader::loadData(symbol, interval, period, endDate);
        
        if (data.empty()) {
            emit error("Aucune donnée chargée");
            return;
        }
        
        QString strategyClass = allParams.value("strategy", "BuyHeikinGreenBA").toString();
        double cash = allParams.value("cash", 100000.0).toDouble();
        double spread = allParams.value("spread", 0.0001).toDouble();
        
        qDebug() << "Données disponibles:" << data.size() << "barres";
        qDebug() << "Stratégie:" << strategyClass;
        qDebug() << "Cash:" << cash;
        qDebug() << "Paramètres:" << allParams;
        
        qDebug() << "Démarrage du backtest Python...";
        
        QVariant result = pyManager.runPythonBacktest(
            data, 
            strategyClass, 
            cash, 
            spread, 
            allParams
        );
        
        qDebug() << "Backtest Python terminé";
        
        if (result.isNull()) {
            QString error = pyManager.getLastError();
            qCritical() << "Erreur du backtest:" << error;
            emit this->error(error.isEmpty() ? "Erreur inconnue du backtest" : error);
            return;
        }
        
        // Extraire les données du QVariantMap
        QVariantMap resultMap = result.toMap();
        
        if (!resultMap.contains("data") || !resultMap.contains("stats")) {
            qCritical() << "Format de résultat invalide - manque data ou stats";
            emit error("Format de résultat invalide");
            return;
        }
        
        void* dataPtr = resultMap["data"].value<void*>();
        void* statsPtr = resultMap["stats"].value<void*>();
        
        qDebug() << "Données extraites - data:" << dataPtr << "stats:" << statsPtr;
        
        if (!dataPtr || !statsPtr) {
            qCritical() << "Pointeurs de données invalides";
            emit error("Données invalides reçues du backtest");
            return;
        }
        
        qDebug() << "Émission du signal finished avec les bonnes données";
        emit finished(dataPtr, statsPtr);  
        
    } catch (const std::exception& e) {
        qCritical() << "Exception dans BacktestWorker::run():" << e.what();
        emit error(QString("Erreur d'exécution: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "Exception inconnue dans BacktestWorker::run()";
        emit error("Erreur d'exécution inconnue");
    }
    
    qDebug() << "BacktestWorker::run() - Fin de l'exécution";
}