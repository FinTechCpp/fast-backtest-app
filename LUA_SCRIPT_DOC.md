# Lua Scripting Documentation

This document explains what your Lua script can use in Fast Backtest App.

## 1) Mandatory function

The engine calls this function on each new candle:

```lua
function on_candle(candle, position)
    -- return nil, a string, or a signal table
end
```

If `on_candle` is absent, the script is rejected at initialization.

## 2) Available Data

### Parameter `candle`

Available Fields:

- `candle.open`
- `candle.high`
- `candle.low`
- `candle.close`
- `candle.year`
- `candle.month`
- `candle.day`
- `candle.hour`
- `candle.minute`
- `candle.second`

### Parameter `position`

Available Fields:

- `position.is_open`
- `position.entry_price`
- `position.take_profit_price`
- `position.closed_trade_pnl`

## 3) Lua Helpers available

- `get_candle(offset)`
  - `offset = 0` -> current candle
  - `offset = 1` -> previous candle
  - returns `nil` if out of buffer
- `candles_count()`
  - returns the number of candles currently in memory
- `get_position()`
  - returns a table equivalent to `position`
- `log(message)`
  - writes a debug log with prefix `[Lua]`
- `set_required_history(count)`
  - sets the number of past candles kept in memory for the script (default: 200). Call this outside of `on_candle()`.

## 4) Signal Types

Constants availables in `SignalType`:

- `SignalType.NONE`
- `SignalType.BUY`
- `SignalType.SELL`
- `SignalType.LIQUIDATE`
- `SignalType.MOVE_SL`

You can retrn from `on_candle`:

- `nil` -> no signal
- a string -> e.g., `"BUY"`, `"SELL"`, `"LIQUIDATE"`, `"CLOSE"`, `"MOVE_SL"`, `"MOVE_STOP"`
- a signal table

### Table signal (supported fields)

```lua
{
  type = SignalType.BUY,   -- mandatory (or equivalent string)
  quantity = 1.0,          -- optional
  price = 1.2345,          -- optional
  take_profit = 1.2400,    -- optional
  stop_loss = 1.2300,      -- optional
  new_sl = 1.2330          -- for MOVE_SL
}
```

## 5) Engine behavior rules

- The time/day check is executed before the Lua script.
- The automatic break-even is executed before the Lua script.
- For `BUY`/`SELL`, the engine calculates a base (quantity/price/SL/TP) then replaces each field only if your value is `> 0`.
- For `LIQUIDATE`, if `quantity <= 0`, it is forced to `1.0` (complete closure).
- `quantity` in `]0,1[` on `LIQUIDATE` allows for partial closure.

## 6) Basic example

Objective:

- Enter a buy position on a green candle if no position is open.
- Exit on a red candle if a position is open.

```lua
function on_candle(candle, position)
  -- Determine candle color from OHLC values.
  -- Green candle: bullish close above open.
    local is_green = candle.close > candle.open
  -- Red candle: bearish close below open.
    local is_red = candle.close < candle.open

  -- ENTRY RULE
  -- Open a long position only if we are flat (no open position)
  -- and the current candle is green.
    if (not position.is_open) and is_green then
    -- Returning a table gives flexibility to add quantity/SL/TP later.
        return { type = SignalType.BUY }
    end

  -- EXIT RULE
  -- If a position is currently open and the candle turns red,
  -- close the position.
    if position.is_open and is_red then
    -- LIQUIDATE without quantity means full closure
    -- (engine defaults quantity to 1.0).
        return { type = SignalType.LIQUIDATE }
    end

  -- No signal on this candle.
  -- Returning nil explicitly means "do nothing".
    return nil
end
```

## 7) Advanced example (with custom indicator code)

Objective:

- Build a custom SMA indicator in Lua.
- Open a position on a bullish SMA crossover.
- Close the position on a bearish SMA crossover.

```lua
-- Simple Moving Average indicator on candle.close.
-- offset = 0 -> current candle, offset = 1 -> previous candle, etc.
local function sma(period, offset)
  -- Defensive check: invalid period means no value.
  if period <= 0 then
    return nil
  end

  -- We need at least (offset + period) candles in memory to compute the SMA.
  -- Example: period=20 and offset=1 requires at least 21 candles.
  if candles_count() < (offset + period) then
    return nil
  end

  -- Compute average of candle.close over [offset, offset + period - 1].
  local sum = 0.0
  for i = offset, offset + period - 1 do
    -- Read candle i bars ago.
    local c = get_candle(i)
    -- If history is unexpectedly missing, abort computation safely.
    if c == nil then
      return nil
    end
    -- Accumulate closing prices.
    sum = sum + c.close
  end

  -- Return arithmetic mean.
  return sum / period
end

-- Strategy parameters (tune these to change sensitivity).
local FAST_PERIOD = 5
local SLOW_PERIOD = 20

function on_candle(candle, position)
  -- Warm-up guard:
  -- We need previous and current slow SMA, hence (SLOW_PERIOD + 1).
  -- Ensure enough history for current and previous values.
  if candles_count() < (SLOW_PERIOD + 1) then
    return nil
  end

  -- Compute current/previous fast SMA values.
  local fast_now = sma(FAST_PERIOD, 0)
  local fast_prev = sma(FAST_PERIOD, 1)
  -- Compute current/previous slow SMA values.
  local slow_now = sma(SLOW_PERIOD, 0)
  local slow_prev = sma(SLOW_PERIOD, 1)

  -- Any missing value means we skip this candle.
  if (not fast_now) or (not fast_prev) or (not slow_now) or (not slow_prev) then
    return nil
  end

  -- Bullish crossover:
  -- Fast SMA was below/at slow SMA and is now above it.
  local bullish_cross = (fast_prev <= slow_prev) and (fast_now > slow_now)
  -- Bearish crossover:
  -- Fast SMA was above/at slow SMA and is now below it.
  local bearish_cross = (fast_prev >= slow_prev) and (fast_now < slow_now)

  -- ENTRY: open long only when flat and bullish crossover appears.
  if (not position.is_open) and bullish_cross then
    -- Helpful trace in logs for debugging strategy decisions.
    log("Bullish SMA cross -> BUY")
    return {
      -- Mandatory field.
      type = SignalType.BUY,
      -- 1.0 means 100% of the configured position size.
      quantity = 1.0
    }
  end

  -- EXIT: close open position on bearish crossover.
  if position.is_open and bearish_cross then
    -- Helpful trace in logs for debugging strategy decisions.
    log("Bearish SMA cross -> LIQUIDATE")
    return {
      -- Full exit signal.
      type = SignalType.LIQUIDATE,
      -- Explicit full close.
      quantity = 1.0
    }
  end

  -- Neither entry nor exit condition was met.
  return nil
end
```

## 8) Practical Tips

- Start simple (`BUY` / `LIQUIDATE`) then add `quantity`, `stop_loss`, `take_profit`.
- Use `log("...")` to verify your logic in the logs.
- Check the trading hours/days in the profile if you don't see any trades.