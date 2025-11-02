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
        Auto // For automatic detection based on file extension
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
            qWarning() << "Unknown file extension, using JSON format by default for:" << filePath;
        }
        // Default to JSON
        return FileFormat::JSON;
    }

    static QString getExtensionForFormat(FileFormat format) {
        switch (format) {
            case FileFormat::JSON: return ".json";
            case FileFormat::Binary: return ".bin";
            case FileFormat::PortableBinary: return ".pbin";
            default: return ".json"; // Default
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
                qWarning() << "Unable to open file:" << filePath;
                return false;
            }
            
            cereal::JSONOutputArchive archive(os);
            archive(CEREAL_NVP(config));
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Error saving file:" << e.what();
            return false;
        }
    }
    
    template <typename T>
    static bool loadFromJsonFile(const QString& filePath, T& config) {
        try {
            std::ifstream is(filePath.toStdString());
            if (!is.is_open()) {
                qWarning() << "Unable to open file:" << filePath;
                return false;
            }
            
            cereal::JSONInputArchive archive(is);
            archive(CEREAL_NVP(config));
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Error loading file:" << e.what();
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
                qWarning() << "Unable to open file for writing:" << filePath;
                return false;
            }
            
            cereal::BinaryOutputArchive archive(os);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Error saving binary file:" << e.what();
            return false;
        }
    }
    
    template <typename T>
    static bool loadFromBinaryFile(const QString& filePath, T& object) {
        try {
            std::ifstream is(filePath.toStdString(), std::ios::binary);
            if (!is.is_open()) {
                qWarning() << "Unable to open file for reading:" << filePath;
                return false;
            }
            
            cereal::BinaryInputArchive archive(is);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Error loading binary file:" << e.what();
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
                qWarning() << "Unable to open file for writing:" << filePath;
                return false;
            }

            cereal::PortableBinaryOutputArchive archive(os);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Error saving portable binary file:" << e.what();
            return false;
        }
    };

    template <typename T>
    static bool loadFromPortableBinaryFile(const QString& filePath, T& object) {
        try {
            std::ifstream is(filePath.toStdString(), std::ios::binary);
            if (!is.is_open()) {
                qWarning() << "Unable to open file for reading:" << filePath;
                return false;
            }

            cereal::PortableBinaryInputArchive archive(is);
            archive(object);
            return true;
        }
        catch (const std::exception& e) {
            qWarning() << "Error loading portable binary file:" << e.what();
            return false;
        }
    }
};