#include "components/backtest_runner.h"
// Garder uniquement les inclusions nécessaires à l'UI
#include "components/config_manager.h"
#include "components/data_loader.h"
#include "app.h"
#include "panels/general_params_panel.h"
#include "panels/strategy_specific_panels/strategy_base_panel.h"
#include "panels/base_panel.h"
#include <QDebug>
#include <QApplication>  

// Constructeur, destructeur et méthodes UI restent inchangés
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
}

// Méthodes d'UI inchangées
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
    
    // Créer le worker et lui passer la responsabilité
    m_worker = new BacktestWorker(m_mainWindow, this);
    
    connect(m_worker, &BacktestWorker::finished, this, &BacktestRunner::onBacktestFinished);
    connect(m_worker, &BacktestWorker::error, this, &BacktestRunner::onBacktestError);
    connect(m_worker, &QThread::finished, m_worker, &QObject::deleteLater);
    
    m_worker->start();
}

void BacktestRunner::onBacktestFinished(BacktestResults* results)
{
    m_isRunning = false;
    resetUI();
    
    if (!results || !results->data) {
        qCritical() << "Pas de résultats de backtest reçus";
        showError("Aucun résultat de backtest reçu");
        return;
    }
    
    qInfo() << "Backtest terminé avec succès, transmission des résultats";
    qDebug() << "Taille des données reçues:" << results->data->size() << "barres";
    
    // Transmettre les résultats au gestionnaire principal
    if (m_mainWindow) {
        m_mainWindow->updateResultViews(results);
        emit backtestCompleted(results);
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

// Implémentation de BacktestWorker avec le backtest C++
BacktestWorker::BacktestWorker(App* mainWindow, QObject* parent)
    : QThread(parent)
    , m_mainWindow(mainWindow)
    , m_results(nullptr)
{
}

BacktestWorker::~BacktestWorker()
{
    // Le destructeur nettoie automatiquement m_results
}

void BacktestWorker::run()
{
    qDebug() << "BacktestWorker::run() - Début de l'exécution";
    
    try {
        // Récupération des paramètres
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
        
        // Extraction des paramètres pour le chargement des données
        QString symbol = allParams.value("symbol", "NDX").toString();
        QString interval = allParams.value("interval", "20secs").toString();
        QString period = allParams.value("period", "10d").toString();
        double cash = allParams.value("cash", 100000.0).toDouble();
        double spread = allParams.value("spread", 0.0001).toDouble();
        double commission = allParams.value("commission", 0.001).toDouble();
        
        // Récupération et conversion de la date de fin
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
        
        // Chargement des données avec DataLoader puis conversion en be::Data
        std::vector<OHLCBar> rawData = DataLoader::loadData(symbol, interval, period, endDate);
        
        if (rawData.empty()) {
            emit error("Aucune donnée chargée");
            return;
        }
        
        // Convertir en format be::Data
        std::shared_ptr<be::Data> data = convertToBeData(rawData);
        
        // Créer un objet BacktestResults pour stocker les résultats
        m_results = std::make_unique<BacktestResults>();
        m_results->data = data;
        
        qDebug() << "Données disponibles:" << data->size() << "barres";
        qDebug() << "Démarrage du backtest C++...";
        
        // Créer une factory de stratégie
        auto strategyFactory = [this, &allParams](std::shared_ptr<be::Broker> broker, std::shared_ptr<be::Data> data) {
            // return createStrategy(broker, data, allParams);
            StrategyBaseConfig baseConfig;
            BuyHeikinGreenConfig bhgConfig;
    
            bhgConfig.use_previous_ha_candle_red_filter = true;
            bhgConfig.use_stoch_filter = false;
            bhgConfig.use_ema_short_filter = false;
            bhgConfig.use_ema_long_filter = false;
            bhgConfig.use_rsi_filter = false;

            // Utiliser la stratégie SmaCrossStrategy avec une moyenne rapide de 10 jours et une lente de 30 jours
            // return std::make_shared<SmaCrossStrategy>(broker, data, 10, 30);
            return std::make_shared<BuyHeikinGreenAdapter>(broker, data, baseConfig, bhgConfig);
        };
        
        // Paramètres optionnels
        bool tradeOnClose = allParams.value("trade_on_close", false).toBool();
        bool hedging = allParams.value("hedging", false).toBool();
        bool exclusiveOrders = allParams.value("exclusive_orders", true).toBool();
        bool finalizeTrades = allParams.value("finalize_trades", true).toBool();
        
        // Créer et exécuter le backtest
        be::Backtest backtest(
            data,               // Données historiques
            strategyFactory,    // Factory de stratégie
            cash,               // Capital initial
            spread,             // Spread
            commission,         // Commission
            1.0,                // Marge (défaut: 1.0)
            tradeOnClose,       // Trade à la clôture
            hedging,            // Hedging
            exclusiveOrders,    // Ordres exclusifs
            finalizeTrades      // Finalisation des trades
        );
        
        qDebug() << "Exécution du backtest...";
        m_results->stats = backtest.run();
        qDebug() << "Backtest terminé avec succès";
        
        // Émettre le signal avec les résultats
        qDebug() << "Émission du signal finished avec les résultats C++";
        emit finished(m_results.get());
        
    } catch (const std::exception& e) {
        qCritical() << "Exception dans BacktestWorker::run():" << e.what();
        emit error(QString("Erreur d'exécution: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "Exception inconnue dans BacktestWorker::run()";
        emit error("Erreur d'exécution inconnue");
    }
    
    qDebug() << "BacktestWorker::run() - Fin de l'exécution";
}

std::shared_ptr<be::Data> BacktestWorker::convertToBeData(const std::vector<OHLCBar>& bars)
{
    std::vector<be::Date> dates;
    std::vector<double> open, high, low, close, volume;
    
    for (const auto& bar : bars) {
        // Convertir la date depuis la structure OHLCBar vers be::Date
        QDateTime dt = bar.timestamp;
        be::Date date(
            dt.date().year(),
            dt.date().month(),
            dt.date().day(),
            dt.time().hour(),
            dt.time().minute(),
            dt.time().second()
        );
        
        dates.push_back(date);
        open.push_back(bar.open);
        high.push_back(bar.high);
        low.push_back(bar.low);
        close.push_back(bar.close);
        volume.push_back(bar.volume);
    }
    
    return std::make_shared<be::Data>(dates, open, high, low, close, volume);
}

std::shared_ptr<be::Strategy> BacktestWorker::createStrategy(
    std::shared_ptr<be::Broker> broker, 
    std::shared_ptr<be::Data> data,
    const QMap<QString, QVariant>& params)
{
    // Récupérer le nom de la stratégie
    QString strategyName = params.value("strategy", "BuyHeikinGreen").toString();
    
    // Création de la configuration de base
    StrategyBaseConfig baseConfig;
    
    // Remplir la configuration de base depuis params
    baseConfig.cash = params.value("cash", 10000.0).toDouble();
    baseConfig.use_atr_for_sl = params.value("use_atr_for_sl", true).toBool();
    baseConfig.use_atr_for_tp = params.value("use_atr_for_tp", true).toBool();
    baseConfig.atr_period = params.value("atr_period", 14).toInt();
    baseConfig.stop_loss_atr_multiplier = params.value("sl_atr_multiple", 2.0).toDouble();
    baseConfig.take_profit_atr_multiplier = params.value("tp_atr_multiple", 3.0).toDouble();
    baseConfig.risk_percentage = params.value("risk_per_trade_pct", 1.0).toDouble();
    
    // Si c'est BuyHeikinGreen, créer un adaptateur spécifique
    // if (strategyName.contains("BuyHeikinGreen", Qt::CaseInsensitive)) {
    // Créer la configuration spécifique
    BuyHeikinGreenConfig bhgConfig;
    
    // Remplir la configuration BHG depuis params
    bhgConfig.use_previous_ha_candle_red_filter = params.value("use_previous_ha_candle_red_filter", true).toBool();
    bhgConfig.use_ema_short_filter = params.value("use_ema_short_filter", false).toBool();
    bhgConfig.use_ema_long_filter = params.value("use_ema_long_filter", false).toBool();
    bhgConfig.use_stoch_filter = params.value("use_stoch_filter", false).toBool();
    bhgConfig.use_rsi_filter = params.value("use_rsi_filter", false).toBool();
    bhgConfig.ema_short_period = params.value("ema_short_period", 150).toInt();
    bhgConfig.ema_long_period = params.value("ema_long_period", 198).toInt();
    bhgConfig.stoch_fastk = params.value("stoch_fastk", 10).toInt();
    bhgConfig.stoch_slowk = params.value("stoch_slowk", 7).toInt();
    bhgConfig.stoch_slowd = params.value("stoch_slowd", 3).toInt();
    bhgConfig.stoch_threshold = params.value("stoch_threshold", 20).toInt();
    bhgConfig.rsi_period = params.value("rsi_period", 14).toInt();
    bhgConfig.rsi_threshold = params.value("rsi_threshold", 30).toInt();
    
    // Créer et retourner l'adaptateur
    return std::make_shared<BuyHeikinGreenAdapter>(broker, data, baseConfig, bhgConfig);
}