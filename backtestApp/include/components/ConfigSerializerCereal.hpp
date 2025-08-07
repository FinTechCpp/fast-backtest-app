#pragma once

#include <QFile>
#include <QDir>
#include <QDebug>
#include <sstream>
#include <fstream>
#include "components/configSerializerAdapters.h"
#include <cereal/archives/json.hpp>

// Structure pour contenir toutes les configurations
struct ProfileConfig {
    std::string name;
    std::string version;
    std::string createdAt;
    GeneralParamsConfig generalParams;
    StrategyBaseConfig baseConfig;
    BuyHeikinGreenConfig buyConfig;
    SellHeikinRedConfig sellConfig;
};

// Sérialisation pour ProfileConfig
namespace cereal {
    template<class Archive>
    void serialize(Archive & ar, ProfileConfig & config) {
        ar(make_nvp("name", config.name),
           make_nvp("version", config.version),
           make_nvp("createdAt", config.createdAt),
           make_nvp("generalParams", config.generalParams),
           make_nvp("baseConfig", config.baseConfig),
           make_nvp("buyConfig", config.buyConfig),
           make_nvp("sellConfig", config.sellConfig));
    }
}

class ConfigSerializerCereal {
public:
    // Méthodes spécifiques pour ProfileConfig
    static bool saveProfile(const QString& filePath, const ProfileConfig& config) {
        return saveToJsonFile(filePath, config);
    }
    
    static bool loadProfile(const QString& filePath, ProfileConfig& config) {
        return loadFromJsonFile(filePath, config);
    }
    
    // Méthode utilitaire pour convertir une config en chaîne JSON
    template <typename T>
    static QString toJsonString(const T& config) {
        std::stringstream ss;
        try {
            {
                cereal::JSONOutputArchive archive(ss);
                archive(cereal::make_nvp("config", config));
            }
            return QString::fromStdString(ss.str());
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors de la conversion en JSON:" << e.what();
            return QString();
        }
    }
    
    // Méthode utilitaire pour parser une chaîne JSON en config
    template <typename T>
    static bool fromJsonString(const QString& jsonString, T& config) {
        std::stringstream ss(jsonString.toStdString());
        try {
            cereal::JSONInputArchive archive(ss);
            archive(cereal::make_nvp("config", config));
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors de l'analyse JSON:" << e.what();
            return false;
        }
    }
    
    template <typename T>
    static bool saveToJsonFile(const QString& filePath, const T& config) {
        try {
            std::ofstream os(filePath.toStdString());
            if (!os.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier:" << filePath;
                return false;
            }
            
            cereal::JSONOutputArchive archive(os);
            archive(cereal::make_nvp("config", config));
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors de la sauvegarde:" << e.what();
            return false;
        }
    }
    
    template <typename T>
    static bool loadFromJsonFile(const QString& filePath, T& config) {
        try {
            std::ifstream is(filePath.toStdString());
            if (!is.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier:" << filePath;
                return false;
            }
            
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("config", config));
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors du chargement:" << e.what();
            return false;
        }
    }
};