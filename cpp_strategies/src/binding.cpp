#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "strategy.hpp"
#include "buy_heikin_green.hpp"
#include "indicators.hpp"

namespace py = pybind11;

PYBIND11_MODULE(cpp_strategies, m) {
    m.doc() = "C++ Trading Strategies";

    // Expose indicators
    py::class_<EMA>(m, "CppEMA")
        .def(py::init<int>())
        .def("initialize_with_history", &EMA::initialize_with_history)
        .def("update", &EMA::update)
        .def("get_value", &EMA::get_value)
        .def_property_readonly("is_initialized", &EMA::initialized);
        
    py::class_<STOCH>(m, "CppSTOCH")
        .def(py::init<int, int, int>())
        .def("initialize_with_history", &STOCH::initialize_with_history)
        .def("update", &STOCH::update)
        .def("get_values", &STOCH::get_values)
        .def_property_readonly("is_initialized", &STOCH::initialized);
        
    py::class_<ATR>(m, "CppATR")
        .def(py::init<int>())
        .def("initialize_with_history", &ATR::initialize_with_history)
        .def("update", &ATR::update)
        .def("get_value", &ATR::get_value)
        .def_property_readonly("is_initialized", &ATR::initialized);
        
    // Expose Time structure
    py::class_<Time>(m, "CppTime")
        .def(py::init<>())
        .def_readwrite("hour", &Time::hour)
        .def_readwrite("minute", &Time::minute)
        .def_readwrite("second", &Time::second)
        .def("__lt__", [](const Time &self, const Time &other) { return self < other; })
        .def("__le__", [](const Time &self, const Time &other) { return self <= other; })
        .def("__eq__", [](const Time &self, const Time &other) { return self == other; })
        .def("__ne__", [](const Time &self, const Time &other) { return self != other; });

    // Expose DateTime structure
    py::class_<DateTime>(m, "CppDateTime")
        .def(py::init<>())
        .def_readwrite("year", &DateTime::year)
        .def_readwrite("month", &DateTime::month)
        .def_readwrite("day", &DateTime::day)
        .def_readwrite("time", &DateTime::time)
        .def("is_valid", &DateTime::is_valid)
        .def("to_string", &DateTime::to_string)
        .def("__eq__", [](const DateTime &self, const DateTime &other) { return self == other; })
        .def("__ne__", [](const DateTime &self, const DateTime &other) { return self != other; });

    // Add utility function
    m.def("parse_iso_datetime", &parse_iso_datetime);
    m.def("get_day_of_week", &get_day_of_week);

    // Expose the Candle structure
    py::class_<Candle>(m, "CppCandle")
        .def(py::init<>())
        .def_readwrite("date", &Candle::date)
        .def_readwrite("open", &Candle::open)
        .def_readwrite("high", &Candle::high)
        .def_readwrite("low", &Candle::low)
        .def_readwrite("close", &Candle::close)
        .def_readwrite("indicators", &Candle::indicators)
        .def_readwrite("in_position", &Candle::in_position)
        .def_readwrite("entry_price", &Candle::entry_price)
        .def_readwrite("position_size", &Candle::position_size)
        .def_readwrite("position_pl_pct", &Candle::position_pl_pct);

    // Expose the Signal structure
    py::class_<Signal>(m, "CppSignal")
        .def_readonly("action", &Signal::action)
        .def_readonly("quantity", &Signal::quantity)
        .def_readonly("price", &Signal::price)
        .def_readonly("take_profit", &Signal::take_profit)
        .def_readonly("stop_loss", &Signal::stop_loss)
        .def_readonly("new_sl", &Signal::new_sl);

    // Expose StrategyBaseConfig
    py::class_<StrategyBaseConfig>(m, "CppStrategyBaseConfig")
        .def(py::init<>())
        .def_readwrite("trading_from", &StrategyBaseConfig::trading_from)
        .def_readwrite("trading_to", &StrategyBaseConfig::trading_to)
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
        .def_readwrite("max_position_percentage", &StrategyBaseConfig::max_position_percentage)
        .def_readwrite("leverage_limit", &StrategyBaseConfig::leverage_limit)
        .def_readwrite("use_break_even", &StrategyBaseConfig::use_break_even)
        .def_readwrite("break_even_threshold", &StrategyBaseConfig::break_even_threshold);

    // Expose BuyHeikinGreenConfig
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

    // Expose base Strategy class as abstract
    py::class_<Strategy, std::unique_ptr<Strategy>>(m, "CppStrategy")
        .def("update_candle", &Strategy::update_candle, py::return_value_policy::reference);

    // Expose BuyHeikinGreen strategy
    py::class_<BuyHeikinGreen, Strategy>(m, "CppBuyHeikinGreen")
        .def(py::init<const StrategyBaseConfig&, const BuyHeikinGreenConfig&>());
}