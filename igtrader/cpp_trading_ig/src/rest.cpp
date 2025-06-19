#include "../include/rest.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cryptopp/rsa.h>
#include <cryptopp/base64.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pssr.h>
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
    return response_text.find("oauth-token-invalid") != std::string::npos ||
           response_text.find("client-token-invalid") != std::string::npos;
}

// IGSessionCRUD implementation
IGSessionCRUD::IGSessionCRUD(const std::string& base_url, const std::string& api_key, 
                             std::shared_ptr<cpr::Session> session)
    : base_url_(base_url), api_key_(api_key), session_(session) {
    
    // Set default headers
    session_->SetHeader(cpr::Header{{"X-IG-API-KEY", api_key_},
                                   {"Content-Type", "application/json"},
                                   {"Accept", "application/json; charset=UTF-8"}});
}

std::string IGSessionCRUD::buildUrl(const std::string& endpoint) const {
    return base_url_ + endpoint;
}

cpr::Response IGSessionCRUD::create(const std::string& endpoint, const std::string& params, 
                                  const std::string& version) {
    std::string url = buildUrl(endpoint);
    session_->SetHeader(cpr::Header{{"VERSION", version}});
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
    session_->SetHeader(cpr::Header{{"VERSION", version}});
    session_->SetUrl(cpr::Url{url});
    
    // For GET requests, params should be URL parameters, not body
    if (!params.empty()) {
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
    }
    
    auto response = session_->Get();
    spdlog::info("GET '{}', resp {}", endpoint, response.status_code);
    
    // Handle session tokens if present
    if (response.header.find("CST") != response.header.end()) {
        session_->SetHeader(cpr::Header{{"CST", response.header["CST"]}});
    }
    if (response.header.find("X-SECURITY-TOKEN") != response.header.end()) {
        session_->SetHeader(cpr::Header{{"X-SECURITY-TOKEN", response.header["X-SECURITY-TOKEN"]}});
    }
    
    return response;
}

cpr::Response IGSessionCRUD::update(const std::string& endpoint, const std::string& params, 
                                  const std::string& version) {
    std::string url = buildUrl(endpoint);
    session_->SetHeader(cpr::Header{{"VERSION", version}});
    session_->SetUrl(cpr::Url{url});
    session_->SetBody(cpr::Body{params});
    auto response = session_->Put();
    spdlog::info("PUT '{}', resp {}", endpoint, response.status_code);
    return response;
}

cpr::Response IGSessionCRUD::delete_req(const std::string& endpoint, const std::string& params, 
                                      const std::string& version) {
    std::string url = buildUrl(endpoint);
    session_->SetHeader(cpr::Header{{"VERSION", version}, {"_method", "DELETE"}});
    session_->SetUrl(cpr::Url{url});
    session_->SetBody(cpr::Body{params});
    auto response = session_->Post();
    spdlog::info("DELETE (POST) '{}', resp {}", endpoint, response.status_code);
    
    // Remove _method header after use
    session_->UpdateHeader(cpr::Header{{"_method", ""}});
    
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
    
    try {
        auto it = D_BASE_URL.find(acc_type);
        if (it != D_BASE_URL.end()) {
            base_url_ = it->second;
        } else {
            throw IGException("Invalid account type '" + acc_type + "', please provide LIVE or DEMO");
        }
    } catch (const std::exception& e) {
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
    if (check) {
        check_session();
    }
    
    auto response = crud_session_->req(action, endpoint, params, version);
    
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
    json response = json::parse(response_text);
    if (response.contains("errorCode")) {
        throw IGException(response["errorCode"].get<std::string>());
    }
    return response;
}

nlohmann::json IGService::create_session(bool encryption, const std::string& version) {
    if (version == "3" && acc_number_.empty()) {
        throw IGException("Account number must be set for v3 sessions");
    }
    
    spdlog::info("Creating new v{} session for user '{}' at '{}'", version, username_, base_url_);
    
    std::string password_str = encryption ? encrypted_password() : password_;
    
    json params = {
        {"identifier", username_},
        {"password", password_str}
    };
    
    if (encryption) {
        params["encryptedPassword"] = true;
    }
    
    auto response = request("create", "/session", params.dump(), version, false);
    manage_headers(response);
    json data = parse_response(response.text);
    
    if (use_rate_limiter_) {
        setup_rate_limiter();
    }
    
    return data;
}

void IGService::manage_headers(const cpr::Response& response) {
    // Handle v1 and v2 logins
    if (response.header.find("CST") != response.header.end()) {
        session_->SetHeader(cpr::Header{{"CST", response.header.at("CST")}});
    }
    
    if (response.header.find("X-SECURITY-TOKEN") != response.header.end()) {
        session_->SetHeader(cpr::Header{{"X-SECURITY-TOKEN", response.header.at("X-SECURITY-TOKEN")}});
    }
    
    // Handle v3 logins
    if (!response.text.empty()) {
        session_->SetHeader(cpr::Header{{"IG-ACCOUNT-ID", acc_number_}});
        json payload = json::parse(response.text);
        if (payload.contains("oauthToken")) {
            handle_oauth(payload["oauthToken"]);
        }
    }
}

void IGService::handle_oauth(const nlohmann::json& oauth) {
    std::string access_token = oauth["access_token"];
    std::string token_type = oauth["token_type"];
    session_->SetHeader(cpr::Header{{"Authorization", token_type + " " + access_token}});
    refresh_token_ = oauth["refresh_token"];
    int validity = oauth["expires_in"];
    valid_until_ = std::chrono::system_clock::now() + std::chrono::seconds(validity);
}

int IGService::refresh_session(const std::string& version) {
    spdlog::info("Refreshing session '{}'", username_);
    
    json params = {
        {"refresh_token", refresh_token_}
    };
    
    auto response = request("create", "/session/refresh-token", params.dump(), version, false);
    handle_oauth(json::parse(response.text));
    
    return response.status_code;
}

void IGService::check_session() {
    spdlog::debug("Checking session status...");
    
    // Check if token will expire soon or has already expired
    if (valid_until_ != std::chrono::system_clock::time_point{}) {
        auto time_until_expiry = std::chrono::duration_cast<std::chrono::seconds>(
            valid_until_ - std::chrono::system_clock::now()).count();
        
        if (time_until_expiry < 10) {  // Token expires in less than 10 seconds
            if (!refresh_token_.empty()) {
                try {
                    spdlog::info("Proactively refreshing session (expires in {:.1f} seconds)", time_until_expiry);
                    refresh_session();
                } catch (const IGException&) {
                    spdlog::info("Proactive refresh failed, logging in again...");
                    refresh_token_.clear();
                    valid_until_ = std::chrono::system_clock::time_point{};
                    session_->UpdateHeader(cpr::Header{{"Authorization", ""}});
                    create_session(false, "3");
                }
            }
        }
    } else if (std::chrono::system_clock::now() > valid_until_) {
        if (!refresh_token_.empty()) {
            try {
                spdlog::info("Current session has expired, refreshing...");
                refresh_session();
            } catch (const IGException&) {
                spdlog::info("Refresh failed, logging in again...");
                refresh_token_.clear();
                valid_until_ = std::chrono::system_clock::time_point{};
                session_->UpdateHeader(cpr::Header{{"Authorization", ""}});
                create_session(false, "3");
            }
        }
    }
}

std::pair<std::string, std::string> IGService::get_encryption_key() {
    session_->SetUrl(cpr::Url{base_url_ + "/session/encryptionKey"});
    auto response = session_->Get();

    if (response.status_code != 200) {
        throw IGException("Could not get encryption key for login.");
    }
    
    json data = json::parse(response.text);
    return {data["encryptionKey"], data["timeStamp"]};
}

std::string IGService::encrypted_password() {
    auto [key, timestamp] = get_encryption_key();
    
    // Use CryptoPP for RSA encryption
    CryptoPP::Base64Decoder decoder;
    decoder.Put((CryptoPP::byte*)key.data(), key.size());
    decoder.MessageEnd();
    
    CryptoPP::ByteQueue bytes;
    decoder.CopyTo(bytes);
    bytes.MessageEnd();
    
    CryptoPP::RSA::PublicKey publicKey;
    publicKey.Load(bytes);
    
    std::string message = password_ + "|" + timestamp;
    
    // Encrypt the message
    CryptoPP::RSAES_PKCS1v15_Encryptor encryptor(publicKey);
    
    CryptoPP::AutoSeededRandomPool rng;
    std::string encrypted;
    CryptoPP::StringSource(message, true,
        new CryptoPP::PK_EncryptorFilter(rng, encryptor,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(encrypted)
            )
        )
    );
    
    return encrypted;
}

void IGService::logout() {
    std::string version = "1";
    json params = json::object();
    std::string endpoint = "/session";
    
    request("delete", endpoint, params.dump(), version);
    exit_bucket_threads();
}

nlohmann::json IGService::switch_account(const std::string& account_id, bool default_account) {
    std::string version = "1";
    json params = {
        {"accountId", account_id},
        {"defaultAccount", default_account}
    };
    std::string endpoint = "/session";
    
    auto response = request("update", endpoint, params.dump(), version);
    manage_headers(response);
    return parse_response(response.text);
}

nlohmann::json IGService::read_session(const std::string& fetch_session_tokens) {
    std::string version = "1";
    json params = {
        {"fetchSessionTokens", fetch_session_tokens}
    };
    std::string endpoint = "/session";
    
    auto response = request("read", endpoint, params.dump(), version);

    if (response.status_code != 200) {
        throw IGException("Error in read_session() " + std::to_string(response.status_code));
    }
    
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_accounts() {
    non_trading_rate_limit_pause_or_pass();
    std::string version = "1";
    json params = json::object();
    std::string endpoint = "/accounts";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_account_preferences() {
    non_trading_rate_limit_pause_or_pass();
    std::string version = "1";
    json params = json::object();
    std::string endpoint = "/accounts/preferences";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

std::string IGService::update_account_preferences(bool trailing_stops_enabled) {
    non_trading_rate_limit_pause_or_pass();
    std::string version = "1";
    json params = {
        {"trailingStopsEnabled", trailing_stops_enabled ? "true" : "false"}
    };
    std::string endpoint = "/accounts/preferences";
    
    auto response = request("update", endpoint, params.dump(), version);
    auto update_status = parse_response(response.text);
    return update_status["status"];
}

nlohmann::json IGService::fetch_account_activity_by_period(int64_t milliseconds) {
    non_trading_rate_limit_pause_or_pass();
    std::string version = "1";
    json params = json::object();
    std::string endpoint = "/history/activity/" + std::to_string(milliseconds);
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_account_activity_by_date(
    const std::chrono::system_clock::time_point& from_date,
    const std::chrono::system_clock::time_point& to_date) {
    
    non_trading_rate_limit_pause_or_pass();
    std::string version = "1";
    
    if (from_date > to_date) {
        throw IGException("from_date must be before to_date");
    }
    
    json params = json::object();
    std::string from_date_str = format_date(from_date);
    std::string to_date_str = format_date(to_date);
    std::string endpoint = "/history/activity/" + from_date_str + "/" + to_date_str;
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::fetch_account_activity_v2(
    std::optional<std::chrono::system_clock::time_point> from_date,
    std::optional<std::chrono::system_clock::time_point> to_date,
    std::optional<int> max_span_seconds,
    int page_size) {
    
    non_trading_rate_limit_pause_or_pass();
    std::string version = "2";
    json params = json::object();
    
    if (from_date) {
        params["from"] = format_datetime(*from_date);
    }
    
    if (to_date) {
        params["to"] = format_datetime(*to_date);
    }
    
    if (max_span_seconds) {
        params["maxSpanSeconds"] = *max_span_seconds;
    }
    
    params["pageSize"] = page_size;
    std::string endpoint = "/history/activity/";
    
    // Variables to handle pagination
    json result;
    std::vector<json> activities;
    int page_number = 1;
    bool more_results = true;
    
    while (more_results) {
        params["pageNumber"] = page_number;
        auto response = request("read", endpoint, params.dump(), version);
        json data = parse_response(response.text);
        
        // Extract activities from this page
        for (const auto& activity : data["activities"]) {
            activities.push_back(activity);
        }
        
        // Check if there are more pages
        json page_data = data["metadata"]["pageData"];
        if (page_data["totalPages"] == 0 || page_data["pageNumber"] == page_data["totalPages"]) {
            more_results = false;
        } else {
            page_number++;
        }
    }
    
    // Construct final result
    result = json::object();
    result["activities"] = activities;
    
    return result;
}

nlohmann::json IGService::fetch_account_activity(
    std::optional<std::chrono::system_clock::time_point> from_date,
    std::optional<std::chrono::system_clock::time_point> to_date,
    bool detailed,
    const std::string& deal_id,
    const std::string& fiql_filter,
    int page_size) {
    
    non_trading_rate_limit_pause_or_pass();
    std::string version = "3";
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
    std::string version = "1";
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
    std::string version = "2";
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

nlohmann::json IGService::fetch_open_positions(const std::string& version) {
    non_trading_rate_limit_pause_or_pass();
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
    std::string version = "1";
    
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
    std::string version = "2";
    
    json params = {
        {"currencyCode", currency_code},
        {"direction", direction},
        {"epic", epic},
        {"expiry", expiry},
        {"forceOpen", force_open},
        {"guaranteedStop", guaranteed_stop},
        {"level", level},
        {"limitDistance", limit_distance},
        {"limitLevel", limit_level},
        {"orderType", order_type},
        {"quoteId", quote_id},
        {"size", size},
        {"stopDistance", stop_distance},
        {"stopLevel", stop_level},
        {"trailingStop", trailing_stop},
        {"trailingStopIncrement", trailing_stop_increment}
    };
    
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
    std::string version = "3";
    json params = json::object();
    std::string endpoint = "/markets/" + epic;
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

nlohmann::json IGService::search_markets(const std::string& search_term) {
    non_trading_rate_limit_pause_or_pass();
    std::string version = "1";
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
    
    std::string version = "3";
    json params = json::object();
    
    if (!resolution.empty()) {
        params["resolution"] = resolution;
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
    std::string version = "1";
    json params = json::object();
    std::string endpoint = "/operations/application";
    
    auto response = request("read", endpoint, params.dump(), version);
    return parse_response(response.text);
}

} // namespace ig