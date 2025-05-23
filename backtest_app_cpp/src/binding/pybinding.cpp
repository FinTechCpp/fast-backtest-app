#include "pybinding.h"
#include <QDir>
#include <stdexcept>
#include "../data_loader.h"

// Add numpy support for arrays
#include <pybind11/numpy.h>

namespace py = pybind11;

PyBindingManager* PyBindingManager::instance = nullptr;

PyBindingManager::PyBindingManager()
    : m_initialized(false)
    , m_pyHelpers(nullptr)
    , m_pyBacktest(nullptr)
{
}

PyBindingManager::~PyBindingManager()
{
    finalize();
}

PyBindingManager* PyBindingManager::getInstance()
{
    if (!instance) {
        instance = new PyBindingManager();
    }
    return instance;
}

bool PyBindingManager::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    try {
        // Initialize Python interpreter
        py::initialize_interpreter();
        
        // Add the project path to Python path
        py::module sys = py::module::import("sys");
        QString projectPath = "/home/max/ig-trading-bot";
        sys.attr("path").attr("insert")(0, projectPath.toStdString());
        
        m_initialized = true;
        qDebug() << "Python interpreter initialized successfully";
        return true;
    } 
    catch (const py::error_already_set& e) {
        setError(QString("Failed to initialize Python: %1").arg(e.what()));
        return false;
    }
}

void PyBindingManager::finalize()
{
    if (m_initialized) {
        // Don't finalize the interpreter if it was already initialized externally
        // py::finalize_interpreter();
        m_initialized = false;
    }
}

bool PyBindingManager::isInitialized() const
{
    return m_initialized;
}

void* PyBindingManager::runBacktest(
    void* data,
    const QString& strategyClass,
    double cash,
    double spread,
    const QMap<QString, QVariant>& strategyParams)
{
    if (!m_initialized) {
        setError("Python interpreter not initialized");
        return nullptr;
    }
    
    try {
        qDebug() << "Starting backtest with strategy:" << strategyClass;
        
        // Import required modules
        py::module backtesting = py::module::import("backtestingpy.backtesting");
        py::module strategies = py::module::import("igtrader.Strategies");
        
        // Convert strategy parameters to Python dict
        py::dict py_params = mapToPythonDict(strategyParams);
        
        // Get the strategy class
        py::object strategy_module;
        if (strategyClass == "BuyHeikinGreenBA") {
            strategy_module = py::module::import("igtrader.Strategies.BuyHeikinGreen");
        } else if (strategyClass == "SellHeikinRedBA") {
            strategy_module = py::module::import("igtrader.Strategies.SellHeikinRed");
        } else if (strategyClass == "CrossEMABA") {
            strategy_module = py::module::import("igtrader.Strategies.CrossEMA");
        } else {
            setError(QString("Unknown strategy class: %1").arg(strategyClass));
            return nullptr;
        }
        
        py::object strategy_class = strategy_module.attr(strategyClass.toStdString().c_str());
        
        // Create and run backtest
        py::object bt = backtesting.attr("Backtest")(
            reinterpret_cast<py::object*>(data),
            strategy_class,
            py::arg("cash") = cash,
            py::arg("commission") = spread
        );
        
        // Run the backtest
        py::object result = bt.attr("run")(**py_params);
        
        qDebug() << "Backtest completed successfully";
        return new py::object(result);
        
    } catch (const py::error_already_set& e) {
        setError(QString("Backtest execution failed: %1").arg(e.what()));
        return nullptr;
    } catch (const std::exception& e) {
        setError(QString("Backtest execution failed: %1").arg(e.what()));
        return nullptr;
    }
}

void PyBindingManager::freeBacktestResults(void* data, void* stats)
{
    if (data) {
        delete reinterpret_cast<py::object*>(data);
    }
    if (stats) {
        delete reinterpret_cast<py::object*>(stats);
    }
}

QVariant PyBindingManager::getStatValue(void* stats, const QString& key)
{
    if (!stats) {
        return QVariant();
    }
    
    try {
        py::object* stats_obj = reinterpret_cast<py::object*>(stats);
        if (stats_obj->contains(key.toStdString().c_str())) {
            py::object value = (*stats_obj)[key.toStdString().c_str()];
            return pythonToVariant(&value);
        }
    } catch (const py::error_already_set& e) {
        qDebug() << "Error getting stat value:" << e.what();
    }
    
    return QVariant();
}

QList<QVariantMap> PyBindingManager::getTrades(void* stats)
{
    QList<QVariantMap> trades;
    
    if (!stats) {
        return trades;
    }
    
    try {
        py::object* stats_obj = reinterpret_cast<py::object*>(stats);
        if (stats_obj->contains("_trades")) {
            py::object trades_obj = (*stats_obj)["_trades"];
            // Convert DataFrame to list of dicts
            py::object to_dict = trades_obj.attr("to_dict");
            py::object records = to_dict("records");
            
            for (auto trade : records) {
                QVariantMap trade_map;
                py::dict trade_dict = trade.cast<py::dict>();
                for (auto item : trade_dict) {
                    QString key = QString::fromStdString(item.first.cast<std::string>());
                    QVariant value = pythonToVariant(&item.second);
                    trade_map[key] = value;
                }
                trades.append(trade_map);
            }
        }
    } catch (const py::error_already_set& e) {
        qDebug() << "Error getting trades:" << e.what();
    }
    
    return trades;
}

QList<QVariantMap> PyBindingManager::getEquity(void* stats)
{
    QList<QVariantMap> equity;
    
    if (!stats) {
        return equity;
    }
    
    try {
        py::object* stats_obj = reinterpret_cast<py::object*>(stats);
        if (stats_obj->contains("_equity_curve")) {
            py::object equity_obj = (*stats_obj)["_equity_curve"];
            // Convert DataFrame to list of dicts
            py::object to_dict = equity_obj.attr("to_dict");
            py::object records = to_dict("records");
            
            for (auto eq : records) {
                QVariantMap eq_map;
                py::dict eq_dict = eq.cast<py::dict>();
                for (auto item : eq_dict) {
                    QString key = QString::fromStdString(item.first.cast<std::string>());
                    QVariant value = pythonToVariant(&item.second);
                    eq_map[key] = value;
                }
                equity.append(eq_map);
            }
        }
    } catch (const py::error_already_set& e) {
        qDebug() << "Error getting equity:" << e.what();
    }
    
    return equity;
}

void* PyBindingManager::variantToPython(const QVariant& var)
{
    py::object obj = qVariantToPython(var);
    return new py::object(obj);
}

QVariant PyBindingManager::pythonToVariant(void* pyObj)
{
    if (!pyObj) {
        return QVariant();
    }
    
    try {
        py::object* obj = reinterpret_cast<py::object*>(pyObj);
        return pythonToQVariant(*obj);
    } catch (const py::error_already_set& e) {
        qDebug() << "Error converting Python to QVariant:" << e.what();
        return QVariant();
    }
}

bool PyBindingManager::strategyExists(const QString& strategyName)
{
    if (!m_initialized) {
        return false;
    }
    
    try {
        QString moduleName;
        if (strategyName == "BuyHeikinGreenBA") {
            moduleName = "igtrader.Strategies.BuyHeikinGreen";
        } else if (strategyName == "SellHeikinRedBA") {
            moduleName = "igtrader.Strategies.SellHeikinRed";
        } else if (strategyName == "CrossEMABA") {
            moduleName = "igtrader.Strategies.CrossEMA";
        } else {
            return false;
        }
        
        py::module strategy_module = py::module::import(moduleName.toStdString().c_str());
        py::object strategy_class = strategy_module.attr(strategyName.toStdString().c_str());
        
        return true;
    } catch (const py::error_already_set& e) {
        return false;
    }
}

void* PyBindingManager::loadData(
    const QString& symbol,
    const QString& interval,
    const QString& period,
    const QString& endDate,
    const QTime& tradingFrom,
    const QTime& tradingTo)
{
    if (!m_initialized) {
        setError("Python interpreter not initialized");
        return nullptr;
    }
    
    try {
        qDebug() << "Loading data with symbol:" << symbol << "interval:" << interval << "period:" << period;
        
        // Import pandas and helpers
        py::module pd = py::module::import("pandas");
        py::module helpers = py::module::import("igtrader.Helpers");
        
        // Call load_data function
        py::object load_data_func = helpers.attr("load_data");
        
        // Convert QTime to Python time objects
        auto time_module = py::module::import("datetime");
        py::object py_trading_from = time_module.attr("time")(
            tradingFrom.hour(), tradingFrom.minute(), tradingFrom.second()
        );
        py::object py_trading_to = time_module.attr("time")(
            tradingTo.hour(), tradingTo.minute(), tradingTo.second()
        );
        
        // Load the data
        py::object df = load_data_func(
            symbol.toStdString(),
            interval.toStdString(),
            period.toStdString(),
            endDate.toStdString(),
            py_trading_from,
            py_trading_to
        );
        
        // Convert index to datetime
        df.attr("index") = pd.attr("to_datetime")(df.attr("index"));
        
        return new py::object(df);
        
    } catch (const py::error_already_set& e) {
        setError(QString("Error loading data: %1").arg(e.what()));
        return nullptr;
    } catch (const std::exception& e) {
        setError(QString("Error loading data: %1").arg(e.what()));
        return nullptr;
    }
}

py::dict PyBindingManager::mapToPythonDict(const QMap<QString, QVariant>& map)
{
    if (!isInitialized()) {
        setError("Python non initialisé");
        return py::dict();
    }
    
    try {
        py::dict result;
        
        for (auto it = map.begin(); it != map.end(); ++it) {
            const QString& key = it.key();
            const QVariant& value = it.value();
            
            // CORRECTION: Convertir la clé en pybind11::str
            result[py::str(key.toStdString())] = qVariantToPython(value);
        }
        
        return result;
    }
    catch (const std::exception& e) {
        setError(QString("Erreur lors de la conversion dict: %1").arg(e.what()));
        return py::dict();
    }
}

// Méthodes non implémentées (stubs)
void* PyBindingManager::executePythonScript(const QString& scriptPath, 
                                           const QMap<QString, QVariant>& params)
{
    Q_UNUSED(scriptPath)
    Q_UNUSED(params)
    // TODO: Implémenter
    return nullptr;
}

bool PyBindingManager::loadModule(const QString& moduleName)
{
    Q_UNUSED(moduleName)
    // TODO: Implémenter
    return false;
}

void* PyBindingManager::callFunction(const QString& moduleName, 
                                    const QString& functionName,
                                    const QMap<QString, QVariant>& args)
{
    Q_UNUSED(moduleName)
    Q_UNUSED(functionName)
    Q_UNUSED(args)
    // TODO: Implémenter
    return nullptr;
}

pybind11::object PyBindingManager::qVariantToPython(const QVariant& value)
{
    switch (value.type()) {
        case QVariant::Bool:
            return py::bool_(value.toBool());
        case QVariant::Int:
            return py::int_(value.toInt());
        case QVariant::Double:
            return py::float_(value.toDouble());
        case QVariant::String:
            return py::str(value.toString().toStdString());
        case QVariant::Time: {
            QTime time = value.toTime();
            auto time_module = py::module::import("datetime");
            return time_module.attr("time")(time.hour(), time.minute(), time.second());
        }
        case QVariant::Date: {
            QDate date = value.toDate();
            auto date_module = py::module::import("datetime");
            return date_module.attr("date")(date.year(), date.month(), date.day());
        }
        case QVariant::DateTime: {
            QDateTime datetime = value.toDateTime();
            auto datetime_module = py::module::import("datetime");
            return datetime_module.attr("datetime")(
                datetime.date().year(), datetime.date().month(), datetime.date().day(),
                datetime.time().hour(), datetime.time().minute(), datetime.time().second()
            );
        }
        case QVariant::List: {
            py::list py_list;
            QVariantList list = value.toList();
            for (const auto& item : list) {
                py_list.append(qVariantToPython(item));
            }
            return py_list;
        }
        default:
            return py::none();
    }
}

QVariant PyBindingManager::pythonToQVariant(const pybind11::object& obj)
{
    try {
        if (py::isinstance<py::bool_>(obj)) {
            return QVariant(obj.cast<bool>());
        } else if (py::isinstance<py::int_>(obj)) {
            return QVariant(obj.cast<int>());
        } else if (py::isinstance<py::float_>(obj)) {
            return QVariant(obj.cast<double>());
        } else if (py::isinstance<py::str>(obj)) {
            return QVariant(QString::fromStdString(obj.cast<std::string>()));
        } else if (py::isinstance<py::list>(obj)) {
            QVariantList list;
            for (auto item : obj) {
                // CORRECTION: Convertir pybind11::handle en pybind11::object
                py::object item_obj = py::reinterpret_borrow<py::object>(item);
                list.append(pythonToQVariant(item_obj));
            }
            return QVariant(list);
        } else {
            return QVariant();
        }
    } catch (const py::error_already_set& e) {
        qDebug() << "Error converting Python object to QVariant:" << e.what();
        return QVariant();
    }
}

QString PyBindingManager::getLastError() const
{
    return m_lastError;
}

void PyBindingManager::clearError()
{
    m_lastError.clear();
}

void PyBindingManager::setError(const QString& error)
{
    m_lastError = error;
    qDebug() << "PyBindingManager error:" << error;
}

bool PyBindingManager::handlePythonException()
{
    try {
        if (PyErr_Occurred()) {
            PyErr_Print();
            return true;
        }
    } catch (...) {
        return true;
    }
    return false;
}