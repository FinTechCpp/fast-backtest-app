#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "strategy.hpp"
#include "buy_heikin_green.hpp"

namespace py = pybind11;

PYBIND11_MODULE(cpp_strategies, m) {
    m.doc() = "Stratégies de trading C++";

    // Exposer la classe Candle
    py::class_<Candle>(m, "CppCandle")
        .def(py::init<>())
        .def_readwrite("date", &Candle::date)
        .def_readwrite("open", &Candle::open)
        .def_readwrite("high", &Candle::high)
        .def_readwrite("low", &Candle::low)
        .def_readwrite("close", &Candle::close)
        .def_readwrite("indicators", &Candle::indicators);

    // Exposer la classe Signal
    py::class_<Signal>(m, "CppSignal")
        .def_readonly("action", &Signal::action)
        .def_readonly("quantity", &Signal::quantity)
        .def_readonly("price", &Signal::price)
        .def_readonly("take_profit", &Signal::take_profit)
        .def_readonly("stop_loss", &Signal::stop_loss);

    // Exposer StrategyBaseConfig
    py::class_<StrategyBaseConfig>(m, "CppStrategyBaseConfig")
        .def(py::init<>())
        .def_readwrite("trading_days", &StrategyBaseConfig::trading_days)
        .def_readwrite("take_profit_distance", &StrategyBaseConfig::take_profit_distance)
        .def_readwrite("stop_loss_distance", &StrategyBaseConfig::stop_loss_distance)
        .def_readwrite("use_atr_for_sl_tp", &StrategyBaseConfig::use_atr_for_sl_tp)
        .def_readwrite("atr_period", &StrategyBaseConfig::atr_period)
        .def_readwrite("stop_loss_atr_multiplier", &StrategyBaseConfig::stop_loss_atr_multiplier)
        .def_readwrite("take_profit_atr_multiplier", &StrategyBaseConfig::take_profit_atr_multiplier)
        .def_readwrite("min_stop_loss_distance", &StrategyBaseConfig::min_stop_loss_distance)
        .def_readwrite("min_take_profit_distance", &StrategyBaseConfig::min_take_profit_distance)
        .def_readwrite("use_risk_based_sizing", &StrategyBaseConfig::use_risk_based_sizing)
        .def_readwrite("risk_percentage", &StrategyBaseConfig::risk_percentage)
        .def_readwrite("cash", &StrategyBaseConfig::cash)
        .def_readwrite("leverage_limit", &StrategyBaseConfig::leverage_limit);

    // Exposer BuyHeikinGreenConfig
    py::class_<BuyHeikinGreenConfig>(m, "CppBuyHeikinGreenConfig")
        .def(py::init<>())
        .def_readwrite("ema_short_period", &BuyHeikinGreenConfig::ema_short_period)
        .def_readwrite("ema_long_period", &BuyHeikinGreenConfig::ema_long_period)
        .def_readwrite("stoch_fastk", &BuyHeikinGreenConfig::stoch_fastk)
        .def_readwrite("stoch_slowk", &BuyHeikinGreenConfig::stoch_slowk)
        .def_readwrite("stoch_slowd", &BuyHeikinGreenConfig::stoch_slowd)
        .def_readwrite("stoch_threshold", &BuyHeikinGreenConfig::stoch_threshold)
        .def_readwrite("use_ema_short_filter", &BuyHeikinGreenConfig::use_ema_short_filter)
        .def_readwrite("use_ema_long_filter", &BuyHeikinGreenConfig::use_ema_long_filter)
        .def_readwrite("use_stoch_filter", &BuyHeikinGreenConfig::use_stoch_filter)
        .def_readwrite("use_previous_ha_candle_red_filter", &BuyHeikinGreenConfig::use_previous_ha_candle_red_filter);

    // Exposer BuyHeikinGreen
    py::class_<BuyHeikinGreen>(m, "CppBuyHeikinGreen")
        .def(py::init<const StrategyBaseConfig&, const BuyHeikinGreenConfig&>())
        .def("update_candle", &BuyHeikinGreen::update_candle, py::return_value_policy::reference);
}