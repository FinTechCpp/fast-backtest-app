#include "../include/rest.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <queue>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace ig {

// Initialize static members
const std::unordered_map<std::string, std::string> IGService::D_BASE_URL = {
    {"live", "https://api.ig.com/gateway/deal"},
    {"demo", "https://demo-api.ig.com/gateway/deal"}
};

// Helper functions for date formatting
std::string format_datetime(const std::chrono::system_clock::time_point& time_point, const std::string& format = "%Y-%m-%dT%H:%M:%S") {
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), format.c_str());
    return ss.str();
}

std::string format_date(const std::chrono::system_clock::time_point& time_point, const std::string& format = "%d-%m-%Y") {
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), format.c_str());
    return ss.str();
}

// Helper function to convert milliseconds to time duration
int64_t conv_to_ms(const std::chrono::milliseconds& duration) {
    return duration.count();
}

// Helper functions to check response content
bool api_limit_hit(const std::string& response_text) {
    return response_text.find("exceeded-api-key-allowance") != std::string::npos ||
           response_text.find("exceeded-account-allowance") != std::string::npos ||
           response_text.find("exceeded-account-trading-allowance") != std::string::npos;
}

bool token_invalid(const std::string& response_text) {
    return response_text.find("oauth-token-invalid") != std::string::npos;
}

// Convert user-friendly resolution format to IG API format
std::string conv_resol(const std::string& resolution) {
    static const std::unordered_map<std::string, std::string> resolution_map = {
        {"1s", "SECOND"},
        {"1Min", "MINUTE"},
        {"2Min", "MINUTE_2"},
        {"3Min", "MINUTE_3"},
        {"5Min", "MINUTE_5"},
        {"10Min", "MINUTE_10"},
        {"15Min", "MINUTE_15"},
        {"30Min", "MINUTE_30"},
        {"1h", "HOUR"},
        {"1H", "HOUR"},
        {"2h", "HOUR_2"},
        {"2H", "HOUR_2"},
        {"3h", "HOUR_3"},
        {"3H", "HOUR_3"},
        {"4h", "HOUR_4"},
        {"4H", "HOUR_4"},
        {"D", "DAY"},
        {"W", "WEEK"},
        {"M", "MONTH"}
    };
    
    auto it = resolution_map.find(resolution);
    if (it != resolution_map.end()) {
        return it->second;
    } 
    
    // Try some common alternatives
    if (resolution == "15M") return "MINUTE_15";
    if (resolution == "30M") return "MINUTE_30";
    if (resolution == "1d" || resolution == "1D") return "DAY";
    
    spdlog::warn("Resolution '{}' not recognized, using as-is", resolution);
    return resolution;
}

// IGSessionCRUD implementation
IGSessionCRUD::IGSessionCRUD(const std::string& base_url, const std::string& api_key, 
                             std::shared_ptr<cpr::Session> session)
    : base_url_(base_url), api_key_(api_key), session_(session) {
    
    // Set default headers and track them
    current_headers_["X-IG-API-KEY"] = api_key_;
    current_headers_["Content-Type"] = "application/json";
    current_headers_["Accept"] = "application/json; charset=UTF-8";
    
    // Set in the session
    session_->SetHeader(cpr::Header{
        {"X-IG-API-KEY", api_key_},
        {"Content-Type", "application/json"},
        {"Accept", "application/json; charset=UTF-8"}
    });
}

std::string IGSessionCRUD::buildUrl(const std::string& endpoint) const {
    return base_url_ + endpoint;
}

cpr::Response IGSessionCRUD::create(const std::string& endpoint, const std::string& params, 
                                  const std::string& version) {
    std::string url = buildUrl(endpoint);
    
    spdlog::info("Making POST request to: {}", url);
    
    // Create header map with all required headers
    cpr::Header headers = {
        {"X-IG-API-KEY", api_key_},
        {"VERSION", version},
        {"Content-Type", "application/json"},
        {"Accept", "application/json; charset=UTF-8"}
    };
    
    // Add Authorization header if it exists in our tracked headers
    if (current_headers_.find("Authorization") != current_headers_.end()) {
        spdlog::info("Including Authorization header in request");
        headers["Authorization"] = current_headers_["Authorization"];
    } else {
        spdlog::warn("No Authorization header found for request");
    }

    // Add IG-ACCOUNT-ID header if it exists in our tracked headers
    if (current_headers_.find("IG-ACCOUNT-ID") != current_headers_.end()) {
        spdlog::info("Including IG-ACCOUNT-ID header: {}", current_headers_["IG-ACCOUNT-ID"]);
        headers["IG-ACCOUNT-ID"] = current_headers_["IG-ACCOUNT-ID"];
    } else {
        spdlog::warn("No IG-ACCOUNT-ID header found for request");
    }
    
    // Update our tracked headers
    current_headers_["VERSION"] = version;
    
    // Set all headers at once
    session_->SetHeader(headers);
    
    session_->SetUrl(cpr::Url{url});
    session_->SetBody(cpr::Body{params});
    auto response = session_->Post();
    spdlog::info("POST '{}', resp {}", endpoint, response.status_code);
    
    if (response.status_code == 401 || response.status_code == 403) {
        if (response.text.find("error.security.oauth-token-invalid") != std::string::npos) {
            throw TokenInvalidException();
        }
    }
    
    return response;
}

cpr::Response IGSessionCRUD::read(const std::string& endpoint, const std::string& params, 
                                const std::string& version) {
    std::string url = buildUrl(endpoint);
    
    spdlog::info("Making GET request to: {}", url);
    
    // Create header map with all required headers
    cpr::Header headers = {
        {"X-IG-API-KEY", api_key_},
        {"VERSION", version},
        {"Content-Type", "application/json"},
        {"Accept", "application/json; charset=UTF-8"}
    };
    
    // Add Authorization header if it exists in our tracked headers
    if (current_headers_.find("Authorization") != current_headers_.end()) {
        spdlog::info("Including Authorization header in request");
        headers["Authorization"] = current_headers_["Authorization"];
    } else {
        spdlog::warn("No Authorization header found for request");
    }

    // Add IG-ACCOUNT-ID header if it exists in our tracked headers
    if (current_headers_.find("IG-ACCOUNT-ID") != current_headers_.end()) {
        spdlog::info("Including IG-ACCOUNT-ID header: {}", current_headers_["IG-ACCOUNT-ID"]);
        headers["IG-ACCOUNT-ID"] = current_headers_["IG-ACCOUNT-ID"];
    } else {
        spdlog::warn("No IG-ACCOUNT-ID header found for request");
    }
    
    // Update our tracked headers
    current_headers_["VERSION"] = version;
    
    // Set all headers at once
    session_->SetHeader(headers);
    
    // Log headers for debugging
    spdlog::info("Headers - API Key: {}..., Version: {}", api_key_.substr(0, 8), version);
    
    session_->SetUrl(cpr::Url{url});
    
    // For GET requests, params should be URL parameters, not body
    if (!params.empty()) {
        spdlog::info("Request params: {}", params);
        json jsonParams = json::parse(params);
        cpr::Parameters parameters;
        for (auto& [key, value] : jsonParams.items()) {
            if (value.is_string()) {
                parameters.Add({key, value.get<std::string>()});
            } else {
                parameters.Add({key, value.dump()});
            }
        }
        session_->SetParameters(parameters);
    } else {
        session_->SetParameters(cpr::Parameters{});  // Clear any existing parameters
    }
    
    cpr::Response response = session_->Get();
    spdlog::info("GET '{}', resp {}, text: {}", endpoint, response.status_code, response.text.substr(0, 200));
    
    return response;
}

cpr::Response IGSessionCRUD::update(const std::string& endpoint, const std::string& params, 
                                  const std::string& version) {
    std::string url = buildUrl(endpoint);
    
    spdlog::info("Making PUT request to: {}", url);
    
    // Create header map with all required headers
    cpr::Header headers = {
        {"X-IG-API-KEY", api_key_},
        {"VERSION", version},
        {"Content-Type", "application/json"},
        {"Accept", "application/json; charset=UTF-8"}
    };
    
    // Add Authorization header if it exists in our tracked headers
    if (current_headers_.find("Authorization") != current_headers_.end()) {
        spdlog::info("Including Authorization header in request");
        headers["Authorization"] = current_headers_["Authorization"];
    } else {
        spdlog::warn("No Authorization header found for request");
    }

    // Add IG-ACCOUNT-ID header if it exists in our tracked headers
    if (current_headers_.find("IG-ACCOUNT-ID") != current_headers_.end()) {
        spdlog::info("Including IG-ACCOUNT-ID header: {}", current_headers_["IG-ACCOUNT-ID"]);
        headers["IG-ACCOUNT-ID"] = current_headers_["IG-ACCOUNT-ID"];
    } else {
        spdlog::warn("No IG-ACCOUNT-ID header found for request");
    }
    
    // Update our tracked headers
    current_headers_["VERSION"] = version;
    
    // Set all headers at once
    session_->SetHeader(headers);
    
    session_->SetUrl(cpr::Url{url});
    session_->SetBody(cpr::Body{params});
    auto response = session_->Put();
    spdlog::info("PUT '{}', resp {}", endpoint, response.status_code);
    
    return response;
}

cpr::Response IGSessionCRUD::delete_req(const std::string& endpoint, const std::string& params, 
                                      const std::string& version) {
    std::string url = buildUrl(endpoint);
    
    spdlog::info("Making DELETE request to: {}", url);
    
    // Create header map with all required headers
    cpr::Header headers = {
        {"X-IG-API-KEY", api_key_},
        {"VERSION", version},
        {"_method", "DELETE"},
        {"Content-Type", "application/json"},
        {"Accept", "application/json; charset=UTF-8"}
    };
    
    // Add Authorization header if it exists in our tracked headers
    if (current_headers_.find("Authorization") != current_headers_.end()) {
        spdlog::info("Including Authorization header in request");
        headers["Authorization"] = current_headers_["Authorization"];
    } else {
        spdlog::warn("No Authorization header found for request");
    }

    // Add IG-ACCOUNT-ID header if it exists in our tracked headers
    if (current_headers_.find("IG-ACCOUNT-ID") != current_headers_.end()) {
        spdlog::info("Including IG-ACCOUNT-ID header: {}", current_headers_["IG-ACCOUNT-ID"]);
        headers["IG-ACCOUNT-ID"] = current_headers_["IG-ACCOUNT-ID"];
    } else {
        spdlog::warn("No IG-ACCOUNT-ID header found for request");
    }
    
    // Update our tracked headers
    current_headers_["VERSION"] = version;
    
    // Set all headers at once
    session_->SetHeader(headers);
    
    session_->SetUrl(cpr::Url{url});
    session_->SetBody(cpr::Body{params});
    auto response = session_->Post();
    spdlog::info("DELETE (POST) '{}', resp {}", endpoint, response.status_code);
    
    return response;
}

cpr::Response IGSessionCRUD::req(const std::string& action, const std::string& endpoint, 
                               const std::string& params, const std::string& version) {
    if (action == "create") {
        return create(endpoint, params, version);
    } else if (action == "read") {
        return read(endpoint, params, version);
    } else if (action == "update") {
        return update(endpoint, params, version);
    } else if (action == "delete") {
        return delete_req(endpoint, params, version);
    } else {
        throw std::invalid_argument("Invalid action: " + action);
    }
}

// IGService implementation
IGService::IGService(const std::string& username, const std::string& password,
                   const std::string& api_key, const std::string& acc_type,
                   const std::string& acc_number, bool use_rate_limiter)
    : api_key_(api_key), 
      username_(username), 
      password_(password), 
      acc_number_(acc_number),
      use_rate_limiter_(use_rate_limiter),
      bucket_threads_run_(false) {
    
    // Convert account type to lowercase for case-insensitive comparison
    std::string acc_type_lower = acc_type;
    std::transform(acc_type_lower.begin(), acc_type_lower.end(), acc_type_lower.begin(),
                  [](unsigned char c){ return std::tolower(c); });
    
    auto it = D_BASE_URL.find(acc_type_lower);
    if (it != D_BASE_URL.end()) {
        base_url_ = it->second;
    } else {
        throw IGException("Invalid account type '" + acc_type + "', please provide LIVE or DEMO");
    }
    
    // Initialize session
    session_ = std::make_shared<cpr::Session>();
    crud_session_ = std::make_unique<IGSessionCRUD>(base_url_, api_key_, session_);
}

IGService::~IGService() {
    exit_bucket_threads();
}

void IGService::setup_rate_limiter() {
    auto data = get_client_apps();
    
    // Find application with matching API key
    for (const auto& acc : data) {
        if (acc["apiKey"] == api_key_) {
            break;
        }
    }
    
    // Clean up any existing threads
    exit_bucket_threads();
    
    // Magic number to reduce API limits to avoid hitting actual limits
    const int MAGIC_NUMBER = 2;
    
    trading_requests_per_minute_ = data[0]["allowanceAccountTrading"].get<int>() - MAGIC_NUMBER;
    spdlog::info("Published IG Trading Request limits for trading request: {} per minute. Using: {}", 
                data[0]["allowanceAccountTrading"].get<int>(), trading_requests_per_minute_);
    
    non_trading_requests_per_minute_ = data[0]["allowanceAccountOverall"].get<int>() - MAGIC_NUMBER;
    spdlog::info("Published IG Trading Request limits for non-trading request: {} per minute. Using: {}", 
                data[0]["allowanceAccountOverall"].get<int>(), non_trading_requests_per_minute_);
    
    // Sleep briefly to ensure we don't hit rate limits
    std::this_thread::sleep_for(std::chrono::milliseconds(
        static_cast<int>(60000.0 / non_trading_requests_per_minute_)));
    
    bucket_threads_run_ = true;
    
    // Start token bucket for trading requests
    const int trading_requests_burst = 1;
    std::unique_lock<std::mutex> trading_lock(trading_queue_mutex_);
    for (int i = 0; i < trading_requests_burst; ++i) {
        trading_requests_queue_.push(true);
    }
    trading_lock.unlock();
    
    token_bucket_trading_thread_ = std::make_unique<std::thread>(&IGService::token_bucket_trading, this);
    
    // Start token bucket for non-trading requests
    const int non_trading_requests_burst = 1;
    std::unique_lock<std::mutex> non_trading_lock(non_trading_queue_mutex_);
    for (int i = 0; i < non_trading_requests_burst; ++i) {
        non_trading_requests_queue_.push(true);
    }
    non_trading_lock.unlock();
    
    token_bucket_non_trading_thread_ = std::make_unique<std::thread>(&IGService::token_bucket_non_trading, this);
}

void IGService::token_bucket_trading() {
    while (bucket_threads_run_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(60000.0 / trading_requests_per_minute_)));
        
        std::unique_lock<std::mutex> lock(trading_queue_mutex_);
        trading_requests_queue_.push(true);
        lock.unlock();
    }
}

void IGService::token_bucket_non_trading() {
    while (bucket_threads_run_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(60000.0 / non_trading_requests_per_minute_)));
        
        std::unique_lock<std::mutex> lock(non_trading_queue_mutex_);
        non_trading_requests_queue_.push(true);
        lock.unlock();
    }
}

void IGService::trading_rate_limit_pause_or_pass() {
    if (use_rate_limiter_) {
        std::unique_lock<std::mutex> lock(trading_queue_mutex_);
        if (trading_requests_queue_.empty()) {
            lock.unlock();
            // Wait for a token
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(60000.0 / trading_requests_per_minute_)));
            lock.lock();
        }
        
        trading_requests_queue_.pop();
        lock.unlock();
        
        auto now = std::chrono::system_clock::now();
        trading_times_.push_back(now);
        
        // Remove expired timestamps (older than 60 seconds)
        auto sixtySecondsAgo = now - std::chrono::seconds(60);
        trading_times_.erase(
            std::remove_if(trading_times_.begin(), trading_times_.end(),
                          [sixtySecondsAgo](const auto& time) { return time < sixtySecondsAgo; }),
            trading_times_.end());
        
        spdlog::info("Number of trading requests in last 60 seconds = {} of {}",
                    trading_times_.size(), trading_requests_per_minute_);
    }
}

void IGService::non_trading_rate_limit_pause_or_pass() {
    if (use_rate_limiter_) {
        std::unique_lock<std::mutex> lock(non_trading_queue_mutex_);
        if (non_trading_requests_queue_.empty()) {
            lock.unlock();
            // Wait for a token
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(60000.0 / non_trading_requests_per_minute_)));
            lock.lock();
        }
        
        non_trading_requests_queue_.pop();
        lock.unlock();
        
        auto now = std::chrono::system_clock::now();
        non_trading_times_.push_back(now);
        
        // Remove expired timestamps (older than 60 seconds)
        auto sixtySecondsAgo = now - std::chrono::seconds(60);
        non_trading_times_.erase(
            std::remove_if(non_trading_times_.begin(), non_trading_times_.end(),
                          [sixtySecondsAgo](const auto& time) { return time < sixtySecondsAgo; }),
            non_trading_times_.end());
        
        spdlog::info("Number of non-trading requests in last 60 seconds = {} of {}",
                    non_trading_times_.size(), non_trading_requests_per_minute_);
    }
}

void IGService::exit_bucket_threads() {
    if (use_rate_limiter_ && bucket_threads_run_) {
        bucket_threads_run_ = false;
        
        // Clean up trading queue
        std::unique_lock<std::mutex> trading_lock(trading_queue_mutex_);
        while (!trading_requests_queue_.empty()) {
            trading_requests_queue_.pop();
        }
        trading_lock.unlock();
        
        // Clean up non-trading queue
        std::unique_lock<std::mutex> non_trading_lock(non_trading_queue_mutex_);
        while (!non_trading_requests_queue_.empty()) {
            non_trading_requests_queue_.pop();
        }
        non_trading_lock.unlock();
        
        // Join threads if they exist
        if (token_bucket_trading_thread_ && token_bucket_trading_thread_->joinable()) {
            token_bucket_trading_thread_->join();
        }
        
        if (token_bucket_non_trading_thread_ && token_bucket_non_trading_thread_->joinable()) {
            token_bucket_non_trading_thread_->join();
        }
    }
}

cpr::Response IGService::request(const std::string& action, const std::string& endpoint,
                               const std::string& params, const std::string& version, bool check) {
    spdlog::info("Making {} request to {}", action, endpoint); // Ajouté
    
    if (check) {
        check_session();
    }
    
    auto response = crud_session_->req(action, endpoint, params, version);
    
    spdlog::info("Response status: {}, text: {}", response.status_code, response.text.substr(0, 200)); // Ajouté
    
    if (response.status_code >= 500) {
        throw IGException("Server problem: status code: " + std::to_string(response.status_code) + 
                         ", reason: " + response.reason);
    }
    
    if (api_limit_hit(response.text)) {
        throw ApiExceededException();
    }
    
    if (token_invalid(response.text)) {
        spdlog::warn("Invalid session token, triggering refresh...");
        valid_until_ = std::chrono::system_clock::now() - std::chrono::seconds(15);
        throw TokenInvalidException();
    }
    
    return response;
}

json IGService::parse_response(const std::string& response_text) {
    try {
        json response = json::parse(response_text);
        if (response.contains("errorCode")) {
            throw IGException(response["errorCode"].get<std::string>());
        }
        return response;
    } catch (const json::exception& e) {
        spdlog::error("JSON parsing error: {}", e.what());
        spdlog::error("Response text (partial): {:.100}...", response_text);
        throw;
    }
}

nlohmann::json IGService::create_session() {
    if (acc_number_.empty()) {
        throw IGException("Account number must be set for v3 sessions");
    }
    
    spdlog::info("Creating new v3 session for user '{}' at '{}'", username_, base_url_);
    
    json params = {
        {"identifier", username_},
        {"password", password_}
    };
    
    const std::string version = "3";
    auto response = request("create", "/session", params.dump(), version, false);
    
    // Set the account ID header for v3 authentication
    session_->SetHeader(cpr::Header{{"IG-ACCOUNT-ID", acc_number_}});
    
    // Track the account ID header in crud_session_
    if (crud_session_) {
        crud_session_->current_headers_["IG-ACCOUNT-ID"] = acc_number_;
    }
    
    // Handle OAuth token response
    json data = parse_response(response.text);
    if (data.contains("oauthToken")) {
        handle_oauth(data["oauthToken"]);
    }
    
    if (use_rate_limiter_) {
        setup_rate_limiter();
    }
    
    return data;
}

void IGService::handle_oauth(const nlohmann::json& oauth) {
    std::string access_token = oauth["access_token"];
    std::string token_type = oauth["token_type"];
    
    // Store the complete authorization header string
    authorization_header_ = token_type + " " + access_token;
    
    // Set the header in the session
    session_->SetHeader(cpr::Header{{"Authorization", authorization_header_}});
    
    // Also update the tracked headers in the crud_session_
    if (crud_session_) {
        crud_session_->current_headers_["Authorization"] = authorization_header_;
    }
    
    refresh_token_ = oauth["refresh_token"];
    
    // Handle expires_in which might be a string or a number
    int validity;
    if (oauth["expires_in"].is_string()) {
        validity = std::stoi(oauth["expires_in"].get<std::string>());
    } else {
        validity = oauth["expires_in"].get<int>();
    }
    
    valid_until_ = std::chrono::system_clock::now() + std::chrono::seconds(validity);
    
    spdlog::info("OAuth token received. Valid for {} seconds", validity);
}

int IGService::refresh_session() {
    spdlog::info("Refreshing session '{}'", username_);
    
    json params = {
        {"refresh_token", refresh_token_}
    };
    
    const std::string version = "1";
    auto response = request("create", "/session/refresh-token", params.dump(), version, false);
    
    // Ensure we keep the IG-ACCOUNT-ID header set after refresh
    session_->SetHeader(cpr::Header{{"IG-ACCOUNT-ID", acc_number_}});
    
    // Make sure it's tracked in crud_session_ too
    if (crud_session_) {
        crud_session_->current_headers_["IG-ACCOUNT-ID"] = acc_number_;
    }
    
    handle_oauth(json::parse(response.text));
    
    return response.status_code;
}

void IGService::check_session() {
    spdlog::info("Checking session status..."); // Changé de debug à info
    
    // Check if token will expire soon or has already expired
    if (valid_until_ != std::chrono::system_clock::time_point{}) {
        auto time_until_expiry = std::chrono::duration_cast<std::chrono::seconds>(
            valid_until_ - std::chrono::system_clock::now()).count();
        
        spdlog::info("Session expires in {} seconds", time_until_expiry); // Ajouté
        
        if (time_until_expiry < 10) {  // Token expires in less than 10 seconds
            if (!refresh_token_.empty()) {
                spdlog::info("Proactively refreshing session (expires in {} seconds)", time_until_expiry);
                refresh_session();
            } else {
                spdlog::info("No refresh token available, creating new session...");
                create_session();
            }
        }
    } else {
        spdlog::info("No valid session found, creating new session...");
        create_session();
    }
}

void IGService::logout() {
    const std::string version = "1";
    json params = json::object();
    std::string endpoint = "/session";
    
    request("delete", endpoint, params.dump(), version);
    exit_bucket_threads();
}

nlohmann::json IGService::switch_account(const std::string& account_id, bool default_account) {
    const std::string version = "1";
    json params = {
        {"accountId", account_id},
        {"defaultAccount", default_account}
    };
    std::string endpoint = "/session";
    
    auto response = request("update", endpoint, params.dump(), version);
    
    // Update the account ID header
    session_->SetHeader(cpr::Header{{"IG-ACCOUNT-ID", account_id}});
    
    return parse_response(response.text);
}

nlohmann::json IGService::read_session() {
    const std::string version = "1";
    json params = json::object();
    std::string endpoint = "/session";
    
    auto response = request("read", endpoint, params.dump(), version);

    if (response.status_code != 200) {
        throw IGException("Error in read_session() " + std::to_string(response.status_code));
    }
    
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_accounts() {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "1";
    json params = json::object();
    std::string endpoint = "/accounts";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_account_preferences() {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "1";
    json params = json::object();
    std::string endpoint = "/accounts/preferences";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

std::string IGService::update_account_preferences(bool trailing_stops_enabled) {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "1";
    json params = {
        {"trailingStopsEnabled", trailing_stops_enabled ? "true" : "false"}
    };
    std::string endpoint = "/accounts/preferences";
    
    auto response = request("update", endpoint, params.dump(), version);
    auto update_status = parse_response(response.text);
    return update_status["status"];
}

nlohmann::json IGService::fetch_account_activity(
    std::optional<std::chrono::system_clock::time_point> from_date,
    std::optional<std::chrono::system_clock::time_point> to_date,
    bool detailed,
    const std::string& deal_id,
    const std::string& fiql_filter,
    int page_size) {
    
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "3";
    json params = json::object();
    
    if (from_date) {
        params["from"] = format_datetime(*from_date);
    }
    
    if (to_date) {
        params["to"] = format_datetime(*to_date);
    }
    
    if (detailed) {
        params["detailed"] = "true";
    }
    
    if (!deal_id.empty()) {
        params["dealId"] = deal_id;
    }
    
    if (!fiql_filter.empty()) {
        params["filter"] = fiql_filter;
    }
    
    params["pageSize"] = page_size;
    std::string endpoint = "/history/activity/";
    
    // Variables to handle pagination
    json result;
    std::vector<json> activities;
    bool more_results = true;
    
    while (more_results) {
        auto response = request("read", endpoint, params.dump(), version);
        json data = parse_response(response.text);
        
        // Extract activities from this page
        for (const auto& activity : data["activities"]) {
            activities.push_back(activity);
        }
        
        // Check if there are more pages
        json paging = data["metadata"]["paging"];
        if (paging["next"].is_null()) {
            more_results = false;
        } else {
            // Extract new parameters from the next URL
            std::string next_url = paging["next"];
            
            // Parse URL to extract query parameters
            size_t query_start = next_url.find('?');
            if (query_start != std::string::npos) {
                std::string query = next_url.substr(query_start + 1);
                
                // Simple query parsing - in a real implementation, use a proper URL parser
                auto parse_query = [](const std::string& query) -> std::unordered_map<std::string, std::string> {
                    std::unordered_map<std::string, std::string> result;
                    std::stringstream ss(query);
                    std::string item;
                    
                    while (std::getline(ss, item, '&')) {
                        size_t pos = item.find('=');
                        if (pos != std::string::npos) {
                            std::string key = item.substr(0, pos);
                            std::string value = item.substr(pos + 1);
                            result[key] = value;
                        }
                    }
                    
                    return result;
                };
                
                auto query_params = parse_query(query);
                
                // Update params for next request
                if (query_params.find("from") != query_params.end()) {
                    params["from"] = query_params["from"];
                } else if (params.contains("from")) {
                    params.erase("from");
                }
                
                if (query_params.find("to") != query_params.end()) {
                    params["to"] = query_params["to"];
                } else if (params.contains("to")) {
                    params.erase("to");
                }
            }
        }
    }
    
    // Construct final result
    result = json::object();
    result["activities"] = activities;
    
    return result;
}

nlohmann::json IGService::fetch_deal_by_deal_reference(const std::string& deal_reference) {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "1";
    json params = json::object();
    std::string endpoint = "/confirms/" + deal_reference;
    
    // Retry logic for deal confirmation
    cpr::Response response;
    for (int i = 0; i < 5; i++) {
        response = request("read", endpoint, params.dump(), version);
        if (response.status_code == 200) {
            break;
        }
        
        spdlog::info("Deal reference {} not found, retrying.", deal_reference);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_open_position_by_deal_id(const std::string& deal_id) {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "2";
    json params = json::object();
    std::string endpoint = "/positions/" + deal_id;
    
    // Retry logic for position lookup
    cpr::Response response;
    for (int i = 0; i < 5; i++) {
        response = request("read", endpoint, params.dump(), version);
        if (response.status_code == 200) {
            break;
        }
        
        spdlog::info("Deal id {} not found, retrying.", deal_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_open_positions() {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "2";
    json params = json::object();
    std::string endpoint = "/positions";
    
    // Retry logic for position lookup
    cpr::Response response;
    for (int i = 0; i < 5; i++) {
        response = request("read", endpoint, params.dump(), version);
        if (response.status_code == 200) {
            break;
        }
        
        spdlog::info("Error fetching open positions, retrying.");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return parse_response(response.text);
}

nlohmann::json IGService::close_open_position(
    const std::string& deal_id,
    const std::string& direction,
    const std::string& epic,
    const std::string& expiry,
    double level,
    const std::string& order_type,
    const std::string& quote_id,
    double size,
    const std::string& time_in_force) {
    
    trading_rate_limit_pause_or_pass();
    const std::string version = "1";
    
    json params = {
        {"dealId", deal_id},
        {"direction", direction},
        {"epic", epic},
        {"expiry", expiry},
        {"level", level},
        {"orderType", order_type},
        {"quoteId", quote_id},
        {"size", size}
    };
    
    if (!time_in_force.empty()) {
        params["timeInForce"] = time_in_force;
    }
    
    std::string endpoint = "/positions/otc";
    
    auto response = request("delete", endpoint, params.dump(), version);
    
    if (response.status_code == 200) {
        json result = json::parse(response.text);
        std::string deal_reference = result["dealReference"];
        return fetch_deal_by_deal_reference(deal_reference);
    } else {
        throw IGException(response.text);
    }
}

nlohmann::json IGService::create_open_position(
    const std::string& currency_code,
    const std::string& direction,
    const std::string& epic,
    const std::string& expiry,
    bool force_open,
    bool guaranteed_stop,
    double level,
    double limit_distance,
    double limit_level,
    const std::string& order_type,
    const std::string& quote_id,
    double size,
    double stop_distance,
    double stop_level,
    bool trailing_stop,
    double trailing_stop_increment,
    const std::string& time_in_force) {
    
    trading_rate_limit_pause_or_pass();
    const std::string version = "2";
    
json params = {
        {"currencyCode", currency_code},
        {"direction", direction},
        {"epic", epic},
        {"expiry", expiry},
        {"forceOpen", force_open},
        {"guaranteedStop", guaranteed_stop},
        {"orderType", order_type},
        {"size", size},
        {"trailingStop", trailing_stop}
    };
    
    // Only include level for non-MARKET orders
    if (order_type != "MARKET" && level != 0.0) {
        params["level"] = level;
    }
    
    // Only include quoteId for QUOTE orders
    if (order_type == "QUOTE" && !quote_id.empty()) {
        params["quoteId"] = quote_id;
    }
    
    // Only include limit parameters if they're non-zero
    if (limit_distance > 0.0) {
        params["limitDistance"] = limit_distance;
    }
    if (limit_level > 0.0) {
        params["limitLevel"] = limit_level;
    }
    
    // Only include stop parameters if they're non-zero
    if (stop_distance > 0.0) {
        params["stopDistance"] = stop_distance;
    }
    if (stop_level > 0.0) {
        params["stopLevel"] = stop_level;
    }
    
    // Only include trailing stop increment if trailing stop is enabled
    if (trailing_stop && trailing_stop_increment > 0.0) {
        params["trailingStopIncrement"] = trailing_stop_increment;
    }
    
    if (!time_in_force.empty()) {
        params["timeInForce"] = time_in_force;
    }
    
    std::string endpoint = "/positions/otc";
    
    auto response = request("create", endpoint, params.dump(), version);
    
    if (response.status_code == 200) {
        json result = json::parse(response.text);
        std::string deal_reference = result["dealReference"];
        return fetch_deal_by_deal_reference(deal_reference);
    } else {
        throw IGException(response.text);
    }
}

nlohmann::json IGService::fetch_market_by_epic(const std::string& epic) {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "3";
    json params = json::object();
    std::string endpoint = "/markets/" + epic;
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::search_markets(const std::string& search_term) {
    non_trading_rate_limit_pause_or_pass();
    const std::string version = "1";
    json params = {
        {"searchTerm", search_term}
    };
    std::string endpoint = "/markets";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_historical_prices_by_epic(
    const std::string& epic,
    const std::string& resolution,
    const std::string& start_date,
    const std::string& end_date,
    int numpoints,
    int pagesize,
    int wait) {
    
    const std::string version = "3";
    json params = json::object();
    
    if (!resolution.empty()) {
        // Convert the resolution to the format expected by the API
        params["resolution"] = conv_resol(resolution);
    }
    
    if (!start_date.empty()) {
        params["from"] = start_date;
    }
    
    if (!end_date.empty()) {
        params["to"] = end_date;
    }
    
    if (numpoints > 0) {
        params["max"] = numpoints;
    }
    
    params["pageSize"] = pagesize;
    std::string endpoint = "/prices/" + epic;
    
    // Variables to handle pagination
    std::vector<json> prices;
    int page_number = 1;
    bool more_results = true;
    json final_data; // Store the last response for metadata
    
    while (more_results) {
        params["pageNumber"] = page_number;
        auto response = request("read", endpoint, params.dump(), version);
        json data = parse_response(response.text);
        final_data = data; // Keep the last data for metadata
        
        // Extract prices from this page
        for (const auto& price : data["prices"]) {
            prices.push_back(price);
        }
        
        // Check if there are more pages
        json page_data = data["metadata"]["pageData"];
        if (page_data["totalPages"] == 0 || page_data["pageNumber"] == page_data["totalPages"]) {
            more_results = false;
        } else {
            page_number++;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(wait));
    }
    
    // Construct final result
    json result = json::object();
    result["prices"] = prices;
    result["metadata"] = final_data["metadata"];
    
    // Log allowance if we have metadata
    if (final_data.contains("metadata") && final_data["metadata"].contains("allowance")) {
        int remaining_allowance = final_data["metadata"]["allowance"]["remainingAllowance"];
        int allowance_expiry = final_data["metadata"]["allowance"]["allowanceExpiry"];
        auto allowance_expiry_time = std::chrono::system_clock::now() + std::chrono::seconds(allowance_expiry);
        
        std::time_t expiry_time = std::chrono::system_clock::to_time_t(allowance_expiry_time);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&expiry_time), "%Y-%m-%d %H:%M:%S");
        
        spdlog::info("Historic price data allowance: {} remaining until {}", 
                    remaining_allowance, ss.str());
    }
    
    return result;
}

nlohmann::json IGService::get_client_apps() {
    const std::string version = "1";
    json params = json::object();
    std::string endpoint = "/operations/application";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

} // namespace ig