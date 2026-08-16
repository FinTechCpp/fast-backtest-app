#pragma once

#include "components/Utils/dataLoader.h" // for OHLCBar

#include <QByteArray>
#include <QString>
#include <vector>

// Serialization layer only: converts between the raw bytes of a .bin file
// (Cereal binary archive of BinaryFileHeader + std::vector<BinaryBar>) and
// the application's std::vector<OHLCBar>. Knows nothing about HTTP/network.
class BinarySerializer
{
public:
    // Deserializes raw .bin bytes into OHLC bars.
    // Returns an empty vector and fills errorMessage on failure
    // (corrupted file, bad header/magic, Cereal exception, etc.).
    static std::vector<OHLCBar> deserialize(const QByteArray& data, QString& errorMessage);

    // Serializes OHLC bars into raw .bin bytes (mainly useful for tests).
    static QByteArray serialize(const std::vector<OHLCBar>& bars);
};
