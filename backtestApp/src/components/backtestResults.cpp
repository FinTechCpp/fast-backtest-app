// #include "components/backtestResults.h"
// #include <openssl/sha.h>
// #include <sstream>
// #include <iomanip>

// // Implémentation de la méthode de génération de hash
// std::string generateDataHash(const be::Data& data) {
//     unsigned char hash[SHA256_DIGEST_LENGTH];
//     SHA256_CTX sha256;
//     SHA256_Init(&sha256);
    
//     // Hash des données OHLCV
//     for (size_t i = 0; i < data.size(); i++) {
//         std::string candle = data.at(i).date.toString() + 
//                            std::to_string(data.at(i).open) +
//                            std::to_string(data.at(i).high) +
//                            std::to_string(data.at(i).low) +
//                            std::to_string(data.at(i).close);
//         SHA256_Update(&sha256, candle.c_str(), candle.size());
//     }
    
//     SHA256_Final(hash, &sha256);
    
//     // Convertir le hash en hexadécimal
//     std::stringstream ss;
//     for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
//         ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
//     }
    
//     return ss.str();
// }

// // Création d'une référence à partir des données
// DataReference DataReference::fromData(const be::Data& data, 
//                                     const std::string& symbol,
//                                     const std::string& timeframe, 
//                                     const std::string& period) {
//     DataReference ref;
//     ref.symbol = symbol;
//     ref.timeframe = timeframe;
//     ref.period = period;
    
//     if (data.size() > 0) {
//         ref.firstDate = data.at(0).date;
//         ref.lastDate = data.at(data.size() - 1).date;
//         ref.candleCount = data.size();
//         ref.dataHash = generateDataHash(data);
//     }
    
//     return ref;
// }

// // Vérification si les données correspondent
// bool DataReference::matchesData(const be::Data& data) const {
//     // Vérification rapide de la taille
//     if (data.size() != candleCount) {
//         return false;
//     }
    
//     // Vérification des dates de début et fin
//     if (!(data.at(0).date == firstDate && data.at(data.size() - 1).date == lastDate)) {
//         return false;
//     }
    
//     // Vérification complète via le hash
//     std::string newHash = generateDataHash(data);
//     return newHash == dataHash;
// }