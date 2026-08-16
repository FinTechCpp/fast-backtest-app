#include "components/Utils/binarySerializer.h"
#include "BinaryBar.h"

#include <cereal/archives/binary.hpp>

#include <QTimeZone>
#include <sstream>
#include <string>

std::vector<OHLCBar> BinarySerializer::deserialize(const QByteArray& data, QString& errorMessage)
{
    std::vector<OHLCBar> bars;

    if (data.isEmpty()) {
        errorMessage = "Cannot deserialize empty binary data";
        return bars;
    }

    try {
        std::string buffer(data.constData(), static_cast<size_t>(data.size()));
        std::istringstream stream(buffer, std::ios::binary);
        cereal::BinaryInputArchive archive(stream);

        BinaryFileHeader header;
        std::vector<BinaryBar> binaryBars;
        archive(header, binaryBars);

        if (!header.isValid()) {
            errorMessage = "Invalid or unsupported binary file header (bad magic/version)";
            return {};
        }

        bars.reserve(binaryBars.size());
        static const QTimeZone utcZone = QTimeZone::utc();
        for (const BinaryBar& b : binaryBars) {
            QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(b.timestampMs, utcZone);
            bars.emplace_back(timestamp, b.open, b.high, b.low, b.close, b.volume);
        }
    } catch (const std::exception& ex) {
        errorMessage = QString("Failed to deserialize binary data: %1").arg(ex.what());
        return {};
    }

    errorMessage.clear();
    return bars;
}

QByteArray BinarySerializer::serialize(const std::vector<OHLCBar>& bars)
{
    std::vector<BinaryBar> binaryBars;
    binaryBars.reserve(bars.size());
    for (const OHLCBar& bar : bars) {
        BinaryBar b;
        b.timestampMs = bar.timestamp.toMSecsSinceEpoch();
        b.open = bar.open;
        b.high = bar.high;
        b.low = bar.low;
        b.close = bar.close;
        b.volume = bar.volume;
        binaryBars.push_back(b);
    }

    std::ostringstream stream(std::ios::binary);
    {
        cereal::BinaryOutputArchive archive(stream);
        BinaryFileHeader header;
        archive(header, binaryBars);
    }

    std::string result = stream.str();
    return QByteArray(result.data(), static_cast<int>(result.size()));
}
