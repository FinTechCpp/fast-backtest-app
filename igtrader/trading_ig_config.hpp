#ifndef TRADING_IG_CONFIG_HPP
#define TRADING_IG_CONFIG_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <vector>

/**
 * Configuration class for IG trading bot.
 * 
 * This class retrieves configuration values from environment variables
 * and .env file to set up the IG trading service.
 */
class Config {
private:
    std::unordered_map<std::string, std::string> env_vars;
    
    /**
     * Load environment variables from .env file
     */
    void loadDotEnv(const std::string& filename = ".env") {
        // Define possible file locations to search
        std::vector<std::string> paths = {
            filename,                                  // Current directory
            "../" + filename,                          // Parent directory
            "../../" + filename,                       // Project root (from build/cpp_apps)
            "/Users/macmax/repos/ig-trading-bot/.env"  // Absolute path as fallback
        };
        
        bool found = false;
        
        // Try each path until we find the file
        for (const auto& path : paths) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::cout << "Loading configuration from: " << path << std::endl;
                std::string line;
                
                while (std::getline(file, line)) {
                    // Skip empty lines and comments
                    if (line.empty() || line[0] == '#' || line[0] == '/') {
                        continue;
                    }
                    
                    size_t pos = line.find('=');
                    if (pos != std::string::npos) {
                        std::string key = line.substr(0, pos);
                        std::string value = line.substr(pos + 1);
                        
                        // Remove leading/trailing whitespace
                        key.erase(0, key.find_first_not_of(" \t"));
                        key.erase(key.find_last_not_of(" \t") + 1);
                        value.erase(0, value.find_first_not_of(" \t"));
                        value.erase(value.find_last_not_of(" \t") + 1);
                        
                        env_vars[key] = value;
                    }
                }
                file.close();
                found = true;
                break;
            }
        }
        
        if (!found) {
            std::cerr << "Warning: Could not open " << filename << " file in any known location" << std::endl;
        }
    }
    
    /**
     * Get environment variable value, first from .env file, then from system env
     */
    std::string getEnv(const std::string& key) const {
        // First check .env file
        auto it = env_vars.find(key);
        if (it != env_vars.end()) {
            return it->second;
        }
        
        // Then check system environment variables
        const char* env_val = std::getenv(key.c_str());
        return env_val ? std::string(env_val) : "";
    }

public:
    std::string username;
    std::string password;
    std::string api_key;
    std::string acc_type;
    std::string acc_number;
    
    /**
     * Constructor that loads configuration from .env file and environment variables
     */
    Config() {
        loadDotEnv();
        
        username = getEnv("IG_SERVICE_USERNAME");
        password = getEnv("IG_SERVICE_PASSWORD");
        api_key = getEnv("IG_SERVICE_API_KEY");
        acc_type = getEnv("IG_SERVICE_ACC_TYPE");
        acc_number = getEnv("IG_SERVICE_ACC_NUMBER");
    }
};

#endif // TRADING_IG_CONFIG_HPP