#ifdef slots
#undef slots
#endif

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Pour les arguments nommés
using namespace pybind11::literals;

#ifndef QT_NO_KEYWORDS
#define slots Q_SLOTS
#endif

#include <QDebug>
#include <QDir>
#include <QCoreApplication>

#include "pybinding.h"

namespace py = pybind11;

PyBindingManager* PyBindingManager::instance = nullptr;

PyBindingManager::PyBindingManager()
    : m_initialized(false)
{
}

PyBindingManager::~PyBindingManager()
{
    finalize();
}

void PyBindingManager::finalize()
{
    if (m_initialized) {
        try {
            py::finalize_interpreter();
            m_initialized = false;
        } catch (const std::exception& e) {
            qCritical() << "Erreur lors de la finalisation Python:" << e.what();
        }
    }
}

PyBindingManager& PyBindingManager::getInstance() 
{
    if (!instance) {
        instance = new PyBindingManager();
    }
    return *instance;
}

bool PyBindingManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    try {
        qDebug() << "Initialisation de Python...";
        
        // Initialiser Python avec support multi-threading
        if (!Py_IsInitialized()) {
            py::initialize_interpreter();
            // Libérer le GIL pour permettre l'utilisation dans d'autres threads
            PyEval_SaveThread();
        }
        
        // Tester l'acquisition du GIL
        {
            py::gil_scoped_acquire acquire;
            
            QString projectRoot = findProjectRoot(QCoreApplication::applicationDirPath());
            if (!projectRoot.isEmpty()) {
                py::module sys = py::module::import("sys");
                py::list path = sys.attr("path");
                path.append(projectRoot.toStdString());
                qDebug() << "Chemin Python ajouté:" << projectRoot;
            }
            
            // Test des imports critiques
            py::module::import("pandas");
            py::module::import("numpy");
            py::module::import("igtrader.backtestingpy.backtesting");
            qDebug() << "Modules Python importés avec succès";
        }
        
        m_initialized = true;
        qDebug() << "PyBindingManager initialisé avec succès";
        return true;
        
    } catch (const std::exception& e) {
        setError(QString("Erreur lors de l'initialisation: %1").arg(e.what()));
        qCritical() << m_lastError;
        return false;
    }
}

QString PyBindingManager::findProjectRoot(const QString& startPath)
{
    QDir dir(startPath);
    do {
        if (dir.exists("igtrader") && QFileInfo(dir.filePath("igtrader")).isDir()) {
            return dir.absolutePath();
        }
    } while (dir.cdUp());
    
    return QString();
}

QVariant PyBindingManager::runPythonBacktest(const std::vector<OHLCBar>& data,
                                           const QString& strategyClass,
                                           double cash,
                                           double spread,
                                           const QMap<QString, QVariant>& strategyParams)
{
    if (!m_initialized) {
        setError("PyBindingManager non initialisé");
        return QVariant();
    }

    qDebug() << "runPythonBacktest - Début";
    qDebug() << "Données:" << data.size() << "barres";
    qDebug() << "Stratégie:" << strategyClass;

    try {
        qDebug() << "Acquisition du GIL Python...";
        py::gil_scoped_acquire acquire;
        qDebug() << "GIL acquis avec succès";

        // Créer le DataFrame pandas
        qDebug() << "Création du DataFrame pandas...";
        py::module pd = py::module::import("pandas");
        
        // Créer les listes pour chaque colonne
        std::vector<std::string> timestamps;
        std::vector<double> opens, highs, lows, closes, volumes;
        
        timestamps.reserve(data.size());
        opens.reserve(data.size());
        highs.reserve(data.size());
        lows.reserve(data.size());
        closes.reserve(data.size());
        volumes.reserve(data.size());

        qDebug() << "Conversion des données OHLC...";
        for (size_t i = 0; i < data.size(); ++i) {
            if (i % 1000 == 0) {
                qDebug() << "Conversion:" << i << "/" << data.size();
            }
            
            const OHLCBar& bar = data[i];
            timestamps.push_back(bar.timestamp.toString(Qt::ISODate).toStdString());
            opens.push_back(bar.open);
            highs.push_back(bar.high);
            lows.push_back(bar.low);
            closes.push_back(bar.close);
            volumes.push_back(bar.volume);
        }
        
        qDebug() << "Données converties, création du DataFrame...";
        
        // Créer le DataFrame
        py::dict data_dict;
        data_dict["Open"] = opens;
        data_dict["High"] = highs;
        data_dict["Low"] = lows;
        data_dict["Close"] = closes;
        data_dict["Volume"] = volumes;

        qDebug() << "Données converties, création du DataFrame...";
        py::object data_df = pd.attr("DataFrame")(data_dict);
        qDebug() << "DataFrame créé";

        // Conversion de l'index datetime
        qDebug() << "Conversion de l'index datetime...";
        data_df.attr("set_index")(timestamps, py::arg("inplace")=true);

        qDebug() << "Index datetime défini";

        // Importer la stratégie
        qDebug() << "Import de la stratégie:" << strategyClass;
        py::module strategies_module;
        
        if (strategyClass == "BuyHeikinGreenBA") {
            strategies_module = py::module::import("igtrader.Strategies.BuyHeikinGreen");
        } else if (strategyClass == "SellHeikinRedBA") {
            strategies_module = py::module::import("igtrader.Strategies.SellHeikinRed");
        } else {
            setError(QString("Stratégie non supportée: %1").arg(strategyClass));
            return QVariant();
        }
        
        py::object strategy_class = strategies_module.attr(strategyClass.toStdString().c_str());
        qDebug() << "Stratégie importée avec succès";

        // Convertir les paramètres
        qDebug() << "Conversion des paramètres de stratégie...";
        py::dict py_params = convertParamsToPython(strategyParams);
        qDebug() << "Paramètres convertis";

        // Importer Backtest
        qDebug() << "Import du module Backtest...";
        py::module backtest_module = py::module::import("igtrader.backtestingpy.backtesting.backtesting");
        py::object backtest_class = backtest_module.attr("Backtest");
        qDebug() << "Module Backtest importé";

        // Calculer la marge
        double leverage = py_params.contains("maximal_leverage") ? 
            py_params["maximal_leverage"].cast<double>() : 20.0;
        double margin = 1.0 / leverage;
        
        qDebug() << "Configuration du backtest avec:";
        qDebug() << "- Cash:" << cash;
        qDebug() << "- Spread:" << spread;
        qDebug() << "- Margin:" << margin;

        // Créer l'instance Backtest
        qDebug() << "Création de l'instance Backtest...";
        py::object bt = backtest_class(
            data_df, 
            strategy_class,
            py::arg("cash") = cash,
            py::arg("commission") = 0.0,
            py::arg("spread") = spread,
            py::arg("exclusive_orders") = false,
            py::arg("strategy_kwargs") = py_params,
            py::arg("margin") = margin
        );
        qDebug() << "Instance Backtest créée";

        // Exécuter le backtest avec timeout
        qDebug() << "Démarrage de l'exécution du backtest...";
        py::object stats;
        
        // Ajouter un mécanisme de timeout simple
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout_duration = std::chrono::minutes(5); // 5 minutes timeout
        
        try {
            stats = bt.attr("run")();
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
            qDebug() << "Backtest terminé en" << duration.count() << "secondes";
        } catch (const py::error_already_set& e) {
            qCritical() << "Erreur Python lors de l'exécution du backtest:" << e.what();
            setError(QString("Erreur Python: %1").arg(e.what()));
            return QVariant();
        }
        
        qDebug() << "Backtest exécuté avec succès";

        // Vérifier les résultats
        if (stats.is_none()) {
            setError("Le backtest a retourné des résultats vides");
            return QVariant();
        }

        // Convertir les résultats en QVariant (stockage opaque)
        void* stats_ptr = stats.ptr();
        qDebug() << "Résultats convertis, pointeur:" << stats_ptr;
        
        // Incrémenter la référence pour éviter la destruction
        stats.inc_ref();
        
        return QVariant::fromValue(stats_ptr);

    } catch (const py::error_already_set& e) {
        qCritical() << "Erreur Python:" << e.what();
        setError(QString("Erreur Python: %1").arg(e.what()));
        return QVariant();
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++:" << e.what();
        setError(QString("Erreur C++: %1").arg(e.what()));
        return QVariant();
    }
}

py::dict PyBindingManager::convertParamsToPython(const QMap<QString, QVariant>& params)
{
    py::dict result;
    
    for (auto it = params.begin(); it != params.end(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        
        // Conversion selon le type QVariant
        if (value.type() == QVariant::Bool) {
            result[py::str(key.toStdString())] = py::bool_(value.toBool());
        }
        else if (value.type() == QVariant::Int) {
            result[py::str(key.toStdString())] = py::int_(value.toInt());
        }
        else if (value.type() == QVariant::Double) {
            result[py::str(key.toStdString())] = py::float_(value.toDouble());
        }
        else if (value.type() == QVariant::String) {
            result[py::str(key.toStdString())] = py::str(value.toString().toStdString());
        }
        else if (value.type() == QVariant::Time) {
            QTime time = value.toTime();
            result[py::str(key.toStdString())] = py::str(time.toString("hh:mm:ss").toStdString());
        }
        else if (value.type() == QVariant::Date) {
            QDate date = value.toDate();
            result[py::str(key.toStdString())] = py::str(date.toString("dd/MM/yyyy").toStdString());
        }
        else if (value.type() == QVariant::List) {
            py::list py_list;
            QVariantList list = value.toList();
            for (const QVariant& item : list) {
                py_list.append(py::int_(item.toInt()));  // Supposant que ce sont des entiers
            }
            result[py::str(key.toStdString())] = py_list;
        }
        else {
            qWarning() << "Type non supporté pour la clé" << key << ":" << value.typeName();
            // Convertir en string comme fallback
            result[py::str(key.toStdString())] = py::str(value.toString().toStdString());
        }
    }
    
    return result;
}

QList<QVariantMap> PyBindingManager::getTrades(void* stats)
{
    QList<QVariantMap> trades;
    
    if (!stats || !m_initialized) {
        return trades;
    }
    
    try {
        py::object* stats_obj = reinterpret_cast<py::object*>(stats);
        py::object trades_df = (*stats_obj)["_trades"];
        
        // Convertir le DataFrame pandas en QList<QVariantMap>
        py::object records = trades_df.attr("to_dict")("records");
        py::list trade_list = py::cast<py::list>(records);
        
        for (auto item : trade_list) {
            py::dict trade_dict = py::cast<py::dict>(item);
            QVariantMap trade_map;
            
            for (auto pair : trade_dict) {
                QString key = QString::fromStdString(py::str(pair.first));
                // CORRECTION: Convertir explicitement handle en object
                py::object value_obj = py::reinterpret_borrow<py::object>(pair.second);
                QVariant value = pythonToQVariant(value_obj);
                trade_map[key] = value;
            }
            trades.append(trade_map);
        }
        
    } catch (const std::exception& e) {
        setError(QString("Error getting trades: %1").arg(e.what()));
    }
    
    return trades;
}

QVariant PyBindingManager::getStatValue(void* stats, const QString& key)
{
    if (!stats || !m_initialized) {
        return QVariant();
    }
    
    try {
        py::object* stats_obj = reinterpret_cast<py::object*>(stats);
        py::object value = (*stats_obj)[key.toStdString().c_str()];
        return pythonToQVariant(value);
    } catch (const std::exception& e) {
        setError(QString("Error getting stat value %1: %2").arg(key, e.what()));
        return QVariant();
    }
}

QList<QVariantMap> PyBindingManager::getEquityCurve(void* stats)
{
    QList<QVariantMap> equity;
    
    if (!stats || !m_initialized) {
        return equity;
    }
    
    try {
        py::object* stats_obj = reinterpret_cast<py::object*>(stats);
        py::object equity_df = (*stats_obj)["_equity_curve"];
        
        // Convertir le DataFrame pandas en QList<QVariantMap>
        py::object records = equity_df.attr("to_dict")("records");
        py::list equity_list = py::cast<py::list>(records);
        
        for (auto item : equity_list) {
            py::dict equity_dict = py::cast<py::dict>(item);
            QVariantMap equity_map;
            
            for (auto pair : equity_dict) {
                QString key = QString::fromStdString(py::str(pair.first));
                // CORRECTION: Convertir explicitement handle en object
                py::object value_obj = py::reinterpret_borrow<py::object>(pair.second);
                QVariant value = pythonToQVariant(value_obj);
                equity_map[key] = value;
            }
            equity.append(equity_map);
        }
        
    } catch (const std::exception& e) {
        setError(QString("Error getting equity curve: %1").arg(e.what()));
    }
    
    return equity;
}

QVariant PyBindingManager::pythonToQVariant(const py::object& obj)
{
    try {
        if (py::isinstance<py::bool_>(obj)) {
            return QVariant(py::cast<bool>(obj));
        } else if (py::isinstance<py::int_>(obj)) {
            return QVariant(py::cast<int>(obj));
        } else if (py::isinstance<py::float_>(obj)) {
            return QVariant(py::cast<double>(obj));
        } else if (py::isinstance<py::str>(obj)) {
            return QVariant(QString::fromStdString(py::cast<std::string>(obj)));
        } else if (obj.is_none()) {
            return QVariant();
        } else {
            // Tenter de convertir en string pour les autres types
            py::object str_obj = py::str(obj);
            return QVariant(QString::fromStdString(py::cast<std::string>(str_obj)));
        }
    } catch (const std::exception& e) {
        setError(QString("Error converting Python object: %1").arg(e.what()));
        return QVariant();
    }
}

void PyBindingManager::setError(const QString& error)
{
    m_lastError = error;
}

QString PyBindingManager::getLastError() const
{
    return m_lastError;
}

void PyBindingManager::clearError()
{
    m_lastError.clear();
}

bool PyBindingManager::isInitialized() const
{
    return m_initialized;
}