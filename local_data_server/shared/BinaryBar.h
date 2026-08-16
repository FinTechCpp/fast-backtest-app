#pragma once

// Shared binary representation of an OHLC bar, used by both:
//  - the CSV -> BIN converter (local_data_server/converter)
//  - the client application (backtestApp BinarySerializer)
//
// QDateTime is not Cereal-serializable, so the timestamp is stored as a
// plain Unix timestamp in milliseconds (UTC).

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cstdint>

struct BinaryBar {
    int64_t timestampMs = 0; // Unix timestamp in milliseconds (UTC)
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;

    template<class Archive>
    void serialize(Archive& ar) {
        ar(cereal::make_nvp("timestampMs", timestampMs),
           cereal::make_nvp("open", open),
           cereal::make_nvp("high", high),
           cereal::make_nvp("low", low),
           cereal::make_nvp("close", close),
           cereal::make_nvp("volume", volume));
    }
};

// Simple magic header written at the start of every .bin file so that the
// reader can quickly reject files that are not in the expected format,
// without relying solely on Cereal exceptions.
struct BinaryFileHeader {
    static constexpr char kMagic[4] = {'O', 'H', 'L', 'C'};
    static constexpr uint32_t kVersion = 1;

    char magic[4] = {'O', 'H', 'L', 'C'};
    uint32_t version = kVersion;

    template<class Archive>
    void serialize(Archive& ar) {
        ar(magic, version);
    }

    bool isValid() const {
        return magic[0] == 'O' && magic[1] == 'H' && magic[2] == 'L' && magic[3] == 'C'
            && version == kVersion;
    }
};
