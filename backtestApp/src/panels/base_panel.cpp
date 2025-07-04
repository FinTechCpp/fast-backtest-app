#include "panels/base_panel.h"
#include "Strategies/buy_heikin_green.hpp"
#include "Strategies/sell_heikin_red.hpp"
#include "common.h"

template<>
BuyHeikinGreenConfig BasePanel::convertToConfig<BuyHeikinGreenConfig>(const QMap<QString, QVariant>& values)
{
    BuyHeikinGreenConfig config;
    
    // EMA Court
    if (values.contains("use_ema_short_filter"))
        config.use_ema_short_filter = values["use_ema_short_filter"].toBool();
    
    if (values.contains("ema_short_period"))
        config.ema_short_period = values["ema_short_period"].toInt();
    
    // EMA Long
    if (values.contains("use_ema_long_filter"))
        config.use_ema_long_filter = values["use_ema_long_filter"].toBool();
    
    if (values.contains("ema_long_period"))
        config.ema_long_period = values["ema_long_period"].toInt();
    
    // RSI
    if (values.contains("use_rsi_filter"))
        config.use_rsi_filter = values["use_rsi_filter"].toBool();
    
    if (values.contains("rsi_period"))
        config.rsi_period = values["rsi_period"].toInt();
    
    if (values.contains("rsi_threshold"))
        config.rsi_threshold = values["rsi_threshold"].toInt();
    
    // Stochastique
    if (values.contains("use_stoch_filter"))
        config.use_stoch_filter = values["use_stoch_filter"].toBool();
    
    if (values.contains("stoch_fastk"))
        config.stoch_fastk = values["stoch_fastk"].toInt();
    
    if (values.contains("stoch_slowk"))
        config.stoch_slowk = values["stoch_slowk"].toInt();
    
    if (values.contains("stoch_slowd"))
        config.stoch_slowd = values["stoch_slowd"].toInt();
    
    if (values.contains("stoch_threshold"))
        config.stoch_threshold = values["stoch_threshold"].toInt();
    
    // Filtre bougie précédente
    if (values.contains("use_previous_ha_candle_red_filter"))
        config.use_previous_ha_candle_red_filter = values["use_previous_ha_candle_red_filter"].toBool();
    
    return config;
}

template<>
SellHeikinRedConfig BasePanel::convertToConfig<SellHeikinRedConfig>(const QMap<QString, QVariant>& values)
{
    SellHeikinRedConfig config;
    
    // EMA Court
    if (values.contains("use_ema_short_filter"))
        config.use_ema_short_filter = values["use_ema_short_filter"].toBool();
    
    if (values.contains("ema_short_period"))
        config.ema_short_period = values["ema_short_period"].toInt();
    
    // EMA Long
    if (values.contains("use_ema_long_filter"))
        config.use_ema_long_filter = values["use_ema_long_filter"].toBool();
    
    if (values.contains("ema_long_period"))
        config.ema_long_period = values["ema_long_period"].toInt();
    
    // // RSI
    // if (values.contains("use_rsi_filter"))
    //     config.use_rsi_filter = values["use_rsi_filter"].toBool();
    
    // if (values.contains("rsi_period"))
    //     config.rsi_period = values["rsi_period"].toInt();
    
    // if (values.contains("rsi_threshold"))
    //     config.rsi_threshold = values["rsi_threshold"].toInt();
    
    // Stochastique
    if (values.contains("use_stoch_filter"))
        config.use_stoch_filter = values["use_stoch_filter"].toBool();
    
    if (values.contains("stoch_fastk"))
        config.stoch_fastk = values["stoch_fastk"].toInt();
    
    if (values.contains("stoch_slowk"))
        config.stoch_slowk = values["stoch_slowk"].toInt();
    
    if (values.contains("stoch_slowd"))
        config.stoch_slowd = values["stoch_slowd"].toInt();
    
    if (values.contains("stoch_threshold"))
        config.stoch_threshold = values["stoch_threshold"].toInt();
    
    // Filtre bougie précédente
    if (values.contains("use_previous_ha_candle_green_filter"))
        config.use_previous_ha_candle_green_filter = values["use_previous_ha_candle_green_filter"].toBool();
    
    return config;
}

template<>
StrategyBaseConfig BasePanel::convertToConfig<StrategyBaseConfig>(const QMap<QString, QVariant>& values)
{
    StrategyBaseConfig config;

    // Ajoutez cette ligne au début de la fonction
    if (values.contains("enable_logging"))
        config.enable_logging = values["enable_logging"].toBool();
    else
        config.enable_logging = true; // valeur par défaut
    
    // Time settings
    if (values.contains("trading_from"))
    {
        QStringList timeParts = values["trading_from"].toString().split(':');
        if (timeParts.size() == 3)
        {
            config.trading_from.hour = timeParts[0].toInt();
            config.trading_from.minute = timeParts[1].toInt();
            config.trading_from.second = timeParts[2].toInt();
        }
    }
    if (values.contains("trading_to"))
    {
        QStringList timeParts = values["trading_to"].toString().split(':');
        if (timeParts.size() == 3)
        {
            config.trading_to.hour = timeParts[0].toInt();
            config.trading_to.minute = timeParts[1].toInt();
            config.trading_to.second = timeParts[2].toInt();
        }
    }

    // Trading days
    config.trading_days.clear();
    for (int i = 0; i < 7; ++i) {
        QString key = QString("trading_day_%1").arg(i);
        if (values.contains(key) && values[key].toBool()) {
            config.trading_days.push_back(i);
        }
    }
    
    // Fixed SL/TP values
    if (values.contains("take_profit_distance"))
        config.take_profit_distance = values["take_profit_distance"].toDouble();
    if (values.contains("stop_loss_distance"))
        config.stop_loss_distance = values["stop_loss_distance"].toDouble();
    
    // Paramètres ATR pour SL et TP
    if (values.contains("use_atr_for_sl"))
        config.use_atr_for_sl = values["use_atr_for_sl"].toBool();
    else if (values.contains("sl_method"))
        config.use_atr_for_sl = (values["sl_method"].toInt() == 1); // Index 1 = ATR
    
    if (values.contains("use_atr_for_tp"))
        config.use_atr_for_tp = values["use_atr_for_tp"].toBool();
    else if (values.contains("tp_method"))
        config.use_atr_for_tp = (values["tp_method"].toInt() == 1); // Index 1 = ATR
    
    if (values.contains("atr_period"))
        config.atr_period = values["atr_period"].toInt();
    
    if (values.contains("sl_atr_multiplier"))
        config.stop_loss_atr_multiplier = values["sl_atr_multiplier"].toDouble();
    
    if (values.contains("tp_atr_multiplier"))
        config.take_profit_atr_multiplier = values["tp_atr_multiplier"].toDouble();
    
    if (values.contains("min_stop_loss_distance"))
        config.min_stop_loss_distance = values["min_stop_loss_distance"].toDouble();
    
    if (values.contains("min_take_profit_distance"))
        config.min_take_profit_distance = values["min_take_profit_distance"].toDouble();
    
    // Paramètres Min/Max pour SL
    if (values.contains("use_minmax_for_sl"))
        config.use_minmax_for_sl = values["use_minmax_for_sl"].toBool();
    else if (values.contains("sl_method"))
        config.use_minmax_for_sl = (values["sl_method"].toInt() == 2); // Index 2 = Min/Max
    
    if (values.contains("sl_minmax_periods"))
        config.sl_minmax_periods = values["sl_minmax_periods"].toInt();
    
    if (values.contains("sl_minmax_delta"))
        config.sl_minmax_delta = values["sl_minmax_delta"].toDouble();
    
    // Risk management
    if (values.contains("use_risk_based_sizing"))
        config.use_risk_based_sizing = values["use_risk_based_sizing"].toBool();
    
    if (values.contains("risk_percentage"))
        config.risk_percentage = values["risk_percentage"].toDouble();
    
    if (values.contains("cash"))
        config.cash = values["cash"].toDouble();
    
    if (values.contains("max_position_percentage"))
        config.max_position_percentage = values["max_position_percentage"].toDouble();
    
    if (values.contains("leverage_limit"))
        config.leverage_limit = values["leverage_limit"].toDouble();
    
    // Break-even parameters
    if (values.contains("use_break_even"))
        config.use_break_even = values["use_break_even"].toBool();
    
    if (values.contains("break_even_threshold"))
        config.break_even_threshold = values["break_even_threshold"].toDouble();
    
    // Perte maximale journalière
    if (values.contains("use_daily_max_loss"))
        config.use_daily_max_loss = values["use_daily_max_loss"].toBool();
    
    if (values.contains("daily_max_loss_percentage"))
        config.daily_max_loss_percentage = values["daily_max_loss_percentage"].toDouble();
    
    // Profit maximal journalier
    if (values.contains("use_daily_max_profit"))
        config.use_daily_max_profit = values["use_daily_max_profit"].toBool();
    
    if (values.contains("daily_max_profit_percentage"))
        config.daily_max_profit_percentage = values["daily_max_profit_percentage"].toDouble();
    
    // Calculer le montant de perte maximale journalière basé sur le capital
    if (config.use_daily_max_loss) {
        config.daily_max_loss_amount = config.cash * (config.daily_max_loss_percentage / 100.0);
    } else {
        config.daily_max_loss_amount = 0.0;
    }
    
    // Calculer le montant de profit maximal journalier basé sur le capital
    if (config.use_daily_max_profit) {
        config.daily_max_profit_amount = config.cash * (config.daily_max_profit_percentage / 100.0);
    } else {
        config.daily_max_profit_amount = 0.0;
    }
    
    return config;
}