import pandas as pd
import re
import matplotlib.pyplot as plt
import seaborn as sns
from typing import Dict, Tuple, Optional
import numpy as np
import holidays
import matplotlib.dates as mdates

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

def checkDataFile(
    file_path: str, 
    interval: str = '10s', 
    start_date: Optional[str] = None,
    end_date: Optional[str] = None
) -> Dict:
    """
    Check NASDAQ 100 historical OHLC data for gaps, verifying each trading day has 
    continuous data with the expected pattern: starting at market open + interval 
    and ending at market close - interval.
    
    Parameters:
    -----------
    file_path : str
        Path to the CSV file containing the OHLC data
    interval : str
        Frequency of the data (e.g., '10s', '1min', '10secs')
    start_date : str, optional
        Start date for analysis in format 'YYYY-MM-DD', if None use earliest date
    end_date : str, optional
        End date for analysis in format 'YYYY-MM-DD', if None use latest date
        
    Returns:
    --------
    dict
        A report containing analysis results including:
        - Total trading days analyzed
        - Days with gaps
        - Detailed list of gaps by date
        - Summary statistics
    """
    # Load the data
    df = pd.read_csv(file_path)
    
    # Ensure date column is datetime with timezone
    df['date'] = pd.to_datetime(df['date'])
    
    # Extract date only for grouping
    df['trading_date'] = df['date'].dt.date
    
    # Filter by date range if specified
    if start_date:
        start_date = pd.to_datetime(start_date).date()
        df = df[df['trading_date'] >= start_date]
    if end_date:
        end_date = pd.to_datetime(end_date).date()
        df = df[df['trading_date'] <= end_date]
    
    # Create US market holidays calendar
    us_market_holidays = holidays.US(years=range(df['date'].min().year, df['date'].max().year + 1))
    
    # Parse interval to seconds - improved to handle more formats
    if any(x in interval.lower() for x in ['sec', 's']):
        # Handle '10s', '10sec', '10secs', etc.
        interval_seconds = int(''.join(filter(str.isdigit, interval)))
    elif any(x in interval.lower() for x in ['min', 'm']):
        # Handle '1min', '5m', etc.
        interval_seconds = int(''.join(filter(str.isdigit, interval))) * 60
    else:
        raise ValueError(f"Unsupported interval format: {interval}")
    
    # Calculate expected number of data points for 6h30m of trading
    # Accounting for missing first and last candle
    trading_seconds = 6 * 3600 + 30 * 60  # 6 hours and 30 minutes in seconds
    expected_points = (trading_seconds // interval_seconds) - 2  # -2 for first and last candle
    
    # Initialize report
    report = {
        "analysis_summary": {
            "file_analyzed": file_path,
            "interval": interval,
            "trading_duration": "6 hours 30 minutes (excluding first and last candle)",
            "expected_points_per_day": expected_points,
            "date_range": f"{df['trading_date'].min()} to {df['trading_date'].max()}",
        },
        "total_trading_days": 0,
        "days_with_gaps": 0,
        "total_gaps": 0,
        "gaps_by_date": {},
        "missing_days": []
    }
    
    # Get all dates in range
    all_dates = pd.date_range(start=df['trading_date'].min(), end=df['trading_date'].max())
    all_trading_dates = [d.date() for d in all_dates 
                         if d.weekday() < 5 and d.date() not in us_market_holidays]
    
    # Find days present in the data
    existing_dates = set(df['trading_date'].unique())
    
    # Check for completely missing days
    missing_days = [str(d) for d in all_trading_dates if d not in existing_dates]
    report["missing_days"] = missing_days
    
    # Analyze each trading day
    for date, group in df.groupby('trading_date'):
        # Skip weekends and holidays
        weekday = pd.Timestamp(date).weekday()
        if weekday >= 5 or date in us_market_holidays:
            continue
        
        report["total_trading_days"] += 1
        
        # Sort by timestamp
        group = group.sort_values('date')
        
        # Check if we have enough data points for the day
        if len(group) < expected_points:
            report["days_with_gaps"] += 1
            report["total_gaps"] += 1
            
            report["gaps_by_date"][str(date)] = {
                "expected_points": expected_points,
                "actual_points": len(group),
                "missing_points": expected_points - len(group),
                "gaps": [f"Insufficient data points - found {len(group)}, expected {expected_points}"]
            }
            continue
        
        # Get the timestamps and check for gaps in the continuous sequence
        timestamps = group['date'].values
        
        # Check for gaps in the sequence (intervals larger than expected)
        gaps = identify_gaps(timestamps, interval_seconds)
        
        if gaps:
            report["days_with_gaps"] += 1
            report["total_gaps"] += len(gaps)
            
            # Calculate total missing points from gaps
            missing_points = sum(gap["missing_points"] for gap in gaps)
            
            report["gaps_by_date"][str(date)] = {
                "expected_points": expected_points,
                "actual_points": len(group),
                "missing_points": missing_points,
                "gaps": [f"{gap['start']} to {gap['end']} ({gap['missing_points']} points)" for gap in gaps]
            }
    
    # Add summary stats
    report["analysis_summary"]["gap_percentage"] = (
        report["days_with_gaps"] / report["total_trading_days"] * 100 
        if report["total_trading_days"] > 0 else 0
    )
    
    return report

def identify_gaps(timestamps, interval_seconds):
    """
    Identify gaps in a sequence of timestamps.
    
    Parameters:
    -----------
    timestamps : array
        Array of sorted timestamps
    interval_seconds : int
        Expected interval between consecutive timestamps in seconds
        
    Returns:
    --------
    list
        List of gap dictionaries with start time, end time, and missing points count
    """
    if len(timestamps) <= 1:
        return []
    
    gaps = []
    expected_interval = pd.Timedelta(seconds=interval_seconds)
    
    # Find gaps (jumps larger than the expected interval)
    for i in range(1, len(timestamps)):
        diff = timestamps[i] - timestamps[i-1]
        if diff > expected_interval * 1.1:  # Allow 10% tolerance
            missing_points = int(diff.total_seconds() / interval_seconds) - 1
            if missing_points > 0:  # Only count if there are actually missing points
                gaps.append({
                    "start": timestamps[i-1],
                    "end": timestamps[i],
                    "missing_points": missing_points
                })
    
    return gaps

def visualize_data_gaps(report: Dict, figsize: Tuple[int, int] = (12, 8), min_gap_threshold: int = 5):
    """
    Visualize the gaps in the data from the report generated by checkDataFile.
    
    Parameters:
    -----------
    report : dict
        The report dictionary returned by checkDataFile
    figsize : tuple
        Figure size for the plot
    min_gap_threshold : int
        Minimum number of missing points to highlight as significant gap
    """
    
    # Set up the plot
    plt.figure(figsize=figsize)
    sns.set_style("whitegrid")
    
    # Extract data for plotting
    dates = []
    missing_percentages = []
    significant_gaps = []
    
    for date_str, data in report["gaps_by_date"].items():
        date = pd.to_datetime(date_str).date()
        missing_pct = (data["missing_points"] / data["expected_points"]) * 100
        
        dates.append(date)
        missing_percentages.append(missing_pct)
        
        # Mark significant gaps
        if data["missing_points"] > min_gap_threshold:
            significant_gaps.append(date)
    
    # Sort by date
    sorted_indices = np.argsort(dates)
    dates = [dates[i] for i in sorted_indices]
    missing_percentages = [missing_percentages[i] for i in sorted_indices]
    
    # Create the plot - regular gaps
    plt.bar(dates, missing_percentages, color='lightcoral', alpha=0.5, label='All Gaps')
    
    # Highlight significant gaps
    if significant_gaps:
        significant_indices = [i for i, date in enumerate(dates) if date in significant_gaps]
        significant_percentages = [missing_percentages[i] for i in significant_indices]
        significant_dates = [dates[i] for i in significant_indices]
        plt.bar(significant_dates, significant_percentages, color='crimson', label=f'Significant Gaps (>{min_gap_threshold} points)')
    
    plt.axhline(y=5, color='orange', linestyle='--', label='5% Threshold')
    
    # Add labels and title
    plt.xlabel('Date')
    plt.ylabel('Missing Data (%)')
    plt.title('NASDAQ 100 Data Gaps Analysis')
    
    # Format x-axis
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
    plt.gca().xaxis.set_major_locator(mdates.MonthLocator(interval=2))  # Major ticks every 2 months
    plt.xticks(rotation=45)
    
    # Add summary info
    summary_text = (
        f"Total Trading Days: {report['total_trading_days']}\n"
        f"Days With Gaps: {report['days_with_gaps']} ({report['analysis_summary']['gap_percentage']:.1f}%)\n"
        f"Days With Significant Gaps: {len(significant_gaps)}\n"
        f"Total Gaps: {report['total_gaps']}"
    )
    plt.figtext(0.02, 0.02, summary_text, ha='left', fontsize=10, 
                bbox=dict(facecolor='white', alpha=0.8, boxstyle='round,pad=0.5'))
    
    plt.tight_layout()
    plt.legend()
    
    return plt.gcf()