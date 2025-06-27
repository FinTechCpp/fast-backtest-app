#include "stream.h"
#include "rest.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <thread>

namespace ig {

// IGStreamService implementation
IGStreamService::IGStreamService(std::shared_ptr<IGService> ig_service)
    : ig_service_(ig_service) {
    
    if (!ig_service_) {
        throw std::invalid_argument("IGService cannot be null");
    }
    
    acc_number_ = ig_service_->getAccountNumber();
    spdlog::info("IGStreamService created for account: {}", acc_number_);
}

IGStreamService::~IGStreamService() {
    disconnect();
}

void IGStreamService::create_session(bool encryption, const std::string& version) {
    spdlog::info("Creating streaming session with encryption: {}, version: {}", encryption, version);
    
    // Use existing IG session and get session tokens
    try {
        // Get session tokens from existing session
        auto session_data = ig_service_->read_session(true);  // fetch_session_tokens = true
        
        // Extract lightstreamer endpoint from session data
        if (session_data.contains("lightstreamerEndpoint")) {
            lightstreamer_endpoint_ = session_data["lightstreamerEndpoint"];
        } else {
            // If not in session data, try to get it from a fresh session read
            spdlog::warn("No lightstreamerEndpoint in session data, trying to get it from headers");
            // For demo, use the known endpoint
            lightstreamer_endpoint_ = "https://demo-apd.marketdatasystems.com";
        }
        
        spdlog::info("Lightstreamer endpoint: {}", lightstreamer_endpoint_);
        
        // Get session tokens from session data (they are extracted from headers in read_session)
        std::string cst, xst;
        
        if (session_data.contains("cst")) {
            cst = session_data["cst"];
            spdlog::debug("Found CST token: {}", cst.substr(0, 10) + "...");
        }
        if (session_data.contains("securityToken")) {
            xst = session_data["securityToken"];
            spdlog::debug("Found X-SECURITY-TOKEN: {}", xst.substr(0, 10) + "...");
        }
        
        if (cst.empty() || xst.empty()) {
            spdlog::error("CST empty: {}, XST empty: {}", cst.empty(), xst.empty());
            throw std::runtime_error("Failed to obtain CST and X-SECURITY-TOKEN");
        }
        
        // Format password for Lightstreamer
        std::string ls_password = "CST-" + cst + "|XST-" + xst;
        
        spdlog::info("Starting Lightstreamer connection");
        
        // Create Lightstreamer client
        ls_client_ = std::make_shared<lightstreamer::LightstreamerClient>(
            lightstreamer_endpoint_, "", acc_number_, ls_password);
        
        // Connect to Lightstreamer
        ls_client_->connect();
        
        spdlog::info("Streaming session created successfully");
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create streaming session: {}", e.what());
        throw;
    }
}

void IGStreamService::subscribe(const lightstreamer::Subscription& subscription) {
    if (!ls_client_ || !ls_client_->is_connected()) {
        throw std::runtime_error("Not connected to Lightstreamer server");
    }
    
    // Create a new subscription with the same parameters
    auto sub_ptr = std::make_shared<lightstreamer::Subscription>(
        subscription.get_mode(),
        subscription.get_item_names(),
        subscription.get_field_names(),
        subscription.get_adapter()
    );
    
    try {
        int subscription_key = ls_client_->subscribe(sub_ptr);
        
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscription_keys_[&subscription] = subscription_key;
        
        spdlog::info("Subscribed to {} items with key {}", 
                     subscription.get_item_names().size(), subscription_key);
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to subscribe: {}", e.what());
        throw;
    }
}

void IGStreamService::unsubscribe(const lightstreamer::Subscription& subscription) {
    if (!ls_client_) {
        spdlog::warn("No Lightstreamer client available for unsubscribe");
        return;
    }
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscription_keys_.find(&subscription);
    if (it != subscription_keys_.end()) {
        try {
            ls_client_->unsubscribe(it->second);
            subscription_keys_.erase(it);
            spdlog::info("Unsubscribed from subscription key {}", it->second);
        } catch (const std::exception& e) {
            spdlog::error("Failed to unsubscribe: {}", e.what());
        }
    } else {
        spdlog::warn("Subscription not found for unsubscribe");
    }
}

void IGStreamService::unsubscribe_all() {
    if (!ls_client_) {
        spdlog::warn("No Lightstreamer client available for unsubscribe_all");
        return;
    }
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    for (const auto& [sub_ptr, key] : subscription_keys_) {
        try {
            ls_client_->unsubscribe(key);
            spdlog::debug("Unsubscribed from key {}", key);
        } catch (const std::exception& e) {
            spdlog::error("Failed to unsubscribe from key {}: {}", key, e.what());
        }
    }
    
    subscription_keys_.clear();
    spdlog::info("Unsubscribed from all subscriptions");
}

void IGStreamService::disconnect() {
    if (ls_client_) {
        unsubscribe_all();
        ls_client_->disconnect();
        ls_client_.reset();
        spdlog::info("Disconnected from streaming service");
    }
}

bool IGStreamService::is_connected() const {
    return ls_client_ && ls_client_->is_connected();
}

// StreamingManager implementation
StreamingManager::StreamingManager(std::shared_ptr<IGStreamService> service)
    : service_(service), consumer_running_(false) {
    
    if (!service_) {
        throw std::invalid_argument("IGStreamService cannot be null");
    }
    
    // Start consumer thread
    consumer_running_ = true;
    consumer_thread_ = std::thread(&StreamingManager::consumer_worker, this);
    
    spdlog::info("StreamingManager created and consumer thread started");
}

StreamingManager::~StreamingManager() {
    stop_all_subscriptions();
    
    // Stop consumer thread
    consumer_running_ = false;
    queue_cv_.notify_all();
    
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
    
    spdlog::info("StreamingManager destroyed");
}

void StreamingManager::start_tick_subscription(const std::string& epic) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    std::string subscription_key = "TICK_" + epic;
    if (subscriptions_.count(subscription_key)) {
        spdlog::warn("Tick subscription already exists for epic: {}", epic);
        return;
    }
    
    // Create tick subscription
    std::string item_name = "CHART:" + epic + ":TICK";
    std::vector<std::string> items = {item_name};
    std::vector<std::string> fields = {"UTM", "BID", "OFR", "LTP", "TTV", "TTM"};
    
    auto subscription = std::make_shared<lightstreamer::Subscription>(
        "MERGE", items, fields, "");
    
    // Add update listener
    subscription->add_listener([this](const lightstreamer::ItemUpdate& update) {
        this->on_item_update(update);
    });
    
    // Subscribe via service
    service_->subscribe(*subscription);
    
    // Store subscription
    subscriptions_[subscription_key] = subscription;
    
    spdlog::info("Started tick subscription for epic: {}", epic);
}

void StreamingManager::start_candle_subscription(const std::string& epic, const std::string& resolution) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    std::string subscription_key = "CANDLE_" + epic + "_" + resolution;
    if (subscriptions_.count(subscription_key)) {
        spdlog::warn("Candle subscription already exists for epic: {} resolution: {}", epic, resolution);
        return;
    }
    
    // Create candle subscription
    std::string item_name = "CHART:" + epic + ":" + resolution;
    std::vector<std::string> items = {item_name};
    std::vector<std::string> fields = {"UTM", "BID", "OFR", "LTP", "TTV", "TTM", "LTV"};
    
    auto subscription = std::make_shared<lightstreamer::Subscription>(
        "MERGE", items, fields, "");
    
    // Add update listener
    subscription->add_listener([this](const lightstreamer::ItemUpdate& update) {
        this->on_item_update(update);
    });
    
    // Subscribe via service
    service_->subscribe(*subscription);
    
    // Store subscription
    subscriptions_[subscription_key] = subscription;
    
    spdlog::info("Started candle subscription for epic: {} resolution: {}", epic, resolution);
}

TickerData StreamingManager::get_ticker(const std::string& epic, int timeout_seconds) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeout_seconds);
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            auto it = tickers_.find(epic);
            if (it != tickers_.end() && !it->second.last_update_time.empty()) {
                return it->second;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    spdlog::warn("Timeout waiting for ticker data for epic: {}", epic);
    return TickerData(epic);  // Return empty ticker data
}

CandleData StreamingManager::get_candle(const std::string& epic, int timeout_seconds) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeout_seconds);
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            auto it = candles_.find(epic);
            if (it != candles_.end() && !it->second.last_update_time.empty()) {
                return it->second;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    spdlog::warn("Timeout waiting for candle data for epic: {}", epic);
    return CandleData(epic);  // Return empty candle data
}

void StreamingManager::stop_all_subscriptions() {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    for (const auto& [key, subscription] : subscriptions_) {
        try {
            service_->unsubscribe(*subscription);
            spdlog::debug("Stopped subscription: {}", key);
        } catch (const std::exception& e) {
            spdlog::error("Failed to stop subscription {}: {}", key, e.what());
        }
    }
    
    subscriptions_.clear();
    
    // Clear data
    std::lock_guard<std::mutex> data_lock(data_mutex_);
    tickers_.clear();
    candles_.clear();
    
    spdlog::info("Stopped all subscriptions");
}

void StreamingManager::consumer_worker() {
    spdlog::debug("Consumer worker thread started");
    
    while (consumer_running_.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for updates or stop signal
        queue_cv_.wait(lock, [this] {
            return !update_queue_.empty() || !consumer_running_.load();
        });
        
        // Process all available updates
        while (!update_queue_.empty() && consumer_running_.load()) {
            auto update = update_queue_.front();
            update_queue_.pop();
            lock.unlock();
            
            try {
                process_update(update);
            } catch (const std::exception& e) {
                spdlog::error("Error processing update: {}", e.what());
            }
            
            lock.lock();
        }
    }
    
    spdlog::debug("Consumer worker thread stopped");
}

void StreamingManager::process_update(const lightstreamer::ItemUpdate& update) {
    std::string epic = extract_epic_from_item(update.item_name);
    if (epic.empty()) {
        spdlog::warn("Could not extract epic from item: {}", update.item_name);
        return;
    }
    
    // Determine if this is a tick or candle update
    if (update.item_name.find(":TICK") != std::string::npos) {
        parse_ticker_update(epic, update.values);
    } else {
        parse_candle_update(epic, update.values);
    }
}

void StreamingManager::on_item_update(const lightstreamer::ItemUpdate& update) {
    // Queue the update for processing by consumer thread
    std::lock_guard<std::mutex> lock(queue_mutex_);
    update_queue_.push(update);
    queue_cv_.notify_one();
}

void StreamingManager::parse_ticker_update(const std::string& epic, 
                                         const std::map<std::string, std::string>& values) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    TickerData& ticker = tickers_[epic];
    ticker.epic = epic;
    
    // Update fields if present
    auto it = values.find("UTM");
    if (it != values.end() && !it->second.empty()) {
        ticker.utm = it->second;
    }
    
    it = values.find("BID");
    if (it != values.end() && !it->second.empty()) {
        try {
            ticker.bid = std::stod(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse BID value: {}", it->second);
        }
    }
    
    it = values.find("OFR");
    if (it != values.end() && !it->second.empty()) {
        try {
            ticker.offer = std::stod(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse OFR value: {}", it->second);
        }
    }
    
    it = values.find("LTP");
    if (it != values.end() && !it->second.empty()) {
        try {
            ticker.last_traded_price = std::stod(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse LTP value: {}", it->second);
        }
    }
    
    it = values.find("TTV");
    if (it != values.end() && !it->second.empty()) {
        try {
            ticker.total_traded_volume = std::stoi(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse TTV value: {}", it->second);
        }
    }
    
    it = values.find("TTM");
    if (it != values.end() && !it->second.empty()) {
        ticker.last_update_time = it->second;
    }
    
    spdlog::debug("Updated ticker for {}: BID={}, OFR={}, LTP={}", 
                  epic, ticker.bid, ticker.offer, ticker.last_traded_price);
}

void StreamingManager::parse_candle_update(const std::string& epic, 
                                         const std::map<std::string, std::string>& values) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    CandleData& candle = candles_[epic];
    candle.epic = epic;
    
    // Update fields if present (similar to ticker but for candle data)
    auto it = values.find("UTM");
    if (it != values.end() && !it->second.empty()) {
        candle.utm = it->second;
    }
    
    it = values.find("BID");
    if (it != values.end() && !it->second.empty()) {
        try {
            candle.bid = std::stod(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse BID value: {}", it->second);
        }
    }
    
    it = values.find("OFR");
    if (it != values.end() && !it->second.empty()) {
        try {
            candle.offer = std::stod(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse OFR value: {}", it->second);
        }
    }
    
    it = values.find("LTP");
    if (it != values.end() && !it->second.empty()) {
        try {
            candle.last_traded_price = std::stod(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse LTP value: {}", it->second);
        }
    }
    
    it = values.find("TTV");
    if (it != values.end() && !it->second.empty()) {
        try {
            candle.total_traded_volume = std::stoi(it->second);
        } catch (const std::exception& e) {
            spdlog::debug("Failed to parse TTV value: {}", it->second);
        }
    }
    
    it = values.find("TTM");
    if (it != values.end() && !it->second.empty()) {
        candle.last_update_time = it->second;
    }
    
    it = values.find("LTV");
    if (it != values.end() && !it->second.empty()) {
        candle.consolidation_level = it->second;
    }
    
    spdlog::debug("Updated candle for {}: BID={}, OFR={}, LTP={}", 
                  epic, candle.bid, candle.offer, candle.last_traded_price);
}

std::string StreamingManager::extract_epic_from_item(const std::string& item_name) {
    // Expected format: "CHART:EPIC_NAME:RESOLUTION" or "CHART:EPIC_NAME:TICK"
    if (item_name.find("CHART:") != 0) {
        return "";
    }
    
    size_t first_colon = item_name.find(':', 6);  // Start after "CHART:"
    if (first_colon == std::string::npos) {
        return "";
    }
    
    size_t second_colon = item_name.find(':', first_colon + 1);
    if (second_colon == std::string::npos) {
        return "";
    }
    
    return item_name.substr(first_colon + 1, second_colon - first_colon - 1);
}

} // namespace ig