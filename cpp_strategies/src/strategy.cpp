#include "strategy.h"



// Fonction utilitaire pour parser une chaîne de date ISO
DateTime parse_iso_datetime(const std::string& iso_date) {
    DateTime result;
    
    // Vérification de la longueur minimale
    if (iso_date.size() < 19) {
        return result;  // Return invalid date
    }
    
    std::tm tm = {};
    std::istringstream ss(iso_date.substr(0, 19));
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        return result;  // Return invalid date
    }
    
    result.year = tm.tm_year + 1900;  // tm_year est années depuis 1900
    result.month = tm.tm_mon + 1;     // tm_mon est 0-11
    result.day = tm.tm_mday;
    result.time.hour = tm.tm_hour;
    result.time.minute = tm.tm_min;
    result.time.second = tm.tm_sec;
    
    return result;
}

// Fonction pour obtenir le jour de la semaine (0=lundi, 6=dimanche)
int get_day_of_week(const DateTime& date) {
    // Formule pour calculer le jour de la semaine
    std::tm timeinfo = {};
    timeinfo.tm_year = date.year - 1900;
    timeinfo.tm_mon = date.month - 1;
    timeinfo.tm_mday = date.day;
    
    std::time_t time = std::mktime(&timeinfo);
    std::tm* local_tm = std::localtime(&time);
    int weekday = local_tm->tm_wday;
    
    // Convertir de Sunday=0 à Sunday=6
    return (weekday == 0) ? 6 : weekday - 1;
}
    

// Implementation of Strategy class methods
// Calcule le risque potentiel d'un trade en valeur monétaire
double Strategy::calculate_trade_risk(bool is_long) {
    double position_value;
    double risk_value;
    
    if (is_long) {
        position_value = buy_quantity * buy_price;
        risk_value = position_value * (stop_loss_distance / buy_price);
    } else {
        position_value = sell_quantity * sell_price;
        risk_value = position_value * (stop_loss_distance / sell_price);
    }
    
    return risk_value;
}

// Vérifie si un trade est acceptable en termes de risque quotidien
bool Strategy::is_trade_risk_acceptable(double risk) {
    if (!base_config.use_daily_max_loss) {
        return true;  // Si la limite n'est pas activée, tous les trades sont acceptables
    }
    
    // Calculer la limite de perte quotidienne
    double max_loss_amount = base_config.cash * base_config.daily_max_loss_percentage / 100.0;
    
    // Vérifier si le trade nous ferait dépasser la limite
    // daily_pnl est le PnL cumulé jusqu'à présent, risk est le montant maximum que nous pourrions perdre
    return (daily_pnl - risk) >= -max_loss_amount;
}

bool Strategy::is_new_trading_day() {
    if (!candle_manager.get_latest_candle().date.is_valid() || !current_trading_day.is_valid()) {
        return true;
    }
    
    return (candle_manager.get_latest_candle().date.year != current_trading_day.year ||
            candle_manager.get_latest_candle().date.month != current_trading_day.month ||
            candle_manager.get_latest_candle().date.day != current_trading_day.day);
}
    
void Strategy::update_daily_pnl_tracking() {
    if (!base_config.use_daily_max_loss) {
        return;
    }
    
    // Si c'est un nouveau jour, on réinitialise le compteur et on réactive le trading
    if (is_new_trading_day()) {
        current_trading_day = candle_manager.get_latest_candle().date;
        daily_pnl = 0.0;
        
        // Calculer le montant maximum de perte autorisé pour cette journée
        double max_loss_amount = base_config.cash * base_config.daily_max_loss_percentage / 100.0;
        
        logger->log_general("Nouveau jour de trading: " + current_trading_day.to_string() + 
                          " - Perte max autorisée: " + std::to_string(max_loss_amount) + 
                          " (" + std::to_string(base_config.daily_max_loss_percentage) + "%)", LogLevel::INFO);
    }
    
    if (last_trade_pnl != 0.0) {
        daily_pnl += last_trade_pnl;
        
        logger->log_general("P&L du trade: " + std::to_string(last_trade_pnl) + 
                          " - P&L journalier cumulé: " + std::to_string(daily_pnl), LogLevel::INFO);
        
        last_trade_pnl = 0.0;
    }
}

// Méthode pour vérifier si on est dans les horaires de trading
bool Strategy::check_time() {
    if (!candle_manager.get_latest_candle().date.is_valid()) {
        // Utiliser log_time_check avec false pour indiquer qu'on est hors horaires
        logger->log_time_check(false, "Date de bougie invalide", LogLevel::WARNING);
        return false;
    }
    
    // Vérifier si la date a changé depuis la dernière vérification
    if (candle_manager.get_latest_candle().date != last_check_date) {
        last_check_date = candle_manager.get_latest_candle().date;
        
        // Calculer le jour de la semaine (0=lundi, 6=dimanche)
        int weekday = get_day_of_week(candle_manager.get_latest_candle().date);
        
        // Vérifier si c'est un jour de trading
        weekday_check = std::find(base_config.trading_days.begin(), 
                                  base_config.trading_days.end(), 
                                  weekday) != base_config.trading_days.end();
        
        if (!weekday_check) {
            logger->log_time_check(false, "Jour non autorisé pour le trading: " + 
                                 candle_manager.get_latest_candle().date.to_string(), LogLevel::INFO);
            return false;
        }
        
        // Vérifier les heures de trading
        const Time& current_time = candle_manager.get_latest_candle().date.time;
        
        bool after_start = (base_config.trading_from < current_time || 
                            base_config.trading_from == current_time);
                            
        bool before_end = (current_time < base_config.trading_to || 
                           current_time == base_config.trading_to);
                           
        time_check = after_start && before_end;
        
        if (!time_check) {
            logger->log_time_check(false, 
                                 std::to_string(current_time.hour) + ":" + 
                                 std::to_string(current_time.minute), LogLevel::INFO);
        } else {
            // Ajouter un message positif quand on est dans les heures de trading
            logger->log_time_check(true, 
                                 std::to_string(current_time.hour) + ":" +
                                 std::to_string(current_time.minute), LogLevel::DEBUG);
        }
        
        return time_check;
    }
    
    // Utiliser le résultat mis en cache
    return weekday_check && time_check;
}

std::unique_ptr<Signal> Strategy::check_break_even() {
    if (!in_position || !base_config.use_break_even) {
        return nullptr;
    }
    
    if (entry_price == 0.0 || position_pl_pct == 0.0) {
        return nullptr;
    }
    
    // Calculate threshold based on take profit distance
    double threshold_pct = (base_config.take_profit_distance / entry_price) * 100.0;
    
    // Check if we've reached the threshold to activate break-even
    if (position_pl_pct > (base_config.break_even_threshold * threshold_pct)) {
        logger->log_general("Activation break-even: P&L = " + std::to_string(position_pl_pct) + 
                          "% > seuil (" + std::to_string(base_config.break_even_threshold * threshold_pct) + 
                          "%)", LogLevel::INFO);
                
        auto be_signal = std::make_unique<Signal>();
        be_signal->action = "MOVE_SL";
        be_signal->new_sl = entry_price;
        return be_signal;
    }
    
    return nullptr;
}

std::unique_ptr<Signal> Strategy::generate_buy_signal() {
    auto sig = std::make_unique<Signal>();
    sig->action = "BUY";
    sig->quantity = buy_quantity;
    sig->price = buy_price;
    sig->take_profit = take_profit_distance;
    sig->stop_loss = stop_loss_distance;
    return sig;
}

std::unique_ptr<Signal> Strategy::generate_sell_signal() {
    auto sig = std::make_unique<Signal>();
    sig->action = "SELL";
    sig->quantity = sell_quantity;
    sig->price = sell_price;
    sig->take_profit = take_profit_distance;
    sig->stop_loss = stop_loss_distance;
    return sig;
}

std::unique_ptr<Signal> Strategy::generate_liquidation_signal() {
    auto sig = std::make_unique<Signal>();
    sig->action = "LIQUIDATE";
    return sig;
}

void Strategy::reset() {
    buy_quantity = 0.0;
    buy_price = 0.0;
    sell_quantity = 0.0;
    sell_price = 0.0;
    take_profit_distance = 0.0;
    stop_loss_distance = 0.0;
    signal = nullptr;
}

void Strategy::execute_long() {
    go_long();
    
    if (buy_quantity <= 0.0 || buy_price <= 0.0) {
        logger->log_general("Paramètres d'achat incorrects", LogLevel::ERROR);
        throw std::runtime_error("Buy parameters not properly set");
    }
    
    // Calculer le risque et vérifier s'il est acceptable
    double risk = calculate_trade_risk(true);
    logger->log_risk_calculation(risk, (risk / base_config.cash) * 100.0);
    
    if (base_config.use_daily_max_loss && !is_trade_risk_acceptable(risk)) {
        // Le trade est trop risqué par rapport à notre limite quotidienne
        double max_loss_amount = base_config.cash * base_config.daily_max_loss_percentage / 100.0;
        
        logger->log_general("Trade LONG rejeté: risque excessif", LogLevel::WARNING);
        logger->log_filter_detail("Limite de risque", 
                              "Risque calculé: " + std::to_string(risk) + 
                              ", PnL journalier: " + std::to_string(daily_pnl) + 
                              ", Limite max: " + std::to_string(-max_loss_amount), 
                              LogLevel::INFO);
        reset();
        return;
    }
    
    logger->log_signal("BUY", buy_price, buy_quantity);
    logger->log_sl_tp(stop_loss_distance, take_profit_distance);
            
    signal = generate_buy_signal();
}

void Strategy::execute_short() {
    go_short();
    
    if (sell_quantity <= 0.0 || sell_price <= 0.0) {
        logger->log_general("Paramètres de vente incorrects", LogLevel::ERROR);
        throw std::runtime_error("Sell parameters not properly set");
    }
    
    // Calculer le risque et vérifier s'il est acceptable
    double risk = calculate_trade_risk(false);
    logger->log_risk_calculation(risk, (risk / base_config.cash) * 100.0);
    
    if (base_config.use_daily_max_loss && !is_trade_risk_acceptable(risk)) {
        // Le trade est trop risqué par rapport à notre limite quotidienne
        double max_loss_amount = base_config.cash * base_config.daily_max_loss_percentage / 100.0;
        
        logger->log_general("Trade SHORT rejeté: risque excessif", LogLevel::WARNING);
        logger->log_filter_detail("Limite de risque", 
                              "Risque calculé: " + std::to_string(risk) + 
                              ", PnL journalier: " + std::to_string(daily_pnl) + 
                              ", Limite max: " + std::to_string(-max_loss_amount), 
                              LogLevel::INFO);
        reset();
        return;
    }
    
    logger->log_signal("SELL", sell_price, sell_quantity);
    logger->log_sl_tp(stop_loss_distance, take_profit_distance);
    
    signal = generate_sell_signal();
}

// filepath: /home/max/ig-trading-bot/cpp_strategies/src/strategy.cpp
bool Strategy::execute_filters() {
    auto all_filters = filters();
    logger->log_general("Exécution de " + std::to_string(all_filters.size()) + " filtres", LogLevel::DEBUG);
    
    bool all_passed = true;
    
    for (size_t i = 0; i < all_filters.size(); ++i) {
        bool filter_passed = all_filters[i]();
        if (!filter_passed) {
            logger->log_general("Filtre #" + std::to_string(i) + " échoué", LogLevel::INFO);
            all_passed = false;
        } else {
            logger->log_general("Filtre #" + std::to_string(i) + " passé", LogLevel::DEBUG);
        }
    }
    
    return all_passed;
}

void Strategy::execute() {
    if (is_executing) {
        logger->log_execution_step("Exécution déjà en cours", false);
        return;
    }
    
    is_executing = true;
    
    // Mise à jour du suivi des pertes journalières
    update_daily_pnl_tracking();
    
    // Quick time check before executing anything else
    if (!check_time()) {
        logger->log_execution_step("Vérification horaires", false);
        if (in_position) {
            logger->log_general("Hors horaires de trading - Liquidation de position", LogLevel::INFO);
            signal = generate_liquidation_signal();
        }
        is_executing = false;
        return;
    }
    logger->log_execution_step("Vérification horaires", true);
    
    before();
    
    bool should_long_val = should_long();
    bool should_short_val = should_long_val ? false : should_short();
    
    if (should_long_val) {
        logger->log_execution_step("should_long()", true);
    } else if (should_short_val) {
        logger->log_execution_step("should_short()", true);
    } else {
        logger->log_execution_step("Conditions d'entrée", false);
        reset();
        is_executing = false;
        return;
    }
    
    if (!execute_filters()) {
        logger->log_execution_step("Filtres", false);
        logger->log_general("Filtres non passés - Pas de signal généré", LogLevel::INFO);
        reset();
        is_executing = false;
        return;
    }
    logger->log_execution_step("Filtres", true);
    
    if (should_long_val) {
        execute_long();
    } else {
        execute_short();
    }
    
    after();
    logger->log_execution_step("After()", true);
    is_executing = false;
}


Strategy::Strategy(const StrategyBaseConfig& config) 
    : base_config(config), 
    signal(std::make_unique<Signal>()),
    logger(std::make_unique<StrategyLogger>()) {

}

// Main update method
Signal* Strategy::update_candle(const Candle& candle) {    

    // TODO: C'est probablement a supprimer cela sert juste dans les stratégies de fille pour avoir les information extra (in_position, entry_price, position_size, position_pl_pct)
    current_candle = candle;

    // Mettre à jour le logger avec la bougie actuelle
    logger->set_current_candle(candle);
    logger->clear();  // Vider les logs précédents
    
    logger->log_general("Traitement bougie: " + candle.date.to_string() + 
            " OHLC: " + std::to_string(candle.open) + "/" + 
            std::to_string(candle.high) + "/" + 
            std::to_string(candle.low) + "/" + 
            std::to_string(candle.close), LogLevel::INFO);
    
    // Store the last trade P&L si fourni dans candle
    if (candle.closed_trade_pnl != 0.0) {
        last_trade_pnl = candle.closed_trade_pnl;
        logger->log_general("PnL du trade fermé: " + std::to_string(last_trade_pnl), LogLevel::INFO);
    }
    
    // Update position information
    in_position = candle.in_position;
    entry_price = candle.entry_price;
    position_size = candle.position_size;
    position_pl_pct = candle.position_pl_pct;

    if (in_position) {
        logger->log_general("En position: Prix d'entrée=" + std::to_string(entry_price) + 
                          ", Taille=" + std::to_string(position_size) + 
                          ", P&L=" + std::to_string(position_pl_pct) + "%", LogLevel::INFO);
    }
    
    // Add to buffer for historical calculations
    BasicCandle basic_candle(candle.date, candle.open, candle.high, candle.low, candle.close);
    candle_manager.add_candle(basic_candle);

    
    // Check for break-even signal before executing strategy
    auto be_signal = check_break_even();
    if (be_signal) {
        logger->log_general("Signal de break-even généré: " + 
                          std::to_string(be_signal->new_sl), LogLevel::INFO);
        signal = std::move(be_signal);
        return signal.get();
    }
    
    // Execute strategy
    logger->log_execution_start();
    execute();
    logger->log_execution_end();

    // Obtenir tous les logs complets et les envoyer dans un seul message de log
    cpp_log(logger->get_all_logs(), logger->get_verbosity());
    
    return signal.get();
}

// Properties
double Strategy::price() const {
    return candle_manager.get_latest_candle().close;
}

