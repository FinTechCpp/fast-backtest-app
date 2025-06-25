#pragma once

#include "Managers/LoggerManager.hpp"

// Définir ENABLE_LOGGING à 0 pour désactiver tous les logs
#ifndef ENABLE_LOGGING
#define ENABLE_LOGGING 1
#endif

// Logger conditionnel basé sur templates
template<bool Enabled>
class ConditionalLogger;

// Spécialisation pour le logging activé - utilise le vrai LoggerManager
template<>
class ConditionalLogger<true> {
private:
    std::unique_ptr<ILogger> logger;
    
public:
    ConditionalLogger() : logger(std::make_unique<LoggerManager>()) {}
    
    ILogger* operator->() {
        return logger.get();
    }
    
    // Pour les méthodes qui prennent logger en référence
    operator ILogger*() {
        return logger.get();
    }
};

// Spécialisation pour le logging désactivé - utilise le NullLogger
template<>
class ConditionalLogger<false> {
private:
    std::unique_ptr<ILogger> logger;
    
public:
    // Utilise le NullLogger qui ne fait rien
    ConditionalLogger() : logger(std::make_unique<NullLogger>()) {}
    
    ILogger* operator->() {
        return logger.get();
    }
    
    // Pour les méthodes qui prennent logger en référence
    operator ILogger*() {
        return logger.get();
    }
};

// Type à utiliser dans le code
using Logger = ConditionalLogger<ENABLE_LOGGING>;