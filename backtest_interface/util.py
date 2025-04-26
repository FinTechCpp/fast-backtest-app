def to_heikin_ashi(df):
    data = df.copy()
    # Detect case
    o, h, l, c = (
        ('Open', 'High', 'Low', 'Close')
        if {'Open', 'High', 'Low', 'Close'}.issubset(data.columns)
        else ('open', 'high', 'low', 'close')
    )
    # HA close
    ha_close = (data[o] + data[h] + data[l] + data[c]) / 4.0
    # HA open
    ha_open = pd.Series(index=data.index, dtype=float)
    ha_open.iloc[0] = (data[o].iloc[0] + data[c].iloc[0]) / 2.0
    for i in range(1, len(data)):
        ha_open.iloc[i] = (ha_open.iloc[i - 1] + ha_close.iloc[i - 1]) / 2.0
    # HA high & low
    ha_high = pd.concat([data[h], ha_open, ha_close], axis=1).max(axis=1)
    ha_low = pd.concat([data[l], ha_open, ha_close], axis=1).min(axis=1)
    # Assemble result
    ha = pd.DataFrame({
        'open': ha_open,
        'high': ha_high,
        'low': ha_low,
        'close': ha_close
    }, index=data.index)
    
    # Préserver les colonnes d'indicateurs et autres métadonnées du DataFrame original
    if hasattr(data, 'attrs'):
        ha.attrs = data.attrs.copy()
    for col in data.columns:
        if col not in [o, h, l, c] and col not in ha.columns:
            ha[col] = data[col]
    
    return ha