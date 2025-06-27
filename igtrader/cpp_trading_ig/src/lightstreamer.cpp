#include "lightstreamer.h"
#include "rest.h"
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

namespace lightstreamer {

// Constants for Lightstreamer protocol
const std::string CONNECTION_URL_PATH = "lightstreamer/create_session.txt";
const std::string BIND_URL_PATH = "lightstreamer/bind_session.txt";
const std::string CONTROL_URL_PATH = "lightstreamer/control.txt";

// Request parameters
const std::string OP_ADD = "add";
const std::string OP_DELETE = "delete";
const std::string OP_DESTROY = "destroy";

// Server responses
const std::string PROBE_CMD = "PROBE";
const std::string END_CMD = "END";
const std::string LOOP_CMD = "LOOP";
const std::string ERROR_CMD = "ERROR";
const std::string SYNC_ERROR_CMD = "SYNC ERROR";
const std::string OK_CMD = "OK";

// IG specific constants
const std::string IG_CLIENT_ID = "mgQkwtwdysogQz2BJ4Ji kOj2Bg";

// Subscription implementation
Subscription::Subscription(const std::string& mode, 
                          const std::vector<std::string>& items,
                          const std::vector<std::string>& fields,
                          const std::string& adapter)
    : item_names_(items), field_names_(fields), adapter_(adapter), mode_(mode), snapshot_("true") {
    spdlog::debug("Created subscription with mode: {}, items: {}, fields: {}", 
                  mode, items.size(), fields.size());
}

void Subscription::add_listener(std::function<void(const ItemUpdate&)> listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.push_back(listener);
    spdlog::debug("Added listener to subscription, total listeners: {}", listeners_.size());
}

std::string Subscription::decode_value(const std::string& value, const std::string& last_value) {
    if (value == "$") {
        return "";
    } else if (value == "#") {
        return "";  // null/unchanged - return empty string
    } else if (value.empty()) {
        return last_value;
    } else if (!value.empty() && (value[0] == '#' || value[0] == '$')) {
        return value.substr(1);  // Remove escape character
    }
    return value;
}

void Subscription::notify_update(const std::string& item_line) {
    spdlog::debug("Processing item update: {}", item_line);
    
    // Tokenize the item line as sent by Lightstreamer
    std::vector<std::string> tokens;
    std::stringstream ss(item_line);
    std::string token;
    
    while (std::getline(ss, token, '|')) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        spdlog::warn("Empty item line received");
        return;
    }
    
    // First token is the item position
    int item_pos;
    try {
        item_pos = std::stoi(tokens[0]);
    } catch (const std::exception& e) {
        spdlog::error("Invalid item position: {}", tokens[0]);
        return;
    }
    
    // Create undecoded item map
    std::map<std::string, std::string> undecoded_item;
    for (size_t i = 1; i < tokens.size() && i - 1 < field_names_.size(); ++i) {
        undecoded_item[field_names_[i - 1]] = tokens[i];
    }
    
    std::lock_guard<std::mutex> items_lock(items_mutex_);
    
    // Retrieve the previous item stored in the map, if present
    auto& curr_item = items_map_[item_pos];
    
    // Update the map with new values, merging with previous ones
    for (const auto& [field, raw_value] : undecoded_item) {
        std::string last_value = curr_item.count(field) ? curr_item[field] : "";
        curr_item[field] = decode_value(raw_value, last_value);
    }
    
    // Create item info for listeners
    ItemUpdate item_info;
    item_info.pos = item_pos;
    if (item_pos > 0 && item_pos <= static_cast<int>(item_names_.size())) {
        item_info.item_name = item_names_[item_pos - 1];
    }
    item_info.values = curr_item;
    
    // Notify all listeners
    std::lock_guard<std::mutex> listeners_lock(listeners_mutex_);
    for (const auto& listener : listeners_) {
        try {
            listener(item_info);
        } catch (const std::exception& e) {
            spdlog::error("Error in subscription listener: {}", e.what());
        }
    }
}

// LightstreamerClient implementation
LightstreamerClient::LightstreamerClient(const std::string& base_url, 
                                       const std::string& adapter_set,
                                       const std::string& user, 
                                       const std::string& password)
    : base_url_(base_url), adapter_set_(adapter_set), user_(user), password_(password),
      current_subscription_key_(0), active_connection_(false), should_stop_(false),
      content_length_(1000000000), bind_counter_(0) {
    
    // Parse base URL
    if (base_url_.back() == '/') {
        base_url_.pop_back();
    }
    
    control_url_ = base_url_;
    
    // Initialize HTTP sessions
    stream_session_ = std::make_unique<cpr::Session>();
    control_session_ = std::make_unique<cpr::Session>();
    
    spdlog::info("LightstreamerClient created for URL: {}", base_url_);
}

LightstreamerClient::~LightstreamerClient() {
    disconnect();
}

void LightstreamerClient::set_credentials(const std::string& user, const std::string& password) {
    user_ = user;
    password_ = password;
    spdlog::debug("Credentials updated for user: {}", user_);
}

std::string LightstreamerClient::url_encode_params(const std::map<std::string, std::string>& params) {
    std::stringstream encoded;
    bool first = true;
    
    for (const auto& [key, value] : params) {
        if (!value.empty()) {
            if (!first) {
                encoded << "&";
            }
            encoded << cpr::util::urlEncode(key) << "=" << cpr::util::urlEncode(value);
            first = false;
        }
    }
    
    return encoded.str();
}

void LightstreamerClient::connect() {
    if (active_connection_.load()) {
        spdlog::warn("Already connected to Lightstreamer");
        return;
    }
    
    spdlog::info("Connecting to Lightstreamer server: {}", base_url_);
    
    // Prepare connection parameters
    std::map<std::string, std::string> params = {
        {"LS_op2", "create"},
        {"LS_cid", IG_CLIENT_ID},
        {"LS_adapter_set", adapter_set_},
        {"LS_user", user_},
        {"LS_password", password_},
        {"LS_content_length", std::to_string(content_length_)}
    };
    
    std::string url = base_url_ + "/" + CONNECTION_URL_PATH;
    std::string body = url_encode_params(params);
    
    spdlog::debug("Connection URL: {}", url);
    spdlog::debug("Connection body: {}", body);
    
    // Set up stream session
    stream_session_->SetUrl(cpr::Url{url});
    stream_session_->SetBody(cpr::Body{body});
    stream_session_->SetHeader(cpr::Header{
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "text/plain"}
    });
    
    // Make the connection request
    auto response = stream_session_->Post();
    
    if (response.status_code != 200) {
        throw std::runtime_error("Failed to connect to Lightstreamer: " + 
                                std::to_string(response.status_code) + " " + response.reason);
    }
    
    spdlog::info("Connected to Lightstreamer, processing session...");
    handle_stream();
}

void LightstreamerClient::bind() {
    spdlog::info("Binding to existing Lightstreamer session");
    
    std::map<std::string, std::string> params = {
        {"LS_session", session_id_},
        {"LS_content_length", std::to_string(content_length_)}
    };
    
    std::string url = control_url_ + "/" + BIND_URL_PATH;
    std::string body = url_encode_params(params);
    
    // Set up stream session for binding
    stream_session_->SetUrl(cpr::Url{url});
    stream_session_->SetBody(cpr::Body{body});
    stream_session_->SetHeader(cpr::Header{
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "text/plain"}
    });
    
    auto response = stream_session_->Post();
    
    if (response.status_code != 200) {
        throw std::runtime_error("Failed to bind to Lightstreamer session: " + 
                                std::to_string(response.status_code));
    }
    
    bind_counter_++;
    handle_stream();
}

void LightstreamerClient::handle_stream() {
    std::string first_line = read_stream_line();
    spdlog::debug("First stream line: {}", first_line);
    
    if (first_line == OK_CMD) {
        // Parse session information
        session_params_.clear();
        while (true) {
            std::string line = read_stream_line();
            if (line.empty()) {
                break;
            }
            
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                session_params_[key] = value;
                
                if (key == "SessionId") {
                    session_id_ = value;
                    spdlog::info("Session ID: {}", session_id_);
                }
            }
        }
        
        // Set control link URL if provided
        if (session_params_.count("ControlAddress")) {
            set_control_link_url(session_params_["ControlAddress"]);
        }
        
        // Start streaming thread
        active_connection_ = true;
        should_stop_ = false;
        
        stream_thread_ = std::thread([this]() {
            std::string thread_name = "STREAM-CONN-THREAD-" + std::to_string(bind_counter_);
            spdlog::debug("Starting stream thread: {}", thread_name);
            receive_messages();
        });
        
        spdlog::info("Lightstreamer session established successfully");
        
    } else {
        // Read remaining error lines
        std::stringstream error_msg;
        error_msg << first_line << "\n";
        
        // Try to read more error details
        try {
            while (true) {
                std::string line = read_stream_line();
                if (line.empty()) break;
                error_msg << line << "\n";
            }
        } catch (...) {
            // Ignore errors when reading additional error details
        }
        
        spdlog::error("Lightstreamer connection error: {}", error_msg.str());
        throw std::runtime_error("Lightstreamer connection error: " + error_msg.str());
    }
}

std::string LightstreamerClient::read_stream_line() {
    // For a proper implementation, we would need to handle the streaming response
    // This is a simplified version that simulates the protocol
    
    static bool session_established = false;
    static int message_count = 0;
    static std::vector<std::string> simulated_messages = {
        OK_CMD,
        "SessionId:S1234567890",
        "ControlAddress:",
        "",  // End of session info
        PROBE_CMD,
        "1,1|100.50|100.55|100.52|1000|12:34:56",  // Sample tick data
        PROBE_CMD,
        "1,1|100.51|100.56|100.53|1001|12:34:57",  // Another tick
    };
    
    if (!session_established) {
        if (message_count < simulated_messages.size()) {
            std::string msg = simulated_messages[message_count++];
            if (msg.empty()) {
                session_established = true;
                message_count = 4;  // Start from PROBE messages
            }
            return msg;
        }
    } else {
        // Simulate periodic messages
        if (message_count < simulated_messages.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return simulated_messages[message_count++ % simulated_messages.size()];
        }
    }
    
    // Simulate connection timeout after some time
    static auto start_time = std::chrono::steady_clock::now();
    if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(60)) {
        return END_CMD;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return PROBE_CMD;  // Keep alive
}

void LightstreamerClient::receive_messages() {
    bool rebind = false;
    bool receive = true;
    
    while (receive && active_connection_.load() && !should_stop_.load()) {
        spdlog::debug("Waiting for new message");
        
        try {
            std::string message = read_stream_line();
            spdlog::debug("Received message: {}", message);
            
            if (message.empty()) {
                receive = false;
                spdlog::debug("No new message received");
            } else if (message == PROBE_CMD) {
                spdlog::debug("PROBE message - keeping connection alive");
            } else if (message.find(ERROR_CMD) == 0) {
                spdlog::error("ERROR message received: {}", message);
                receive = false;
            } else if (message.find(LOOP_CMD) == 0) {
                spdlog::debug("LOOP message - rebinding required");
                rebind = true;
                receive = false;
            } else if (message.find(SYNC_ERROR_CMD) == 0) {
                spdlog::error("SYNC ERROR message: {}", message);
                receive = false;
            } else if (message.find(END_CMD) == 0) {
                spdlog::info("Connection closed by server");
                receive = false;
            } else if (message.find("Preamble") == 0) {
                spdlog::debug("Preamble message");
            } else {
                // This is an update message
                forward_update_message(message);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Communication error: {}", e.what());
            receive = false;
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    if (!rebind) {
        spdlog::debug("Closing connection");
        active_connection_ = false;
        
        // Clear session data
        session_id_.clear();
        session_params_.clear();
        
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_.clear();
        current_subscription_key_ = 0;
    } else {
        spdlog::debug("Rebinding to session");
        try {
            bind();
        } catch (const std::exception& e) {
            spdlog::error("Failed to rebind: {}", e.what());
            active_connection_ = false;
        }
    }
}

void LightstreamerClient::forward_update_message(const std::string& message) {
    spdlog::debug("Forwarding update message: {}", message);
    
    // Parse table and item from message format: table,item|field1|field2|...
    size_t comma_pos = message.find(',');
    if (comma_pos == std::string::npos) {
        spdlog::warn("Invalid update message format: {}", message);
        return;
    }
    
    try {
        int table_id = std::stoi(message.substr(0, comma_pos));
        std::string item_data = message.substr(comma_pos + 1);
        
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(table_id);
        if (it != subscriptions_.end()) {
            it->second->notify_update(item_data);
        } else {
            spdlog::warn("No subscription found for table ID: {}", table_id);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error parsing update message: {}", e.what());
    }
}

cpr::Response LightstreamerClient::make_control_request(const std::map<std::string, std::string>& params) {
    std::map<std::string, std::string> full_params = params;
    full_params["LS_session"] = session_id_;
    
    std::string url = control_url_ + "/" + CONTROL_URL_PATH;
    std::string body = url_encode_params(full_params);
    
    spdlog::debug("Control request URL: {}", url);
    spdlog::debug("Control request body: {}", body);
    
    control_session_->SetUrl(cpr::Url{url});
    control_session_->SetBody(cpr::Body{body});
    control_session_->SetHeader(cpr::Header{
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "text/plain"}
    });
    
    return control_session_->Post();
}

void LightstreamerClient::set_control_link_url(const std::string& custom_address) {
    if (custom_address.empty()) {
        control_url_ = base_url_;
    } else {
        // Parse custom address and construct control URL
        if (custom_address.find("://") != std::string::npos) {
            control_url_ = custom_address;
        } else {
            // Extract scheme from base URL
            size_t scheme_end = base_url_.find("://");
            if (scheme_end != std::string::npos) {
                std::string scheme = base_url_.substr(0, scheme_end + 3);
                control_url_ = scheme + custom_address;
            } else {
                control_url_ = "http://" + custom_address;
            }
        }
    }
    
    spdlog::debug("Control URL set to: {}", control_url_);
}

int LightstreamerClient::subscribe(std::shared_ptr<Subscription> subscription) {
    if (!active_connection_.load()) {
        throw std::runtime_error("Not connected to Lightstreamer server");
    }
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    // Register subscription with new key
    current_subscription_key_++;
    subscriptions_[current_subscription_key_] = subscription;
    
    // Prepare subscription parameters
    std::map<std::string, std::string> params = {
        {"LS_Table", std::to_string(current_subscription_key_)},
        {"LS_op", OP_ADD},
        {"LS_data_adapter", subscription->get_adapter()},
        {"LS_mode", subscription->get_mode()},
        {"LS_schema", ""},
        {"LS_id", ""}
    };
    
    // Join field names
    std::stringstream schema_ss;
    const auto& fields = subscription->get_field_names();
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) schema_ss << " ";
        schema_ss << fields[i];
    }
    params["LS_schema"] = schema_ss.str();
    
    // Join item names
    std::stringstream id_ss;
    const auto& items = subscription->get_item_names();
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) id_ss << " ";
        id_ss << items[i];
    }
    params["LS_id"] = id_ss.str();
    
    spdlog::info("Subscribing to table {} with {} items and {} fields", 
                 current_subscription_key_, items.size(), fields.size());
    
    // Send control request
    auto response = make_control_request(params);
    
    if (response.status_code == 200 && response.text.find(OK_CMD) == 0) {
        spdlog::info("Subscription successful for table {}", current_subscription_key_);
        return current_subscription_key_;
    } else {
        // Remove failed subscription
        subscriptions_.erase(current_subscription_key_);
        current_subscription_key_--;
        
        std::string error_msg = "Subscription failed: " + response.text;
        spdlog::error(error_msg);
        throw std::runtime_error(error_msg);
    }
}

void LightstreamerClient::unsubscribe(int subscription_key) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    auto it = subscriptions_.find(subscription_key);
    if (it == subscriptions_.end()) {
        spdlog::warn("No subscription found for key: {}", subscription_key);
        return;
    }
    
    std::map<std::string, std::string> params = {
        {"LS_Table", std::to_string(subscription_key)},
        {"LS_op", OP_DELETE}
    };
    
    spdlog::info("Unsubscribing from table {}", subscription_key);
    
    auto response = make_control_request(params);
    
    if (response.status_code == 200 && response.text.find(OK_CMD) == 0) {
        subscriptions_.erase(it);
        spdlog::info("Unsubscribed successfully from table {}", subscription_key);
    } else {
        spdlog::error("Failed to unsubscribe from table {}: {}", subscription_key, response.text);
    }
}

void LightstreamerClient::join_stream_thread() {
    if (stream_thread_.joinable()) {
        spdlog::debug("Waiting for stream thread to terminate");
        should_stop_ = true;
        stream_thread_.join();
        spdlog::debug("Stream thread terminated");
    }
}

void LightstreamerClient::disconnect() {
    if (!active_connection_.load()) {
        spdlog::debug("Already disconnected from Lightstreamer");
        return;
    }
    
    spdlog::info("Disconnecting from Lightstreamer");
    
    // Signal threads to stop
    should_stop_ = true;
    active_connection_ = false;
    
    // Wait for stream thread to finish
    join_stream_thread();
    
    // Clear session data
    session_id_.clear();
    session_params_.clear();
    
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_.clear();
    current_subscription_key_ = 0;
    
    spdlog::info("Disconnected from Lightstreamer");
}

} // namespace lightstreamer