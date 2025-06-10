import pandas as pd
import re

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
        freq = f"{interval_value}s"
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

def add_index_to_csv(file_path):
    """
    Load a CSV file, add an index column, and save it back to the same file.
    
    Parameters:
    -----------
    file_path : str
        Path to the CSV file to process
    """
    
    # Load the CSV file
    df = pd.read_csv(file_path)
    
    # Add an index column starting from 0
    df.insert(0, 'index', range(len(df)))
    
    # Save back to the same file, replacing it
    df.to_csv(file_path, index=False)
    
    print(f"Added index column to {file_path} and saved successfully.")
    
def checkDataFile(file_path, interval, market_open_time, market_close_time, market_days=None):
    """
    Verify the integrity of an OHLC CSV file by checking for gaps during market hours.
    
    Parameters:
    -----------
    file_path : str
        Path to the CSV file containing OHLC data
    interval : str
        Time interval between candles (e.g., '10s', '1min', '5min')
    market_open_time : str
        Market opening time in HH:MM format (e.g., '09:30')
    market_close_time : str
        Market closing time in HH:MM format (e.g., '16:00')
    market_days : list, optional
        List of market days (0=Monday, 6=Sunday). Default is [0,1,2,3,4] (weekdays)
        
    Returns:
    --------
    dict
        A report containing integrity check results
    """
    
    # Default to weekdays if no market days specified
    if market_days is None:
        market_days = [0, 1, 2, 3, 4]  # Monday to Friday
    
    # Initialize the report
    report = {
        'file_path': file_path,
        'total_rows': 0,
        'date_range': None,
        'missing_gaps': [],
        'gaps_count': 0,
        'data_quality': {},
        'market_coverage': {},
        'errors': []
    }
    
    try:
        # Load the CSV file
        df = pd.read_csv(file_path)
        report['total_rows'] = len(df)
        
        # Check required columns
        required_columns = ['date', 'open', 'high', 'low', 'close']
        missing_columns = [col for col in required_columns if col not in df.columns]
        if missing_columns:
            report['errors'].append(f"Missing required columns: {missing_columns}")
            return report
        
        # Convert date column to datetime
        try:
            df['date'] = pd.to_datetime(df['date'])
        except Exception as e:
            report['errors'].append(f"Error parsing date column: {str(e)}")
            return report
        
        if df.empty:
            report['errors'].append("DataFrame is empty")
            return report
        
        # Sort by date
        df = df.sort_values('date').reset_index(drop=True)
        
        # Get date range
        min_date = df['date'].min()
        max_date = df['date'].max()
        report['date_range'] = {
            'start': min_date.strftime('%Y-%m-%d %H:%M:%S'),
            'end': max_date.strftime('%Y-%m-%d %H:%M:%S')
        }
        
        # Parse market hours
        try:
            open_hour, open_minute = map(int, market_open_time.split(':'))
            close_hour, close_minute = map(int, market_close_time.split(':'))
        except ValueError:
            report['errors'].append("Invalid market time format. Use HH:MM format.")
            return report
        
        # Parse interval
        match = re.match(r'(\d+)([a-zA-Z]+)', interval)
        if not match:
            report['errors'].append(f"Invalid interval format: {interval}")
            return report
        
        interval_value = int(match.group(1))
        interval_unit = match.group(2)
        
        # Convert interval to pandas frequency and calculate interval in seconds
        if interval_unit in ['s', 'sec', 'secs', 'second', 'seconds']:
            freq = f"{interval_value}s"
            interval_seconds = interval_value
        elif interval_unit in ['min', 'mins', 'minute', 'minutes']:
            freq = f"{interval_value}min"
            interval_seconds = interval_value * 60
        elif interval_unit in ['h', 'hour', 'hours']:
            freq = f"{interval_value}H"
            interval_seconds = interval_value * 3600
        else:
            report['errors'].append(f"Unsupported interval unit: {interval_unit}")
            return report
        
        # Check data quality
        report['data_quality'] = {
            'duplicates': df['date'].duplicated().sum(),
            'null_values': df.isnull().sum().to_dict(),
            'invalid_ohlc': 0
        }
        
        # Check OHLC logic (high >= low, open/close within high/low range)
        invalid_ohlc = 0
        for _, row in df.iterrows():
            if (row['high'] < row['low'] or 
                row['open'] > row['high'] or row['open'] < row['low'] or
                row['close'] > row['high'] or row['close'] < row['low']):
                invalid_ohlc += 1
        
        report['data_quality']['invalid_ohlc'] = invalid_ohlc
        
        # Generate expected timestamps during market hours
        current_date = min_date.date()
        end_date = max_date.date()
        expected_timestamps = []
        
        while current_date <= end_date:
            # Check if current date is a market day
            if current_date.weekday() in market_days:
                # Create datetime objects for market open and close
                market_open = pd.Timestamp.combine(current_date, pd.Timestamp(f"{open_hour:02d}:{open_minute:02d}").time())
                market_close = pd.Timestamp.combine(current_date, pd.Timestamp(f"{close_hour:02d}:{close_minute:02d}").time())
                
                # Add timezone if the original data has timezone
                if df['date'].dt.tz is not None:
                    market_open = market_open.tz_localize(df['date'].dt.tz)
                    market_close = market_close.tz_localize(df['date'].dt.tz)
                
                # Calculate the first candle timestamp (market_open + interval)
                first_candle_time = market_open + pd.Timedelta(seconds=interval_seconds)
                
                # Calculate the last candle timestamp (market_close - interval)
                last_candle_time = market_close - pd.Timedelta(seconds=interval_seconds)
                
                # Only generate timestamps if first_candle_time <= last_candle_time
                if first_candle_time <= last_candle_time:
                    # Generate all expected timestamps for this market day
                    day_timestamps = pd.date_range(start=first_candle_time, end=last_candle_time, freq=freq)
                    expected_timestamps.extend(day_timestamps)
            
            current_date += pd.Timedelta(days=1)
        
        # Convert to set for faster lookup
        existing_timestamps = set(df['date'])
        expected_timestamps_set = set(expected_timestamps)
        
        # Find missing timestamps within the actual data range
        data_start = min_date
        data_end = max_date
        relevant_expected = [ts for ts in expected_timestamps if data_start <= ts <= data_end]
        missing_timestamps = [ts for ts in relevant_expected if ts not in existing_timestamps]
        
        # Update report with gap information
        report['missing_gaps'] = [ts.strftime('%Y-%m-%d %H:%M:%S') for ts in sorted(missing_timestamps)]
        report['gaps_count'] = len(missing_timestamps)
        
        # Market coverage statistics
        total_expected_in_range = len(relevant_expected)
        actual_data_points = len(df)
        coverage_percentage = ((total_expected_in_range - len(missing_timestamps)) / total_expected_in_range * 100) if total_expected_in_range > 0 else 0
        
        report['market_coverage'] = {
            'expected_data_points': total_expected_in_range,
            'actual_data_points': actual_data_points,
            'coverage_percentage': round(coverage_percentage, 2),
            'market_days_checked': len([d for d in pd.date_range(min_date.date(), max_date.date()) if d.weekday() in market_days])
        }
        
    except Exception as e:
        report['errors'].append(f"Unexpected error: {str(e)}")
    
    return report