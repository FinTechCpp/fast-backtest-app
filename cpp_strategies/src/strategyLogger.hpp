#pragma once
#include "common.h"
#include <string>
#include <vector>
#include <sstream>
#include <memory>

enum class LogCategory {
    GENERAL,
    INDICATOR,
    FILTER,
    SIGNAL,
    EXECUTION,
    RISK,
    TIME
};

// enum LogLevel {
//     DEBUG = 0,
//     WARNING = 1,
//     INFO = 2,
//     ERROR = 3
// };

class StrategyLogger {
private:
    bool enabled = true;
    LogLevel verbosity_level = LogLevel::DEBUG;
    DateTime current_candle_date;
    
    // Buffer pour stocker les logs par catégorie
    std::vector<std::string> general_logs;
    std::vector<std::string> indicator_logs;
    std::vector<std::string> filter_logs;
    std::vector<std::string> signal_logs;
    std::vector<std::string> execution_logs;
    std::vector<std::string> risk_logs;
    std::vector<std::string> time_logs;
    
    // Indentation pour les logs hiérarchiques
    std::string indent(int level) const {
        return std::string(level * 2, ' ');
    }
    
    // Formatage d'une valeur numérique avec précision
    std::string format_value(double value, int precision = 4) const {
        std::ostringstream ss;
        ss.precision(precision);
        ss << std::fixed << value;
        return ss.str();
    }
    
    // Ajout d'un log à la catégorie appropriée
    void add_log(LogCategory category, const std::string& message, int level = LogLevel::INFO) {
        if (!enabled || level < verbosity_level) {
            return;
        }
        
        std::string formatted = "[" + current_candle_date.to_string() + "] " + message;
        // std::string formatted = indent(2) + message;
        
        switch (category) {
            case LogCategory::GENERAL:   general_logs.push_back(formatted); break;
            case LogCategory::INDICATOR: indicator_logs.push_back(formatted); break;
            case LogCategory::FILTER:    filter_logs.push_back(formatted); break;
            case LogCategory::SIGNAL:    signal_logs.push_back(formatted); break;
            case LogCategory::EXECUTION: execution_logs.push_back(formatted); break;
            case LogCategory::RISK:      risk_logs.push_back(formatted); break;
            case LogCategory::TIME:      time_logs.push_back(formatted); break;
        }
        
        // Toujours envoyer au système de log standard
        // cpp_log(message, level);
    }

public:
    StrategyLogger() = default;
    
    // Configuration du logger
    void set_enabled(bool state) { enabled = state; }
    void set_verbosity(int level) { verbosity_level = static_cast<LogLevel>(level); }
    LogLevel get_verbosity() const { return verbosity_level; }
    void set_current_candle(const Candle& candle) { current_candle_date = candle.date; }
    void clear() {
        general_logs.clear();
        indicator_logs.clear();
        filter_logs.clear();
        signal_logs.clear();
        execution_logs.clear();
        risk_logs.clear();
        time_logs.clear();
    }
    
    // Logs généraux
    void log_general(const std::string& message, int level = LogLevel::INFO) {
        add_log(LogCategory::GENERAL, message, level);
    }
    
    // Logs d'indicateurs
    void log_indicator_value(const std::string& name, double value, int level = LogLevel::DEBUG) {
        std::string msg = "Indicateur " + name + " = " + format_value(value);
        add_log(LogCategory::INDICATOR, msg, level);
    }
    
    void log_indicator_comparison(const std::string& name, double value, 
                                  double threshold, const std::string& comparison_op, 
                                  bool result, int level = LogLevel::DEBUG) {
        std::string status = result ? "VALIDÉ" : "REJETÉ";
        std::string msg = "Indicateur " + name + " " + status + ": " + 
                         format_value(value) + " " + comparison_op + " " + 
                         format_value(threshold);
        add_log(LogCategory::INDICATOR, msg, level);
    }
    
    // Logs de filtres
    void log_filter_result(const std::string& name, bool passed, 
                          int level = LogLevel::INFO) {
        std::string status = passed ? "PASSÉ" : "REJETÉ";
        std::string msg = "Filtre " + name + ": " + status;
        add_log(LogCategory::FILTER, msg, level);
    }
    
    void log_filter_detail(const std::string& name, const std::string& detail, 
                          int level = LogLevel::DEBUG) {
        std::string msg = indent(1) + detail;
        add_log(LogCategory::FILTER, msg, level);
    }

    void log_filter_comparison(const std::string& name, double value, 
                            double threshold, const std::string& comparison_op, 
                            bool result, int level = LogLevel::DEBUG) {
        std::string status = result ? "PASSÉ" : "REJETÉ";
        std::string msg = indent(1) + "Filtre " + name + " " + status + ": " + 
                         format_value(value) + " " + comparison_op + " " + 
                         format_value(threshold);
        add_log(LogCategory::FILTER, msg, level);
    }
    
    // Logs de signaux
    void log_signal(const std::string& action, double price, 
                   double quantity, int level = LogLevel::INFO) {
        std::string msg = "Signal " + action + " généré: Prix=" + 
                         format_value(price) + ", Quantité=" + 
                         format_value(quantity);
        add_log(LogCategory::SIGNAL, msg, level);
    }
    
    void log_sl_tp(double sl_distance, double tp_distance, 
                  int level = LogLevel::INFO) {
        std::string msg = indent(1) + "SL=" + format_value(sl_distance) + 
                         ", TP=" + format_value(tp_distance);
        add_log(LogCategory::SIGNAL, msg, level);
    }
    
    // Logs d'exécution
    void log_execution_start(int level = LogLevel::INFO) {
        add_log(LogCategory::EXECUTION, "Exécution de la stratégie démarrée", level);
    }
    
    void log_execution_end(int level = LogLevel::INFO) {
        add_log(LogCategory::EXECUTION, "Exécution de la stratégie terminée", level);
    }
    
    void log_execution_step(const std::string& step, 
                          bool success, int level = LogLevel::INFO) {
        std::string status = success ? "succès" : "échec";
        std::string msg = indent(1) + "Étape '" + step + "': " + status;
        add_log(LogCategory::EXECUTION, msg, level);
    }
    
    // Logs de risque
    void log_risk_calculation(double risk_amount, double risk_percentage, 
                             int level = LogLevel::INFO) {
        std::string msg = "Risque calculé: " + format_value(risk_amount) + 
                         " (" + format_value(risk_percentage) + "% du capital)";
        add_log(LogCategory::RISK, msg, level);
    }
    
    void log_position_sizing(double raw_size, double adjusted_size, 
                            const std::string& reason, int level = LogLevel::INFO) {
        std::string msg = "Position sizing: " + format_value(raw_size) + 
                         " -> " + format_value(adjusted_size) + " (" + reason + ")";
        add_log(LogCategory::RISK, msg, level);
    }
    
    // Logs de temps
    void log_time_check(bool in_trading_hours, const std::string& detail, 
                       int level = LogLevel::INFO) {
        std::string status = in_trading_hours ? "DANS" : "HORS";
        std::string msg = status + " horaires de trading: " + detail;
        add_log(LogCategory::TIME, msg, level);
    }
    
    // Obtention de tous les logs pour la bougie actuelle
    std::string get_all_logs() const {
        std::ostringstream all_logs;
        
        // Check if there are any logs before showing the header
        // Return early if there are no logs to show
        if (general_logs.empty() && indicator_logs.empty() && filter_logs.empty() && 
            signal_logs.empty() && execution_logs.empty() && risk_logs.empty() && time_logs.empty()) {
            return "";
        }
        
        all_logs << "=== LOGS POUR " << current_candle_date.to_string() << " ===\n";
        
        if (!general_logs.empty()) {
            all_logs << "-- GENERAL --\n";
            for (const auto& log : general_logs) all_logs << log << "\n";
        }
        
        if (!indicator_logs.empty()) {
            all_logs << "-- INDICATEURS --\n";
            for (const auto& log : indicator_logs) all_logs << log << "\n";
        }
        
        if (!filter_logs.empty()) {
            all_logs << "-- FILTRES --\n";
            for (const auto& log : filter_logs) all_logs << log << "\n";
        }
        
        if (!signal_logs.empty()) {
            all_logs << "-- SIGNAUX --\n";
            for (const auto& log : signal_logs) all_logs << log << "\n";
        }
        
        if (!execution_logs.empty()) {
            all_logs << "-- EXÉCUTION --\n";
            for (const auto& log : execution_logs) all_logs << log << "\n";
        }
        
        if (!risk_logs.empty()) {
            all_logs << "-- RISQUE --\n";
            for (const auto& log : risk_logs) all_logs << log << "\n";
        }
        
        if (!time_logs.empty()) {
            all_logs << "-- TEMPS --\n";
            for (const auto& log : time_logs) all_logs << log << "\n";
        }
        
        return all_logs.str();
    }
};