#ifndef PYBINDING_H
#define PYBINDING_H

// CORRECTION: Définir Qt slots avant Python pour éviter le conflit
#ifdef slots
#undef slots
#endif

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <vector>
#include "../data_loader.h"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

// Redéfinir slots pour Qt après les includes Python
#ifndef QT_NO_KEYWORDS
#define slots Q_SLOTS
#endif

namespace py = pybind11;

class PyBindingManager
{
public:
    static PyBindingManager& getInstance();
    
    bool initialize();
    bool isInitialized() const;
    
    // Méthode principale - simple appel au backtest Python
    QVariant runPythonBacktest(const std::vector<OHLCBar>& data,
                              const QString& strategyClass,
                              double cash,
                              double spread,
                              const QMap<QString, QVariant>& strategyParams);
    
    // Récupération des résultats
    QList<QVariantMap> getTrades(void* stats);
    QList<QVariantMap> getEquityCurve(void* stats);
    QVariant getStatValue(void* stats, const QString& key);
    
    // Gestion des erreurs
    QString getLastError() const;
    void clearError();
    
    // AJOUT: Méthode pour finaliser Python
    void finalize();

private:
    PyBindingManager();
    ~PyBindingManager();
    
    static PyBindingManager* instance;
    bool m_initialized;
    QString m_lastError;
    // CORRECTION: Supprimer m_lastResults pour éviter l'avertissement de visibilité
    
    // Méthodes utilitaires
    py::dict convertParamsToPython(const QMap<QString, QVariant>& params);
    QVariant pythonToQVariant(const py::object& obj);
    QString findProjectRoot(const QString& startPath);
    void setError(const QString& error);
};

#endif // PYBINDING_H