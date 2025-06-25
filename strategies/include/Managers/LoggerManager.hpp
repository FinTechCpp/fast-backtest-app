#pragma once
#include "common.h"
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <chrono>

enum class LogCategory {
    GENERAL,
    INDICATOR,
    FILTER,
    SIGNAL,
    EXECUTION,
    RISK,
    TIME
};

class LoggerManager {
private:
    bool enabled = true;
    LogLevel verbosity_level = LogLevel::DEBUG;
    DateTime current_candle_date;
    mutable char buffer[64];
    mutable std::string msgBuffer;
    mutable char numBuffer[64];

    // Variables pour le chronomètre
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    bool chrono_running = false;
    
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
        int len = snprintf(numBuffer, sizeof(numBuffer), "%.*f", precision, value);
        return std::string(numBuffer, len);
    }

    void append_value(std::string& str, double value, int precision = 4) const {
        int len = snprintf(numBuffer, sizeof(numBuffer), "%.*f", precision, value);
        str.append(numBuffer, len);
    }
    
    // Ajout d'un log à la catégorie appropriée
    void add_log(LogCategory category, std::string&& message, int level = LogLevel::INFO) {
        if (!enabled || level < verbosity_level)
            return;
        
        std::vector<std::string>* target_logs;
        
        // Utiliser un pointeur direct au vecteur approprié au lieu d'un switch
        switch (category) {
            case LogCategory::GENERAL:   target_logs = &general_logs; break;
            case LogCategory::INDICATOR: target_logs = &indicator_logs; break;
            case LogCategory::FILTER:    target_logs = &filter_logs; break;
            case LogCategory::SIGNAL:    target_logs = &signal_logs; break;
            case LogCategory::EXECUTION: target_logs = &execution_logs; break;
            case LogCategory::RISK:      target_logs = &risk_logs; break;
            case LogCategory::TIME:      target_logs = &time_logs; break;
        }

        target_logs->push_back(std::move(message));  // Utiliser move pour éviter une copie
    }

public:
    LoggerManager() {
        // Préallouer la mémoire pour le buffer de message
        msgBuffer.reserve(256);
        
        // Préallouer la mémoire pour les vecteurs de logs (évite les réallocations)
        general_logs.reserve(50);
        indicator_logs.reserve(50);
        filter_logs.reserve(50);
        signal_logs.reserve(20);
        execution_logs.reserve(20);
        risk_logs.reserve(20);
        time_logs.reserve(10);
    }
    
    // Configuration du logger
    void set_enabled(bool state) { enabled = state; }
    void set_verbosity(int level) { verbosity_level = static_cast<LogLevel>(level); }
    LogLevel get_verbosity() const { return verbosity_level; }
    void set_current_candle(const Candle& candle) { current_candle_date = candle.ohlc.date; }
    void clear() {
        general_logs.clear();
        indicator_logs.clear();
        filter_logs.clear();
        signal_logs.clear();
        execution_logs.clear();
        risk_logs.clear();
        time_logs.clear();
    }
    
    // Méthodes pour gérer le chronomètre
    void start_chrono() {
        start_time = std::chrono::high_resolution_clock::now();
        chrono_running = true;
    }
    
    int64_t stop_chrono_and_log() {
        if (!chrono_running) {
            log_general("Tentative d'arrêt du chronomètre alors qu'il n'est pas démarré", LogLevel::WARNING);
            return 0;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        int64_t duration_us = duration.count();
        
        // Logger le temps d'exécution
        log_execution_time(duration_us);
        
        chrono_running = false;
        return duration_us;
    }
    
    // Nouvelle méthode pour finaliser les logs et les envoyer
    void finalize_and_send_logs() {
        // Si le chronomètre est toujours en cours, l'arrêter et logger le temps
        if (chrono_running)
            stop_chrono_and_log();
        
        // Envoyer tous les logs
        cpp_log(get_all_logs(), get_verbosity());
    }
    
    // Logs généraux
    void log_general(std::string&& message, int level = LogLevel::INFO) {
        add_log(LogCategory::GENERAL, std::move(message), level);
    }
    
    // Logs d'indicateurs
    void log_indicator_value(const std::string& name, double value, int level = LogLevel::DEBUG) {
        std::string msg = "Indicateur " + name + " = " + format_value(value);
        add_log(LogCategory::INDICATOR, std::move(msg), level);
    }
    
    void log_indicator_comparison(const std::string& name, double value, double threshold, const std::string& comparison_op, bool result, int level = LogLevel::DEBUG) {
        std::string status = result ? "VALIDÉ" : "REJETÉ";
        std::string msg = "Indicateur " + name + " " + status + ": " + format_value(value) + " " + comparison_op + " " + format_value(threshold);
        add_log(LogCategory::INDICATOR, std::move(msg), level);
    }
    
    // Logs de filtres
    void log_filter_result(const std::string& name, bool passed, int level = LogLevel::INFO) {
        std::string status = passed ? "PASSÉ" : "REJETÉ";
        std::string msg = "Filtre " + name + ": " + status;
        add_log(LogCategory::FILTER, std::move(msg), level);
    }
    
    void log_filter_detail(const std::string& name, const std::string& detail, int level = LogLevel::DEBUG) {
        std::string msg = indent(1) + detail;
        add_log(LogCategory::FILTER, std::move(msg), level);
    }

    void log_filter_comparison(const std::string& name, double value, double threshold, const std::string& comparison_op, bool result, int level = LogLevel::DEBUG) {
        std::string status = result ? "PASSÉ" : "REJETÉ";
        std::string msg = indent(1) + "Filtre " + name + " " + status + ": " + format_value(value) + " " + comparison_op + " " + format_value(threshold);
        add_log(LogCategory::FILTER, std::move(msg), level);
    }
    
    // Logs de signaux
    void log_signal(const std::string& action, double price, double quantity, int level = LogLevel::INFO) {
        std::string msg = "Signal " + action + " généré: Prix=" + format_value(price) + ", Quantité=" + format_value(quantity);
        add_log(LogCategory::SIGNAL, std::move(msg), level);
    }
    
    void log_sl_tp(double sl_distance, double tp_distance, int level = LogLevel::INFO) {
        std::string msg = indent(1) + "SL=" + format_value(sl_distance) + ", TP=" + format_value(tp_distance);
        add_log(LogCategory::SIGNAL, std::move(msg), level);
    }
    
    // Logs d'exécution
    void log_execution_step(const std::string& step, bool success, int level = LogLevel::INFO) {
        std::string status = success ? "succès" : "échec";
        std::string msg = "Étape '" + step + "': " + status;
        add_log(LogCategory::EXECUTION, std::move(msg), level);
    }
    
    // Logs de risque
    void log_risk_calculation(double risk_amount, double risk_percentage, int level = LogLevel::INFO) {
        std::string msg = "Risque calculé: " + format_value(risk_amount) + " (" + format_value(risk_percentage) + "% du capital)";
        add_log(LogCategory::RISK, std::move(msg), level);
    }

    void log_position_sizing(double raw_size, double adjusted_size, const std::string& reason, int level = LogLevel::INFO) {
        std::string msg = "Position sizing: " + format_value(raw_size) + " -> " + format_value(adjusted_size) + " (" + reason + ")";
        add_log(LogCategory::RISK, std::move(msg), level);
    }
    
    // Logs de temps
    void log_time_check(bool in_trading_hours, const std::string& detail, int level = LogLevel::INFO) {
        std::string status = in_trading_hours ? "DANS" : "HORS";
        std::string msg = status + " horaires de trading: " + detail;
        add_log(LogCategory::TIME, std::move(msg), level);
    }

    // Logs de performance
    void log_execution_time(int64_t duration_us, int level = LogLevel::DEBUG) {
        std::string msg = "Temps d'exécution: " + std::to_string(duration_us) + " us";
        
        // Changer le niveau si le traitement prend trop de temps
        if (duration_us > 1000) {  // Plus de 1ms
            level = LogLevel::WARNING;
            msg += " (LENT)";
        } else if (duration_us > 500) {  // Plus de 0.5ms
            level = LogLevel::INFO;
            msg += " (Modéré)";
        }

        add_log(LogCategory::EXECUTION, std::move(msg), level);
    }
    
    // Obtention de tous les logs pour la bougie actuelle
    std::string get_all_logs() const {
        if (general_logs.empty() && indicator_logs.empty() && filter_logs.empty() && 
            signal_logs.empty() && execution_logs.empty() && risk_logs.empty() && time_logs.empty()) {
            return "";
        }

        // Estimer la taille totale requise pour éviter les réallocations
        size_t total_size = 200; // En-tête de base
        
        // Ajouter la taille estimée pour chaque section
        total_size += general_logs.size() * 50;    // Moyenne estimée par log
        total_size += indicator_logs.size() * 50;
        total_size += filter_logs.size() * 50;
        total_size += signal_logs.size() * 50;
        total_size += execution_logs.size() * 50;
        total_size += risk_logs.size() * 50;
        total_size += time_logs.size() * 50;
        
        // Pré-allouer la string finale
        std::string result;
        result.reserve(total_size);
        
        // Construire l'en-tête de la bougie directement dans la string
        result += "\n";
        result += "╔══════════════════════════════════════════════════════╗\n";
        result += "║             BOUGIE: ";
        result += current_candle_date.to_string();
        result += std::string(14, ' ');
        result += "║\n";
        result += "╚══════════════════════════════════════════════════════╝\n";

        // Construire chaque section
        auto append_section = [&result, this](const std::vector<std::string>& logs, const char* title) {
            if (logs.empty()) return;
            
            result += title;
            result += "\n";
            
            for (const auto& log : logs) {
                result += indent(1);
                result += log;
                result += "\n";
            }
        };

        append_section(general_logs, "GENERAL");
        append_section(indicator_logs, "INDICATEURS");
        append_section(filter_logs, "FILTRES");
        append_section(signal_logs, "SIGNAUX");
        append_section(execution_logs, "EXÉCUTION");
        append_section(risk_logs, "RISQUE");
        append_section(time_logs, "TEMPS");

        return result;
    }
};