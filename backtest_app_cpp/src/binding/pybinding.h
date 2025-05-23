#ifndef PYBINDING_H
#define PYBINDING_H

// Include Qt headers first - before any Python includes
#include <QMap>
#include <QVariant>
#include <QTime>
#include <QDate>
#include <QDebug>
#include <QString>
#include <QObject>
#include <QList>

// Temporarily undefine Qt's slots for Python compatibility
#pragma push_macro("slots")
#undef slots

// Include Python headers
#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

// Restore Qt's slots definition
#pragma pop_macro("slots")

#include <string>
#include <functional>
#include <memory>

// Forward declarations
class QWidget;

/**
 * @brief Gestionnaire pour l'interface Python-C++
 */
class PyBindingManager : public QObject
{
    Q_OBJECT

public:
    static PyBindingManager* getInstance();
    
    // AJOUT: Déclaration explicite du constructeur et destructeur
    PyBindingManager();
    virtual ~PyBindingManager();
    
    bool initialize();
    void finalize();
    bool isInitialized() const;
    
    // Méthodes pour le backtest
    void* runBacktest(
        void* data,
        const QString& strategyClass,
        double cash,
        double spread,
        const QMap<QString, QVariant>& strategyParams);
    
    void freeBacktestResults(void* data, void* stats);
    
    // Méthodes pour récupérer les résultats
    QVariant getStatValue(void* stats, const QString& key);
    QList<QVariantMap> getTrades(void* stats);
    QList<QVariantMap> getEquity(void* stats);
    
    // Méthodes utilitaires
    bool strategyExists(const QString& strategyName);
    void* loadData(
        const QString& symbol,
        const QString& interval,
        const QString& period,
        const QString& endDate,
        const QTime& tradingFrom,
        const QTime& tradingTo);
    
    // Conversion de types
    void* variantToPython(const QVariant& var);
    QVariant pythonToVariant(void* pyObj);
    pybind11::dict mapToPythonDict(const QMap<QString, QVariant>& map);
    
    // Exécution de scripts Python
    void* executePythonScript(const QString& scriptPath, 
                             const QMap<QString, QVariant>& params);
    bool loadModule(const QString& moduleName);
    void* callFunction(const QString& moduleName, 
                      const QString& functionName,
                      const QMap<QString, QVariant>& args);
    pybind11::object qVariantToPython(const QVariant& value);
    QVariant pythonToQVariant(const pybind11::object& obj);
    QString getLastError() const;
    void clearError();

private:
    static PyBindingManager* instance;
    bool m_initialized;
    QString m_lastError;
    void* m_pyHelpers;
    void* m_pyBacktest;
    
    void setError(const QString& error);
    bool handlePythonException();
};

#endif // PYBINDING_H