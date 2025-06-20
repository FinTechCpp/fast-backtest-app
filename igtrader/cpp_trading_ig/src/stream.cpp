#include "stream.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

namespace ig {

// Initialize static constants
const std::vector<std::string> TickerSubscription::TICKER_FIELDS = {
    "BID", "OFR", "LTP", "LTV", "TTV", "UTM", 
    "DAY_OPEN_MID", "DAY_NET_CHG_MID", "DAY_PERC_CHG_MID", 
    "DAY_HIGH", "DAY_LOW"
};

// Convert string to double, handling empty strings and errors
double parseDouble(const std::string& str, double default_value = 0.0) {
    if (str.empty()) {
        return default_value;
    }
    
    try {
        return std::stod(str);
    } catch (...) {
        return default_value;
    }
}

// Convert string to int, handling empty strings and errors
int parseInt(const std::string& str, int default_value = 0) {
    if (str.empty()) {
        return default_value;
    }
    
    try {
        return std::stoi(str);
    } catch (...) {
        return default_value;
    }
}

// Implementation of Ticker
void Ticker::populate(const std::unordered_map<std::string, std::string>& values) {
    // Process timestamp
    auto it = values.find("UTM");
    if (it != values.end() && !it->second.empty()) {
        timestamp = it->second;
    }
    
    // Process numeric fields
    bid = parseDouble(values.count("BID") ? values.at("BID") : "");
    offer = parseDouble(values.count("OFR") ? values.at("OFR") : "");
    last_traded_price = parseDouble(values.count("LTP") ? values.at("LTP") : "");
    last_traded_volume = parseInt(values.count("LTV") ? values.at("LTV") : "");
    incr_volume = parseInt(values.count("TTV") ? values.at("TTV") : "");
    day_open_mid = parseDouble(values.count("DAY_OPEN_MID") ? values.at("DAY_OPEN_MID") : "");
    day_net_change_mid = parseDouble(values.count("DAY_NET_CHG_MID") ? values.at("DAY_NET_CHG_MID") : "");
    day_percent_change_mid = parseDouble(values.count("DAY_PERC_CHG_MID") ? values.at("DAY_PERC_CHG_MID") : "");
    day_high = parseDouble(values.count("DAY_HIGH") ? values.at("DAY_HIGH") : "");
    day_low = parseDouble(values.count("DAY_LOW") ? values.at("DAY_LOW") : "");
}

// Implementation of TickerSubscription
TickerSubscription::TickerSubscription(const std::string& epic)
    : Subscription("DISTINCT", {std::string("CHART:") + epic + ":TICK"}, TICKER_FIELDS) {
}

// Implementation of TickerListener's onItemUpdate method
void TickerListener::onItemUpdate(const std::string& item_name, 
                                 const std::unordered_map<std::string, std::string>& values) {
    // Extract epic from item name (format: CHART:<epic>:TICK)
    std::string epic;
    size_t first_colon = item_name.find(':');
    if (first_colon != std::string::npos) {
        size_t second_colon = item_name.find(':', first_colon + 1);
        if (second_colon != std::string::npos) {
            epic = item_name.substr(first_colon + 1, second_colon - first_colon - 1);
        }
    }
    
    if (epic.empty()) {
        spdlog::error("Could not extract epic from item name: {}", item_name);
        return;
    }
    
    // Create and populate ticker
    Ticker ticker(epic);
    ticker.populate(values);
    
    // Notify derived class
    onTicker(ticker);
}

// Implementation of IGStreamService
IGStreamService::IGStreamService(IGService& ig_service)
    : ig_service_(ig_service) {
}

IGStreamService::~IGStreamService() {
    disconnect();
}

void IGStreamService::connect(const std::string& account_id) {
    if (ls_client_ && ls_client_->isConnected()) {
        spdlog::info("Already connected to IG streaming service");
        return;
    }
    
    account_id_ = account_id.empty() ? ig_service_.getAccountNumber() : account_id;
    
    createSession();
}

void IGStreamService::disconnect() {
    if (ls_client_) {
        ls_client_->disconnect();
        ls_client_.reset();
    }
}

void IGStreamService::createSession() {
    // Ensure we're logged in
    auto session_data = ig_service_.read_session();
    
    // Get lightstreamer endpoint
    lightstreamer_endpoint_ = session_data["lightstreamerEndpoint"];
    
    if (lightstreamer_endpoint_.empty()) {
        throw std::runtime_error("No Lightstreamer endpoint in session data");
    }
    
    try {
        spdlog::info("Requesting session tokens for Lightstreamer using v3 API...");
        
        // Utiliser fetchSessionTokens=true pour obtenir les tokens nécessaires
        auto tokens_data = ig_service_.read_session(true);
        
        // Vérifier que nous avons reçu les tokens
        if (!tokens_data.contains("cst") || !tokens_data.contains("securityToken")) {
            spdlog::error("Session tokens not found in response");
            spdlog::debug("Available keys in response: ");
            for (auto& [key, value] : tokens_data.items()) {
                spdlog::debug("  {}: {}", key, value.dump());
            }
            throw std::runtime_error("Session tokens not found in response");
        }
        
        std::string cst = tokens_data["cst"];
        std::string xst = tokens_data["securityToken"];
        
        spdlog::info("Session tokens obtained successfully from v3 API");
        spdlog::debug("CST: {}...", cst.substr(0, 10));
        spdlog::debug("XST: {}...", xst.substr(0, 10));
        
        // Construire le mot de passe Lightstreamer
        std::string password = "CST-" + cst + "|XST-" + xst;
        
        spdlog::info("Connecting to Lightstreamer at {}", lightstreamer_endpoint_);
        
        // Create and connect client
        ls_client_ = std::make_shared<LightstreamerClient>(lightstreamer_endpoint_);
        ls_client_->setCredentials(account_id_, password);
        ls_client_->connect();
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to get session tokens: {}", e.what());
        throw;
    }
}

std::shared_ptr<TickerSubscription> IGStreamService::subscribeToTicks(
    const std::string& epic, 
    std::shared_ptr<TickerListener> listener) {
    
    if (!ls_client_ || !ls_client_->isConnected()) {
        throw std::runtime_error("Not connected to Lightstreamer");
    }
    
    // Create subscription
    auto subscription = std::make_shared<TickerSubscription>(epic);
    subscription->addListener(listener);
    
    // Subscribe
    ls_client_->subscribe(subscription);
    
    // Store subscription
    subscriptions_[epic] = subscription;
    
    // Create ticker
    {
        std::lock_guard<std::mutex> lock(tickers_mutex_);
        tickers_[epic] = std::make_shared<Ticker>(epic);
    }
    
    return subscription;
}

void IGStreamService::unsubscribeAll() {
    if (!ls_client_) {
        return;
    }
    
    // Unsubscribe from all subscriptions
    for (const auto& [epic, subscription] : subscriptions_) {
        ls_client_->unsubscribe(subscription);
    }
    
    subscriptions_.clear();
    
    // Clear tickers
    {
        std::lock_guard<std::mutex> lock(tickers_mutex_);
        tickers_.clear();
    }
}

bool IGStreamService::isConnected() const {
    return ls_client_ && ls_client_->isConnected();
}

Ticker IGStreamService::getTicker(const std::string& epic, int timeout_seconds) {
    // Check if we have a ticker for this epic
    std::shared_ptr<Ticker> ticker;
    {
        std::lock_guard<std::mutex> lock(tickers_mutex_);
        auto it = tickers_.find(epic);
        if (it != tickers_.end()) {
            ticker = it->second;
        }
    }
    
    if (!ticker) {
        throw std::runtime_error("No ticker found for epic: " + epic);
    }
    
    // Wait for ticker to be populated
    auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(timeout_seconds);
    
    while (std::chrono::system_clock::now() < timeout) {
        if (!ticker->timestamp.empty()) {
            // Ticker has been populated
            return *ticker;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    
    throw std::runtime_error("Timeout waiting for ticker data for epic: " + epic);
}

} // namespace ig