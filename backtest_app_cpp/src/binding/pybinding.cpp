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
        py::initialize_interpreter();
        
        // Ajouter le chemin du projet Python
        QString projectRoot = findProjectRoot(QCoreApplication::applicationDirPath());
        if (!projectRoot.isEmpty()) {
            py::module_ sys = py::module_::import("sys");
            sys.attr("path").attr("append")(projectRoot.toStdString());
            qDebug() << "Chemin Python ajouté:" << projectRoot;
        }
        
        m_initialized = true;
        qInfo() << "PyBindingManager initialisé avec succès";
        return true;
    }
    catch (const std::exception& e) {
        qCritical() << "Erreur lors de l'initialisation Python:" << e.what();
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
        setError("PyBindingManager not initialized");
        return QVariant();
    }

    try {
        // Créer le DataFrame pandas
        py::module_ pd = py::module_::import("pandas");
        py::dict df_data;
        
        std::vector<double> timestamps, open_vals, high_vals, low_vals, close_vals, volumes;
        for (const auto& bar : data) {
            timestamps.push_back(bar.timestamp.toMSecsSinceEpoch() / 1000.0);
            open_vals.push_back(bar.open);
            high_vals.push_back(bar.high);
            low_vals.push_back(bar.low);
            close_vals.push_back(bar.close);
            volumes.push_back(bar.volume);
        }
        
        df_data["Open"] = py::cast(open_vals);
        df_data["High"] = py::cast(high_vals);
        df_data["Low"] = py::cast(low_vals);
        df_data["Close"] = py::cast(close_vals);
        df_data["Volume"] = py::cast(volumes);
        
        py::object df = pd.attr("DataFrame")(df_data);
        
        // Créer l'index timestamp
        py::object timestamp_index = pd.attr("to_datetime")(timestamps, py::arg("unit")="s");
        df.attr("set_index")(timestamp_index, py::arg("inplace")=true);
        
        // Importer la classe de stratégie
        py::object strategy_module;
        
        if (strategyClass == "BuyHeikinGreenBA") {
            strategy_module = py::module_::import("igtrader.Strategies.BuyHeikinGreen");
        } else if (strategyClass == "SellHeikinRedBA") {
            strategy_module = py::module_::import("igtrader.Strategies.SellHeikinRed");
        } else {
            setError("Unknown strategy class: " + strategyClass);
            return QVariant();
        }
        
        py::object strategy_class = strategy_module.attr(strategyClass.toStdString().c_str());
        
        // Convertir les paramètres
        py::dict py_params = convertParamsToPython(strategyParams);
        
        // Créer le backtest
        py::object backtest_module = py::module_::import("igtrader.backtestingpy.backtesting.backtesting");
        py::object backtest_class = backtest_module.attr("Backtest");
        
        double margin = 1.0;
        py::object bt = backtest_class(
            df,
            strategy_class,
            py::arg("cash")=cash,
            py::arg("commission")=0.0,
            py::arg("spread")=spread,
            py::arg("exclusive_orders")=false,
            py::arg("strategy_kwargs")=py_params,
            py::arg("margin")=margin
        );
        
        // Exécuter le backtest
        py::object stats = bt.attr("run")();
        
        // Retourner un pointeur opaque vers les stats Python
        // CORRECTION: Allouer dynamiquement pour éviter les problèmes de durée de vie
        py::object* stats_ptr = new py::object(stats);
        return QVariant::fromValue(reinterpret_cast<void*>(stats_ptr));
        
    } catch (const std::exception& e) {
        setError(QString("Error in Python backtest: %1").arg(e.what()));
        return QVariant();
    }
}

py::dict PyBindingManager::convertParamsToPython(const QMap<QString, QVariant>& params)
{
    py::dict result;
    
    for (auto it = params.begin(); it != params.end(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        std::string key_str = key.toStdString();
        
        switch (value.type()) {
            case QVariant::Bool:
                result[key_str.c_str()] = value.toBool();
                break;
            case QVariant::Int:
                result[key_str.c_str()] = value.toInt();
                break;
            case QVariant::Double:
                result[key_str.c_str()] = value.toDouble();
                break;
            case QVariant::String:
                result[key_str.c_str()] = value.toString().toStdString();
                break;
            default:
                result[key_str.c_str()] = value.toString().toStdString();
                break;
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