#include "components/backtest_runner.h"
#include "components/config_manager.h"
#include "components/data_loader.h"
#include "app.h"
#include "panels/general_params_panel.h"
#include "panels/strategy_specific_panels/strategy_base_panel.h"
#include "panels/base_panel.h"
#include "Strategies/buy_heikin_green.hpp"
#include <QDebug>
#include <QApplication>  

BacktestRunner::BacktestRunner(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<App*>(parent))
    , m_buttonLayout(nullptr)
    , m_runButton(nullptr)
    , m_loadingIndicator(nullptr)
    , m_statsLabel(nullptr)
    , m_worker(nullptr)
    , m_isRunning(false)
    , m_lastTotalCandles(0)
    , m_lastChrono("")
{
    createUIComponents();
}

BacktestRunner::~BacktestRunner() {
    if (m_worker) {
        if (m_worker->isRunning()) {
            m_worker->quit();
            m_worker->wait();
        }
        delete m_worker;  // Suppression explicite
        m_worker = nullptr;
    }
}

void BacktestRunner::createUIComponents() {
    m_buttonLayout = new QHBoxLayout();
    
    m_runButton = new QPushButton("Lancer le backtest");
    m_runButton->setMinimumHeight(40);
    m_runButton->setStyleSheet("background-color: #4CAF50;"
                                "color: white;"
                                "font-weight: bold;");

    connect(m_runButton, &QPushButton::clicked, this, &BacktestRunner::runBacktest);
    
    m_loadingIndicator = new QProgressBar();
    m_loadingIndicator->setMaximum(100);
    m_loadingIndicator->setMinimum(0);
    m_loadingIndicator->setTextVisible(true);
    m_loadingIndicator->setVisible(false);
    m_loadingIndicator->setMinimumHeight(40); 
    m_loadingIndicator->setStyleSheet(
        "QProgressBar {"
        "   text-align: center;"
        "   font-size: 12px;"
        "   border: 1px solid grey;"
        "   border-radius: 2px;"
        "   padding: 2px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"
        "   border-radius: 2px;"
        "}"
    );
    
    // Créer le label pour les statistiques d'exécution
    m_statsLabel = new QLabel();
    m_statsLabel->setVisible(false);
    m_statsLabel->setMinimumHeight(40);
    m_statsLabel->setStyleSheet("QLabel { background-color: #e8f4fd; color: #2c3e50; border: 1px solid #bdc3c7; border-radius: 4px; padding: 4px; margin: 2px 0px; font-size: 11px; }");
    m_statsLabel->setAlignment(Qt::AlignCenter);

    // Créer un layout vertical pour le bouton et les stats
    QVBoxLayout* buttonStatsLayout = new QVBoxLayout();
    buttonStatsLayout->addWidget(m_runButton);
    buttonStatsLayout->addWidget(m_statsLabel);
    buttonStatsLayout->setSpacing(4); // Espacement réduit entre le bouton et le label
    
    // Ajouter le layout vertical au layout horizontal principal
    m_buttonLayout->addLayout(buttonStatsLayout);
    m_buttonLayout->addWidget(m_loadingIndicator);
}

void BacktestRunner::runBacktest() {
    if (m_isRunning) {
        return;
    }
    
    m_runButton->setEnabled(false);
    m_runButton->setVisible(false);
    m_loadingIndicator->setVisible(true);
    m_statsLabel->setVisible(false); // Cacher les stats précédentes
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
    connect(m_worker, &BacktestWorker::progressUpdated, this, &BacktestRunner::onProgressUpdated); 
    // connect(m_worker, &QThread::finished, m_worker, &QObject::deleteLater);
    m_worker->start();
}

void BacktestRunner::onProgressUpdated(int current, int total, const QString& chrono) {
    int percentage = (current * 100) / total;
    m_loadingIndicator->setValue(percentage);
    
    // Capturer les dernières statistiques
    m_lastTotalCandles = total;
    m_lastChrono = chrono;
    
    // Calculer la vitesse en candles/seconde
    qint64 elapsedMs = QTime::fromString(chrono, "mm:ss.zz").msecsTo(QTime(0, 0, 0)) * -1;
    double candlesPerSecond = (current * 1000.0) / elapsedMs;
    // Utiliser des espaces pour séparer visuellement
    m_loadingIndicator->setFormat(QString("%1/%2 (%p%)  %3 c/s - %4")
                                .arg(current)
                                .arg(total)
                                .arg(QString::number(candlesPerSecond, 'f', 1))
                                .arg(chrono));
}

void BacktestRunner::onBacktestFinished(BacktestResults* results) {
    m_isRunning = false;
    resetUI();
    
    if (!results || !results->data) {
        qCritical() << "Pas de résultats de backtest reçus";
        showError("Aucun résultat de backtest reçu");
        return;
    }
    
    qInfo() << "Backtest terminé avec succès, transmission des résultats";
    qDebug() << "Taille des données reçues:" << results->data->size() << "barres";
    
    // Afficher les statistiques d'exécution
    if (!m_lastChrono.isEmpty() && m_lastTotalCandles > 0) {
        qint64 elapsedMs = QTime::fromString(m_lastChrono, "mm:ss.zz").msecsTo(QTime(0, 0, 0)) * -1;
        double avgCandlesPerSecond = (m_lastTotalCandles * 1000.0) / elapsedMs;
        
        QString statsText = QString("📊 %1 candles • ⏱️ %2 • ⚡ %3 c/s")
                              .arg(m_lastTotalCandles)
                              .arg(m_lastChrono)
                              .arg(QString::number(avgCandlesPerSecond, 'f', 1));
        
        m_statsLabel->setText(statsText);
        m_statsLabel->setVisible(true);
    }
    
    // Transférer la propriété des résultats à l'application
    if (m_mainWindow) {
        // Transférer la propriété à l'App
        m_mainWindow->setBacktestResults(m_worker->takeResults());
        emit backtestCompleted(m_mainWindow->getBacktestResults());
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
    m_runButton->setVisible(true);
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
    QElapsedTimer timer;
    timer.start();

    
    // Récupération des paramètres généraux
    QString strategyName = m_mainWindow->getGeneralParamsPanel()->getValues()["strategy"].toString();
    QString symbol = m_mainWindow->getGeneralParamsPanel()->getValues().value("symbol", "NDX").toString();
    QString interval = m_mainWindow->getGeneralParamsPanel()->getValues().value("interval", "20secs").toString();
    QString period = m_mainWindow->getGeneralParamsPanel()->getValues().value("period", "10d").toString();
    double cash = m_mainWindow->getGeneralParamsPanel()->getValues().value("cash", 100000.0).toDouble();
    double spread = m_mainWindow->getGeneralParamsPanel()->getValues().value("spread", 0.0001).toDouble();
    double commission = m_mainWindow->getGeneralParamsPanel()->getValues().value("commission", 0.0).toDouble();
    double leverage_limit = m_mainWindow->getGeneralParamsPanel()->getValues().value("leverage_limit", 20.0).toDouble();
    bool tradeOnClose = m_mainWindow->getGeneralParamsPanel()->getValues().value("trade_on_close", false).toBool();
    bool hedging = m_mainWindow->getGeneralParamsPanel()->getValues().value("hedging", false).toBool();
    bool exclusiveOrders = m_mainWindow->getGeneralParamsPanel()->getValues().value("exclusive_orders", true).toBool();
    bool finalizeTrades = m_mainWindow->getGeneralParamsPanel()->getValues().value("finalize_trades", true).toBool();
    
    // Récupération et conversion de la date de fin
    QDateTime endDate;
    if (m_mainWindow->getGeneralParamsPanel()->getValues().contains("end_date")) {
        QVariant dateVariant = m_mainWindow->getGeneralParamsPanel()->getValues()["end_date"];
        if (dateVariant.userType() == QMetaType::QDate) {
            endDate = QDateTime(dateVariant.toDate(), QTime(23, 59, 59));
        } else if (dateVariant.userType() == QMetaType::QDateTime) {
            endDate = dateVariant.toDateTime();
        } else if (dateVariant.userType() == QMetaType::QString) {
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
    
    // TODO : mettre des debug ici pour vérifier les valeurs
    // std::cout << "Paramètres de chargement des données:" << std::endl;
    // std::cout << "- Stratégie:" << strategyName.toStdString() << std::endl;
    // std::cout << "- Symbole:" << symbol.toStdString() << std::endl;
    // std::cout << "- Intervalle:" << interval.toStdString() << std::endl;
    // std::cout << "- Période:" << period.toStdString() << std::endl;
    // std::cout << "- Capital initial:" << cash << std::endl;
    // std::cout << "- Spread:" << spread << std::endl;
    // std::cout << "- Commission:" << commission << std::endl;
    // std::cout << "- Trade à la clôture:" << (tradeOnClose ? "Oui" : "Non") << std::endl;
    // std::cout << "- Hedging:" << (hedging ? "Oui" : "Non") << std::endl;
    // std::cout << "- Ordres exclusifs:" << (exclusiveOrders ? "Oui" : "Non") << std::endl;
    // std::cout << "- Finalisation des trades:" << (finalizeTrades ? "Oui" : "Non") << std::endl;
    // std::cout << "- Date de fin:" << endDate.toString("dd/MM/yyyy hh:mm:ss").toStdString() << std::endl;
    
    
    auto strategyCreator = StrategyRegistry::getInstance().getCreator(strategyName);
    if (!strategyCreator) {
        emit error(QString("Stratégie non supportée: %1").arg(strategyName));
        return;
    }
    
    // Chargement des données avec DataLoader puis conversion en be::Data
    std::vector<OHLCBar> rawData = DataLoader::loadData(symbol, interval, period, endDate);
    
    if (rawData.empty()) {
        emit error("Aucune donnée chargée");
        return;
    }
    // Convertir en format be::Data
    std::shared_ptr<be::Data> data = convertToBeData(rawData);
    
    // Extraire les indicateurs de la stratégie
    std::vector<StrategyIndicator> strategyIndicators = m_mainWindow->getIndicatorConfig();
    
    // Créer un objet BacktestResults pour stocker les résultats
    m_results = std::make_unique<BacktestResults>();
    m_results->data = data;
    m_results->indicators = strategyIndicators;  // Stocker les indicateurs
    
    qDebug() << "Données disponibles:" << data->size() << "barres";
    qDebug() << "Démarrage du backtest C++...";
    
    
    // Créer la factory pour le backtest (une closure qui capture le créateur et l'app)
    auto strategyFactory = [strategyCreator, this](std::shared_ptr<be::Broker> b, std::shared_ptr<be::Data> d) {
        return strategyCreator(b, d, m_mainWindow);
    };
    double margin = 1 / leverage_limit; // Calculer la marge à partir du levier
    
    // Créer et exécuter le backtest
    be::Backtest backtest(
        data,               // Données historiques
        strategyFactory,    // Factory de stratégie
        cash,               // Capital initial
        spread,             // Spread
        commission,         // Commission
        margin,                // Marge (défaut: 1.0)
        tradeOnClose,       // Trade à la clôture
        hedging,            // Hedging
        exclusiveOrders,    // Ordres exclusifs
        finalizeTrades      // Finalisation des trades
    );
    
    backtest.setProgressCallback([this, &timer](size_t current, size_t total) {
        qint64 elapsed = timer.elapsed();
        QString chrono = QTime::fromMSecsSinceStartOfDay(elapsed).toString("mm:ss.zz");
        emit progressUpdated(static_cast<int>(current), static_cast<int>(total), chrono);
    });
    
    qDebug() << "Exécution du backtest...";
    m_results->stats = backtest.run();
    qDebug() << "Backtest terminé avec succès";
    
    // Émettre le signal avec les résultats
    emit finished(m_results.get());
        
}

std::shared_ptr<be::Data> BacktestWorker::convertToBeData(const std::vector<OHLCBar>& bars)
{
    // Pré-allouer de la mémoire pour éviter les réallocations
    size_t size = bars.size();
    std::vector<be::Date> dates;
    std::vector<double> open, high, low, close, volume;
    
    dates.reserve(size);
    open.reserve(size);
    high.reserve(size);
    low.reserve(size);
    close.reserve(size);
    volume.reserve(size);
    
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
    
    // Detect gaps in the data (where the time between candles exceeds expected interval)
    std::vector<size_t> gapIndices;
    int expectedIntervalSecs = DataLoader::intervalToSeconds(m_mainWindow->getGeneralParamsPanel()->getValues().value("interval", "20secs").toString());
    
    // Use 1.5x the expected interval as the threshold for gap detection
    int gapThreshold = expectedIntervalSecs * 1.5;
    
    for (size_t i = 0; i < bars.size() - 1; i++) {
        QDateTime current = bars[i].timestamp;
        QDateTime next = bars[i+1].timestamp;
        int secondsDiff = current.secsTo(next);
        
        // If the difference is significantly more than the expected interval, mark as a gap
        if (secondsDiff > gapThreshold) 
            gapIndices.push_back(i);   
    }
    
    // Create the Data object
    auto data = std::make_shared<be::Data>(dates, open, high, low, close, volume);
    
    // Set the gap indices
    data->setGapIndices(gapIndices);    
    return data;
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