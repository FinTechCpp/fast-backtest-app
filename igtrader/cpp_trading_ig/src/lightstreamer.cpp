#include "lightstreamer.h"
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace ig {

// Constants for Lightstreamer protocol
const std::string CONNECTION_URL_PATH = "lightstreamer/create_session.txt";
const std::string BIND_URL_PATH = "lightstreamer/bind_session.txt";
const std::string CONTROL_URL_PATH = "lightstreamer/control.txt";
const std::string OP_ADD = "add";
const std::string OP_DELETE = "delete";
const std::string OP_DESTROY = "destroy";
const std::string PROBE_CMD = "PROBE";
const std::string END_CMD = "END";
const std::string LOOP_CMD = "LOOP";
const std::string ERROR_CMD = "ERROR";
const std::string SYNC_ERROR_CMD = "SYNC ERROR";
const std::string OK_CMD = "OK";

// Utility function to encode parameters
std::string encodeParams(const std::unordered_map<std::string, std::string>& params) {
    std::string result;
    bool first = true;
    
    for (const auto& [key, value] : params) {
        if (!value.empty()) {
            if (!first) {
                result += "&";
            }
            result += cpr::util::urlEncode(key) + "=" + cpr::util::urlEncode(value);
            first = false;
        }
    }
    
    return result;
}

// Implementation of Subscription
Subscription::Subscription(const std::string& mode, 
                         const std::vector<std::string>& items, 
                         const std::vector<std::string>& fields,
                         const std::string& adapter)
    : mode_(mode), items_(items), fields_(fields), adapter_(adapter) {
}

void Subscription::addListener(std::shared_ptr<SubscriptionListener> listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.push_back(listener);
}

void Subscription::removeListener(std::shared_ptr<SubscriptionListener> listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener),
        listeners_.end()
    );
}

std::string Subscription::decodeValue(const std::string& value, const std::string& last) {
    if (value == "$") {
        return "";
    } else if (value == "#") {
        return "";
    } else if (value.empty()) {
        return last;
    } else if (value[0] == '$' || value[0] == '#') {
        return value.substr(1);
    }
    return value;
}

void Subscription::notifyUpdate(int item_position, const std::vector<std::string>& values) {
    if (item_position <= 0 || item_position > static_cast<int>(items_.size()) || 
        values.size() != fields_.size()) {
        spdlog::error("Invalid update: position {} out of range or field count mismatch", item_position);
        return;
    }
    
    // Get item name
    const std::string& item_name = items_[item_position - 1];
    
    // Get or create cache for this item
    auto& item_cache = items_cache_[item_position];
    
    // Create values map
    std::unordered_map<std::string, std::string> updated_values;
    
    // Process each field
    for (size_t i = 0; i < fields_.size(); ++i) {
        const std::string& field_name = fields_[i];
        const std::string& raw_value = values[i];
        
        // Get previous value from cache
        std::string prev_value;
        auto it = item_cache.find(field_name);
        if (it != item_cache.end()) {
            prev_value = it->second;
        }
        
        // Decode value
        std::string decoded = decodeValue(raw_value, prev_value);
        
        // Update cache and result map
        item_cache[field_name] = decoded;
        updated_values[field_name] = decoded;
    }
    
    // Notify listeners
    std::vector<std::shared_ptr<SubscriptionListener>> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listeners_mutex_);
        listeners_copy = listeners_;
    }
    
    for (auto& listener : listeners_copy) {
        try {
            listener->onItemUpdate(item_name, updated_values);
        } catch (const std::exception& e) {
            spdlog::error("Exception in listener callback: {}", e.what());
        }
    }
}

// Implementation of LightstreamerClient
LightstreamerClient::LightstreamerClient(const std::string& server_url, const std::string& adapter_set)
    : server_url_(server_url), adapter_set_(adapter_set), connected_(false), 
      stop_requested_(false), subscription_counter_(0) {
}

LightstreamerClient::~LightstreamerClient() {
    disconnect();
}

void LightstreamerClient::setCredentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}

void LightstreamerClient::addListener(std::shared_ptr<ConnectionListener> listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.push_back(listener);
}

void LightstreamerClient::removeListener(std::shared_ptr<ConnectionListener> listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener),
        listeners_.end()
    );
}

void LightstreamerClient::connect() {
    if (connected_) {
        spdlog::info("Already connected to Lightstreamer");
        return;
    }
    
    spdlog::info("Connecting to Lightstreamer at {}", server_url_);
    
    stop_requested_ = false;
    try {
        createSession();
        
        // Attend que la connexion soit établie
        auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(10);
        spdlog::info("Waiting for Lightstreamer connection to be established...");
        
        // Démarrer le thread de streaming
        stream_thread_ = std::make_unique<std::thread>(&LightstreamerClient::streamThread, this);
        
        // Attendre que la connexion soit établie ou que le timeout soit atteint
        while (!connected_ && std::chrono::system_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (!connected_) {
            spdlog::error("Timeout waiting for Lightstreamer connection");
            throw std::runtime_error("Timeout connecting to Lightstreamer");
        }
        
        spdlog::info("Lightstreamer connection established successfully");
        notifyStatusChange("CONNECTED");
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to Lightstreamer: {}", e.what());
        connected_ = false;
        notifyStatusChange("DISCONNECTED");
        throw;
    }
}

void LightstreamerClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    spdlog::info("Disconnecting from Lightstreamer");
    
    // Signal thread to stop
    stop_requested_ = true;
    
    // Send destroy command to server
    try {
        sendControlRequest({{"LS_op", OP_DESTROY}});
    } catch (const std::exception& e) {
        spdlog::error("Error sending destroy command: {}", e.what());
    }
    
    // Wait for thread to finish
    if (stream_thread_ && stream_thread_->joinable()) {
        stream_thread_->join();
    }
    
    // Clear all subscriptions
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_.clear();
    }
    
    // Reset session data
    session_id_.clear();
    control_address_.clear();
    connected_ = false;
    
    notifyStatusChange("DISCONNECTED");
}

void LightstreamerClient::subscribe(std::shared_ptr<Subscription> subscription) {
    if (!connected_) {
        spdlog::error("Cannot subscribe, not connected to Lightstreamer");
        return;
    }
    
    // Add subscription to map
    int sub_id;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        sub_id = ++subscription_counter_;
        subscriptions_[sub_id] = subscription;
    }
    
    // Prepare parameters
    std::ostringstream schema_str, items_str;
    
    // Join fields with spaces
    for (size_t i = 0; i < subscription->getFields().size(); ++i) {
        if (i > 0) schema_str << " ";
        schema_str << subscription->getFields()[i];
    }
    
    // Join items with spaces
    for (size_t i = 0; i < subscription->getItems().size(); ++i) {
        if (i > 0) items_str << " ";
        items_str << subscription->getItems()[i];
    }
    
    // Send subscription request
    std::unordered_map<std::string, std::string> params = {
        {"LS_Table", std::to_string(sub_id)},
        {"LS_op", OP_ADD},
        {"LS_data_adapter", subscription->getAdapter()},
        {"LS_mode", subscription->getMode()},
        {"LS_schema", schema_str.str()},
        {"LS_id", items_str.str()}
    };
    
    try {
        std::string response = sendControlRequest(params);
        spdlog::info("Subscription response: {}", response);
        
        if (response == OK_CMD) {
            // Notify listeners that subscription was successful
            std::vector<std::shared_ptr<SubscriptionListener>> listeners;
            {
                std::lock_guard<std::mutex> lock(subscription->getListenersMutex());
                listeners = subscription->getListeners();
            }
            
            for (auto& listener : listeners) {
                try {
                    listener->onSubscription();
                } catch (const std::exception& e) {
                    spdlog::error("Exception in subscription listener: {}", e.what());
                }
            }
        } else {
            // Parse error message
            std::string message = response;
            int code = -1;
            
            size_t pos = response.find(' ');
            if (pos != std::string::npos) {
                try {
                    code = std::stoi(response.substr(0, pos));
                    message = response.substr(pos + 1);
                } catch (...) {
                    // Ignore parsing errors
                }
            }
            
            // Notify listeners of error
            std::vector<std::shared_ptr<SubscriptionListener>> listeners;
            {
                std::lock_guard<std::mutex> lock(subscription->getListenersMutex());
                listeners = subscription->getListeners();
            }
            
            for (auto& listener : listeners) {
                try {
                    listener->onSubscriptionError(code, message);
                } catch (const std::exception& e) {
                    spdlog::error("Exception in subscription error listener: {}", e.what());
                }
            }
            
            // Remove subscription
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscriptions_.erase(sub_id);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error subscribing: {}", e.what());
        
        // Remove subscription
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_.erase(sub_id);
    }
}

void LightstreamerClient::unsubscribe(std::shared_ptr<Subscription> subscription) {
    if (!connected_) {
        return;
    }
    
    // Find subscription ID
    int sub_id = -1;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        for (const auto& [id, sub] : subscriptions_) {
            if (sub == subscription) {
                sub_id = id;
                break;
            }
        }
    }
    
    if (sub_id == -1) {
        spdlog::error("Subscription not found");
        return;
    }
    
    // Send unsubscribe request
    std::unordered_map<std::string, std::string> params = {
        {"LS_Table", std::to_string(sub_id)},
        {"LS_op", OP_DELETE}
    };
    
    try {
        std::string response = sendControlRequest(params);
        spdlog::info("Unsubscription response: {}", response);
        
        if (response == OK_CMD) {
            // Notify listeners that unsubscription was successful
            std::vector<std::shared_ptr<SubscriptionListener>> listeners;
            {
                std::lock_guard<std::mutex> lock(subscription->getListenersMutex());
                listeners = subscription->getListeners();
            }
            
            for (auto& listener : listeners) {
                try {
                    listener->onUnsubscription();
                } catch (const std::exception& e) {
                    spdlog::error("Exception in unsubscription listener: {}", e.what());
                }
            }
            
            // Remove subscription
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscriptions_.erase(sub_id);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error unsubscribing: {}", e.what());
    }
}

std::vector<std::shared_ptr<Subscription>> LightstreamerClient::getSubscriptions() const {
    std::vector<std::shared_ptr<Subscription>> result;
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    for (const auto& [_, sub] : subscriptions_) {
        result.push_back(sub);
    }
    
    return result;
}

bool LightstreamerClient::isConnected() const {
    return connected_;
}

std::string LightstreamerClient::sendControlRequest(const std::unordered_map<std::string, std::string>& params) {
    std::unordered_map<std::string, std::string> full_params = params;
    full_params["LS_session"] = session_id_;
    
    std::string url = server_url_;
    if (!control_address_.empty()) {
        // Parse and rebuild URL with control address
        cpr::Url parsed_url = cpr::Url{server_url_};
        std::string scheme = parsed_url.str().substr(0, parsed_url.str().find("://") + 3);
        url = scheme + control_address_;
    }
    
    url += "/" + CONTROL_URL_PATH;
    
    return httpRequest(url, full_params);
}

void LightstreamerClient::createSession() {
    // Prepare parameters
    std::unordered_map<std::string, std::string> params = {
        {"LS_op2", "create"},
        {"LS_cid", "mgQkwtwdysogQz2BJ4Ji kOj2Bg"},
        {"LS_adapter_set", adapter_set_},
        {"LS_user", username_},
        {"LS_password", password_},
        {"LS_content_length", "1000000000"}
    };
    
    // Send request
    std::string url = server_url_ + "/" + CONNECTION_URL_PATH;
    std::string response = httpRequest(url, params);
    
    // Process response
    if (response != OK_CMD) {
        throw std::runtime_error("Failed to create session: " + response);
    }
    
    // Get session parameters from response
    std::istringstream iss(response);
    std::string line;
    
    // Skip OK line
    std::getline(iss, line);
    
    // Parse session parameters
    while (std::getline(iss, line)) {
        if (line.empty()) {
            break;
        }
        
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "SessionId") {
                session_id_ = value;
            } else if (key == "ControlAddress") {
                control_address_ = value;
            }
        }
    }
    
    if (session_id_.empty()) {
        throw std::runtime_error("No session ID in response");
    }
    
    spdlog::info("Created Lightstreamer session: {}", session_id_);
}

void LightstreamerClient::bindSession() {
    // Prepare parameters
    std::unordered_map<std::string, std::string> params = {
        {"LS_session", session_id_},
        {"LS_content_length", "1000000000"}
    };
    
    // Build URL
    std::string url = server_url_;
    if (!control_address_.empty()) {
        // Parse and rebuild URL with control address
        cpr::Url parsed_url = cpr::Url{server_url_};
        std::string scheme = parsed_url.str().substr(0, parsed_url.str().find("://") + 3);
        url = scheme + control_address_;
    }
    
    url += "/" + BIND_URL_PATH;
    
    // Send request in a new thread
    spdlog::info("Binding to session {}", session_id_);
    
    // The actual binding is done in streamThread
}

void LightstreamerClient::streamThread() {
    spdlog::info("Stream thread started");
    
    // Notify listeners
    for (auto& listener : listeners_) {
        try {
            listener->onListenStart();
        } catch (const std::exception& e) {
            spdlog::error("Exception in listener: {}", e.what());
        }
    }
    
    bool rebind = false;
    
    while (!stop_requested_) {
        try {
            // Prepare parameters
            std::unordered_map<std::string, std::string> params;
            std::string url;
            
            if (!rebind) {
                // First connection or after error
                params = {
                    {"LS_op2", "create"},
                    {"LS_cid", "mgQkwtwdysogQz2BJ4Ji kOj2Bg"},
                    {"LS_adapter_set", adapter_set_},
                    {"LS_user", username_},
                    {"LS_password", password_},
                    {"LS_content_length", "1000000000"}
                };
                
                url = server_url_ + "/" + CONNECTION_URL_PATH;
            } else {
                // Rebind to existing session
                params = {
                    {"LS_session", session_id_},
                    {"LS_content_length", "1000000000"}
                };
                
                url = server_url_;
                if (!control_address_.empty()) {
                    // Parse and rebuild URL with control address
                    cpr::Url parsed_url = cpr::Url{server_url_};
                    std::string scheme = parsed_url.str().substr(0, parsed_url.str().find("://") + 3);
                    url = scheme + control_address_;
                }
                
                url += "/" + BIND_URL_PATH;
            }
            
            // Send request
            cpr::Session session = cpr::Session();
            session.SetUrl(cpr::Url{url});
            
            // Set POST data
            std::string post_data = encodeParams(params);
            session.SetBody(cpr::Body{post_data});
            session.SetHeader(cpr::Header{{"Content-Type", "application/x-www-form-urlencoded"}});
            
            // Send request
            cpr::Response r = session.Post();
            
            if (r.status_code != 200) {
                throw std::runtime_error("HTTP error: " + std::to_string(r.status_code));
            }
            
            // Process response
            std::istringstream iss(r.text);
            std::string line;
            
            // Read status line
            std::getline(iss, line);
            
            if (line != OK_CMD) {
                throw std::runtime_error("Server response error: " + line);
            }
            
            // If not rebinding, parse session parameters
            if (!rebind) {
                // Parse session parameters
                while (std::getline(iss, line)) {
                    if (line.empty()) {
                        break;
                    }
                    
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string key = line.substr(0, pos);
                        std::string value = line.substr(pos + 1);
                        
                        if (key == "SessionId") {
                            session_id_ = value;
                        } else if (key == "ControlAddress") {
                            control_address_ = value;
                        }
                    }
                }
                
                if (session_id_.empty()) {
                    throw std::runtime_error("No session ID in response");
                }
                
                spdlog::info("Created Lightstreamer session: {}", session_id_);
            }
            
            // Process stream data
            rebind = false;
            
            while (!stop_requested_) {
                if (!std::getline(iss, line)) {
                    // End of stream
                    rebind = true;
                    break;
                }
                
                if (line.empty()) {
                    continue;
                }
                
                processMessage(line);
            }
            
            if (rebind && !stop_requested_) {
                spdlog::info("Rebinding to session {}", session_id_);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } catch (const std::exception& e) {
            spdlog::error("Stream error: {}", e.what());
            
            if (!stop_requested_) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                rebind = false;  // Try creating a new session
            }
        }
    }
    
    // Notify listeners
    for (auto& listener : listeners_) {
        try {
            listener->onListenEnd();
        } catch (const std::exception& e) {
            spdlog::error("Exception in listener: {}", e.what());
        }
    }
    
    spdlog::info("Stream thread ended");
}

void LightstreamerClient::processMessage(const std::string& message) {
    spdlog::info("Processing message: {}", message);
    
    if (message == PROBE_CMD) {
        // Heartbeat, ignore
        return;
    }
    
    if (message.compare(0, ERROR_CMD.length(), ERROR_CMD) == 0) {
        spdlog::error("Server error: {}", message);
        return;
    }
    
    if (message == LOOP_CMD) {
        spdlog::info("Server requested rebind");
        throw std::runtime_error("LOOP");
    }
    
    if (message.compare(0, SYNC_ERROR_CMD.length(), SYNC_ERROR_CMD) == 0) {
        spdlog::error("Sync error: {}", message);
        throw std::runtime_error("SYNC ERROR");
    }
    
    if (message.compare(0, END_CMD.length(), END_CMD) == 0) {
        spdlog::error("Session ended by server: {}", message);
        throw std::runtime_error("END");
    }
    
    // Regular update - format is: <table>,<item position>|<field1>|<field2>|...
    size_t comma_pos = message.find(',');
    if (comma_pos != std::string::npos) {
        try {
            int table = std::stoi(message.substr(0, comma_pos));
            
            size_t item_end = message.find('|', comma_pos + 1);
            if (item_end == std::string::npos) {
                return;
            }
            
            int item_position = std::stoi(message.substr(comma_pos + 1, item_end - comma_pos - 1));
            
            // Parse fields
            std::vector<std::string> fields;
            size_t start = item_end + 1;
            size_t end;
            
            while ((end = message.find('|', start)) != std::string::npos) {
                fields.push_back(message.substr(start, end - start));
                start = end + 1;
            }
            
            // Add last field
            fields.push_back(message.substr(start));
            
            // Find subscription
            std::shared_ptr<Subscription> subscription;
            {
                std::lock_guard<std::mutex> lock(subscriptions_mutex_);
                auto it = subscriptions_.find(table);
                if (it != subscriptions_.end()) {
                    subscription = it->second;
                    spdlog::info("Found subscription for table {}: {}", table, it->second->getItems()[0]);
                } else {
                    spdlog::warn("No subscription found for table {}", table);
                }
            }
            
            if (subscription) {
                spdlog::info("Notifying update for item {} with {} fields", item_position, fields.size());
                subscription->notifyUpdate(item_position, fields);
            }
        } catch (const std::exception& e) {
            spdlog::error("Error processing update: {}", e.what());
        }
    }
}

void LightstreamerClient::notifyStatusChange(const std::string& status) {
    std::vector<std::shared_ptr<ConnectionListener>> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listeners_mutex_);
        listeners_copy = listeners_;
    }
    
    for (auto& listener : listeners_copy) {
        try {
            listener->onStatusChange(status);
        } catch (const std::exception& e) {
            spdlog::error("Exception in status listener: {}", e.what());
        }
    }
}

std::string LightstreamerClient::httpRequest(const std::string& url, const std::unordered_map<std::string, std::string>& params) {
    cpr::Session session = cpr::Session();
    session.SetUrl(cpr::Url{url});
    
    // Set POST data
    std::string post_data = encodeParams(params);
    session.SetBody(cpr::Body{post_data});
    session.SetHeader(cpr::Header{{"Content-Type", "application/x-www-form-urlencoded"}});
    
    // Send request
    cpr::Response r = session.Post();
    
    if (r.status_code != 200) {
        throw std::runtime_error("HTTP error: " + std::to_string(r.status_code));
    }
    
    return r.text;
}

} // namespace ig

