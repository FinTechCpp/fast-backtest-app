def merge_ohlc_dataframes(existing_df, new_df, frequency='10s'):
    """
    Merge existing and new OHLC dataframes while handling duplicates and checking for gaps.
    
    Parameters:
    -----------
    existing_df : DataFrame
        The existing historical OHLC dataframe
    new_df : DataFrame
        The new OHLC dataframe to merge
    frequency : str
        The expected frequency of the data (e.g., '10s' for 10 seconds)

    Returns:
    --------
    DataFrame
        The merged dataframe without duplicates, sorted by date
    """
    import pandas as pd
    
    # 1. Ensure date columns are datetime with timezone
    for df in [existing_df, new_df]:
        if not pd.api.types.is_datetime64_ns_dtype(df['date']):
            df['date'] = pd.to_datetime(df['date'])
    
    # 2. Combine the dataframes
    combined_df = pd.concat([existing_df, new_df])
    
    # 3. Remove duplicates based on the date column
    # Keep the first occurrence (typically from the existing data)
    combined_df = combined_df.drop_duplicates(subset=['date'], keep='first')
    
    # 4. Sort by date
    combined_df = combined_df.sort_values('date').reset_index(drop=True)
    
    # 5. Check for gaps in the data (optional but recommended)
    min_date = combined_df['date'].min()
    max_date = combined_df['date'].max()
    
    # Create a complete date range with the expected frequency
    expected_dates = pd.date_range(start=min_date, end=max_date, freq=frequency)
    
    # Find missing dates
    existing_dates = set(combined_df['date'])
    missing_dates = [date for date in expected_dates if date not in existing_dates]
    
    if missing_dates:
        print(f"Warning: Found {len(missing_dates)} gaps in the data.")
        # You could print the first few missing dates for debugging
        if len(missing_dates) > 0:
            print(f"First few missing timestamps: {missing_dates[:5]}")
    else:
        print("No gaps found in the merged data.")
    
    # Return the merged dataframe
    return combined_df

def resample_ohlc(df, interval):
    """
    Resample OHLC data to a new time interval with proper timestamp alignment.
    
    Parameters:
    -----------
    df : pandas.DataFrame
        DataFrame containing OHLC data with a 'date' column as the timestamp
    interval : str
        Target interval for resampling (e.g., '20s', '1min', '5min', '1h')
        
    Returns:
    --------
    pandas.DataFrame
        Resampled OHLC data at the specified interval, properly aligned
    """
    import pandas as pd
    import re
    
    # Make a copy to avoid modifying the original
    df_copy = df.copy()
    
    # Ensure date column is in datetime format
    if not pd.api.types.is_datetime64_ns_dtype(df_copy['date']):
        df_copy['date'] = pd.to_datetime(df_copy['date'])
    
    # Parse the interval to understand its components
    # Extract the number and unit from the interval string (e.g., '20s', '1min')
    match = re.match(r'(\d+)([a-zA-Z]+)', interval)
    if not match:
        raise ValueError(f"Invalid interval format: {interval}")
    
    interval_value = int(match.group(1))
    interval_unit = match.group(2)
    
    # Set date as index for resampling
    df_copy = df_copy.set_index('date')
    
    # Determine the offset to apply based on the interval
    if interval_unit in ['s', 'sec', 'secs', 'second', 'seconds']:
        # For seconds, ensure we start at a time that's a multiple of the interval
        # e.g., for 20s intervals, start at XX:XX:00, XX:XX:20, XX:XX:40
        offset = pd.Timedelta(seconds=interval_value)
        freq = f"{interval_value}S"
    elif interval_unit in ['min', 'mins', 'minute', 'minutes']:
        # For minutes, ensure we start at a time with 0 seconds and aligned to the interval
        # e.g., for 5min intervals, start at XX:00:00, XX:05:00, XX:10:00, etc.
        offset = pd.Timedelta(minutes=interval_value)
        freq = f"{interval_value}min"
    elif interval_unit in ['h', 'hour', 'hours']:
        # For hours, ensure we start at the hour boundary
        offset = pd.Timedelta(hours=interval_value)
        freq = f"{interval_value}H"
    else:
        # For other units, use the interval as is
        offset = None
        freq = interval
    
    # Calculate the proper start time that aligns with the interval
    min_date = df_copy.index.min()
    if offset:
        # Find the next timestamp that's aligned with the interval
        # e.g., for 20s intervals, if we start at 14:30:10, we want to start at 14:30:20
        mod_seconds = min_date.second % interval_value if interval_unit.startswith('s') else 0
        mod_minutes = min_date.minute % interval_value if interval_unit.startswith('min') else 0
        mod_hours = min_date.hour % interval_value if interval_unit.startswith('h') else 0
        
        if mod_seconds > 0 or mod_minutes > 0 or mod_hours > 0:
            # Calculate the next aligned timestamp
            if interval_unit.startswith('s'):
                aligned_start = min_date + pd.Timedelta(seconds=(interval_value - mod_seconds))
            elif interval_unit.startswith('min'):
                aligned_start = min_date.replace(second=0) + pd.Timedelta(minutes=(interval_value - mod_minutes))
            elif interval_unit.startswith('h'):
                aligned_start = min_date.replace(minute=0, second=0) + pd.Timedelta(hours=(interval_value - mod_hours))
            else:
                aligned_start = min_date
        else:
            # Already aligned
            aligned_start = min_date
    else:
        aligned_start = min_date
    
    # Get the max date
    max_date = df_copy.index.max()
    
    # Perform the resampling with the aligned frequency
    resampled = df_copy.resample(rule=freq, origin=aligned_start).agg({
        'open': 'first',    # First value in the interval
        'high': 'max',      # Maximum value in the interval
        'low': 'min',       # Minimum value in the interval
        'close': 'last'     # Last value in the interval
    })
    
    # Reset index to get date back as a column
    resampled = resampled.reset_index()
    
    # Drop rows with NaN values (which can occur if there's no data in a particular interval)
    resampled = resampled.dropna()
    
    return resampled