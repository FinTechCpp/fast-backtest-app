// CSV -> Cereal Binary converter
//
// Standalone tool (no Qt dependency) that scans a directory of OHLC CSV
// files and produces one Cereal-serialized .bin file per CSV in the output
// directory, using the same base filename (e.g. "NDX_10secs_....csv" ->
// "NDX_10secs_....bin").
//
// Supported CSV formats:
//   1) timestamp_unix,open,high,low,close[,volume]
//   2) index,date_iso,open,high,low,close[,volume]
//      where date_iso looks like "2025-05-16 18:43:10+00:00"
//
// Usage:
//   csv_to_bin_converter <csv_dir> <bin_dir>

#include "BinaryBar.h"

#include <cereal/archives/binary.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::vector<std::string> splitCommas(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == ',') {
            fields.push_back(line.substr(start, i - start));
            start = i + 1;
        }
    }
    fields.push_back(line.substr(start));
    return fields;
}

bool looksLikeIsoDate(const std::string& field) {
    // "YYYY-MM-DD..." -> position 4 and 7 are '-'
    return field.size() >= 10 && field[4] == '-' && field[7] == '-';
}

// Parses "YYYY-MM-DD HH:MM:SS[+HH:MM|-HH:MM]" into a Unix timestamp in ms (UTC).
bool parseIsoDateToMs(const std::string& field, int64_t& outMs) {
    int year, month, day, hour, minute, second;
    char sep = ' ';
    int parsed = std::sscanf(field.c_str(), "%d-%d-%d%c%d:%d:%d",
                              &year, &month, &day, &sep, &hour, &minute, &second);
    if (parsed < 7) return false;

    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

#if defined(_WIN32)
    time_t utc = _mkgmtime(&tm);
#else
    time_t utc = timegm(&tm);
#endif
    if (utc == static_cast<time_t>(-1)) return false;

    int64_t offsetSeconds = 0;
    size_t offsetPos = field.find_first_of("+-", 10); // search after date/time part
    if (offsetPos != std::string::npos) {
        int offSign = (field[offsetPos] == '-') ? -1 : 1;
        int offHour = 0, offMinute = 0;
        if (std::sscanf(field.c_str() + offsetPos + 1, "%d:%d", &offHour, &offMinute) >= 1) {
            offsetSeconds = offSign * (offHour * 3600 + offMinute * 60);
        }
    }

    outMs = (static_cast<int64_t>(utc) - offsetSeconds) * 1000;
    return true;
}

bool parseDouble(const std::string& field, double& out) {
    if (field.empty()) { out = 0.0; return true; }
    try {
        out = std::stod(field);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseLine(const std::string& line, BinaryBar& bar) {
    if (line.size() < 10) return false;
    std::vector<std::string> fields = splitCommas(line);
    if (fields.size() < 5) return false;

    if (looksLikeIsoDate(fields[1])) {
        // index,date,open,high,low,close[,volume]
        if (fields.size() < 6) return false;
        if (!parseIsoDateToMs(fields[1], bar.timestampMs)) return false;
        if (!parseDouble(fields[2], bar.open)) return false;
        if (!parseDouble(fields[3], bar.high)) return false;
        if (!parseDouble(fields[4], bar.low)) return false;
        if (!parseDouble(fields[5], bar.close)) return false;
        bar.volume = (fields.size() >= 7) ? std::stod(fields[6].empty() ? "0" : fields[6]) : 0.0;
        return true;
    }

    // Legacy: timestamp_unix,open,high,low,close[,volume]
    try {
        bar.timestampMs = std::stoll(fields[0]) * 1000;
    } catch (...) {
        return false;
    }
    if (!parseDouble(fields[1], bar.open)) return false;
    if (!parseDouble(fields[2], bar.high)) return false;
    if (!parseDouble(fields[3], bar.low)) return false;
    if (!parseDouble(fields[4], bar.close)) return false;
    bar.volume = (fields.size() >= 6) ? std::stod(fields[5].empty() ? "0" : fields[5]) : 0.0;
    return true;
}

bool isHeaderLine(const std::string& line) {
    return line.rfind("date,", 0) == 0
        || line.rfind("timestamp,", 0) == 0
        || line.find(",date,") != std::string::npos
        || line.find(",timestamp,") != std::string::npos
        || line.find(",open,high,low,close") != std::string::npos;
}

bool convertFile(const fs::path& csvPath, const fs::path& binPath) {
    std::ifstream in(csvPath);
    if (!in.is_open()) {
        std::cerr << "  ERROR: cannot open " << csvPath << "\n";
        return false;
    }

    std::vector<BinaryBar> bars;
    bars.reserve(1 << 20);

    std::string line;
    bool first = true;
    size_t invalidLines = 0;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (first && isHeaderLine(line)) {
            first = false;
            continue;
        }
        first = false;

        BinaryBar bar;
        if (parseLine(line, bar)) {
            bars.push_back(bar);
        } else {
            ++invalidLines;
        }
    }

    if (bars.empty()) {
        std::cerr << "  ERROR: no valid rows parsed in " << csvPath << "\n";
        return false;
    }

    std::ofstream out(binPath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "  ERROR: cannot write " << binPath << "\n";
        return false;
    }

    {
        cereal::BinaryOutputArchive archive(out);
        BinaryFileHeader header;
        archive(header, bars);
    }

    std::cout << "  OK: " << csvPath.filename() << " -> " << binPath.filename()
              << " (" << bars.size() << " bars, " << invalidLines << " invalid lines skipped)\n";
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <csv_dir> <bin_dir>\n";
        return 1;
    }

    fs::path csvDir = argv[1];
    fs::path binDir = argv[2];

    if (!fs::exists(csvDir) || !fs::is_directory(csvDir)) {
        std::cerr << "ERROR: CSV directory does not exist: " << csvDir << "\n";
        return 1;
    }

    std::error_code ec;
    fs::create_directories(binDir, ec);

    int converted = 0;
    int failed = 0;

    for (const auto& entry : fs::directory_iterator(csvDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".csv") continue;

        fs::path binPath = binDir / entry.path().filename();
        binPath.replace_extension(".bin");

        std::cout << "Converting " << entry.path().filename() << "...\n";
        if (convertFile(entry.path(), binPath)) {
            ++converted;
        } else {
            ++failed;
        }
    }

    std::cout << "\nDone. " << converted << " file(s) converted, " << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
