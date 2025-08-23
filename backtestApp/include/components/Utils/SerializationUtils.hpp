#pragma once

#include <QFile>
#include <QDir>
#include <QDebug>
#include <sstream>
#include <fstream>
#include "components/serializerAdapters.h"
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>

class SerializationUtils {
public:
    enum class FileFormat {
        JSON,
        Binary,
        PortableBinary,
        Auto // Pour la détection automatique basée sur l'extension
    };

    template <typename T>
    static bool saveToFile(const QString& filePath, const T& object, FileFormat format = FileFormat::Auto) {
        FileFormat actualFormat = format;
        
        if (format == FileFormat::Auto) {
            actualFormat = detectFormatFromExtension(filePath);
        }
        
        switch (actualFormat) {
            case FileFormat::JSON:
                return saveToJsonFile(filePath, object);
            case FileFormat::Binary:
                return saveToBinaryFile(filePath, object);
            case FileFormat::PortableBinary:
                return saveToPortableBinaryFile(filePath, object);
            default:
                return false;
        }
    }
    
    template <typename T>
    static bool loadFromFile(const QString& filePath, T& object, FileFormat format = FileFormat::Auto) {
        FileFormat actualFormat = format;
        
        if (format == FileFormat::Auto) {
            actualFormat = detectFormatFromExtension(filePath);
        }
        
        switch (actualFormat) {
            case FileFormat::JSON:
                return loadFromJsonFile(filePath, object);
            case FileFormat::Binary:
                return loadFromBinaryFile(filePath, object);
            case FileFormat::PortableBinary:
                return loadFromPortableBinaryFile(filePath, object);
            default:
                return false;
        }
    }

public:
    static FileFormat detectFormatFromExtension(const QString& filePath) {
        if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
            return FileFormat::JSON;
        } else if (filePath.endsWith(".bin", Qt::CaseInsensitive)) {
            return FileFormat::Binary;
        } else if (filePath.endsWith(".pbin", Qt::CaseInsensitive)) {
            return FileFormat::PortableBinary;
        } else {
            qWarning() << "Extension de fichier inconnue, utilisation du format JSON par défaut pour:" << filePath;
        }
        // Par défaut, utiliser JSON
        return FileFormat::JSON;
    }

    static QString getExtensionForFormat(FileFormat format) {
        switch (format) {
            case FileFormat::JSON: return ".json";
            case FileFormat::Binary: return ".bin";
            case FileFormat::PortableBinary: return ".pbin";
            default: return ".json"; // Par défaut
        }
    }

// private:
    //////////////////////
    // JSON             //
    //////////////////////
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

    //////////////////////
    // Binary           //
    //////////////////////
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

    //////////////////////
    // Portable Binary  //
    //////////////////////
    template <typename T>
    static bool saveToPortableBinaryFile(const QString& filePath, const T& object) {
        try {
            std::ofstream os(filePath.toStdString(), std::ios::binary);
            if (!os.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier pour écriture:" << filePath;
                return false;
            }

            cereal::PortableBinaryOutputArchive archive(os);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors de la sauvegarde binaire portable:" << e.what();
            return false;
        }
    };

    template <typename T>
    static bool loadFromPortableBinaryFile(const QString& filePath, T& object) {
        try {
            std::ifstream is(filePath.toStdString(), std::ios::binary);
            if (!is.is_open()) {
                qWarning() << "Impossible d'ouvrir le fichier pour lecture:" << filePath;
                return false;
            }

            cereal::PortableBinaryInputArchive archive(is);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Erreur lors du chargement binaire portable:" << e.what();
            return false;
        }
    }
};