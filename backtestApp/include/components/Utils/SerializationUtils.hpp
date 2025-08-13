#pragma once

#include <QFile>
#include <QDir>
#include <QDebug>
#include <sstream>
#include <fstream>
#include "components/serializerAdapters.h"
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

class SerializationUtils {
public:
    template <typename T>
    static bool saveToJsonFile(const QString& filePath, const T& config) {
        try {
            std::ofstream os(filePath.toStdString());
            if (!os.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier:" << filePath;
                return false;
            }
            
            cereal::JSONOutputArchive archive(os);
            archive(CEREAL_NVP(config));
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
            archive(CEREAL_NVP(config));
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors du chargement:" << e.what();
            return false;
        }
    }

    // Nouvelles méthodes pour l'enregistrement binaire
    template <typename T>
    static bool saveToBinaryFile(const QString& filePath, const T& object) {
        try {
            std::ofstream os(filePath.toStdString(), std::ios::binary);
            if (!os.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier pour écriture:" << filePath;
                return false;
            }
            
            cereal::BinaryOutputArchive archive(os);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors de la sauvegarde binaire:" << e.what();
            return false;
        }
    }
    
    template <typename T>
    static bool loadFromBinaryFile(const QString& filePath, T& object) {
        try {
            std::ifstream is(filePath.toStdString(), std::ios::binary);
            if (!is.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier pour lecture:" << filePath;
                return false;
            }
            
            cereal::BinaryInputArchive archive(is);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors du chargement binaire:" << e.what();
            return false;
        }
    }
};