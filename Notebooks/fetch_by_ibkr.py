import logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
import pandas as pd
from IPython.display import display
from ib_async import *
util.startLoop()
global historical_data_interval, duration

ib = IB()
ib.connect('127.0.0.1', 7497, clientId=14)
if ib.isConnected():
    print("✅ Connected to IBKR API")
else:
    print("❌Failed to connect to IBKR API")
util.logToConsole(logging.DEBUG)

#======================================================================
# Define the contract HERE
# For futures, you need to specify either expiry or localSymbol
contract = Future(symbol='NG', exchange='NYMEX', currency='USD', lastTradeDateOrContractMonth='202512')

ib.qualifyContracts(contract)
# Below => just some printing on contract chosen:
contract_details = ib.reqContractDetails(contract)
# Extract and display the desired fields from contract_details
filtered_details = [
    {
        "secType": detail.contract.secType,
        "conId": detail.contract.conId,
        "symbol": detail.contract.symbol,
        "exchange": detail.contract.exchange,
        "longName": detail.longName,
        "timezoneId": detail.timeZoneId,
        "tradingHours": "\n".join(
            [f"  {segment}" for segment in detail.tradingHours.split(";")]
        ),
        "liquidHours": "\n".join(
            [f"  {segment}" for segment in detail.liquidHours.split(";")]
        ),
        "minSize": detail.minSize,
    }
    for detail in contract_details
]

# Print the filtered details in a clear format
for idx, detail in enumerate(filtered_details, start=1):
    print(f"Contract Detail {idx}:")
    for key, value in detail.items():
        print(f"  {key}: {value}")
        print()
        
        
#======================================================================
# yesterday's date - UTC format for IBKR API
end_date = (pd.Timestamp.now(tz='UTC') - pd.DateOffset(days=1)).strftime('%Y%m%d-%H:%M:%S')


#======================================================================
historical_data_interval = '10 secs' # Candle period to fetch
request_duration = '3 M'  # Duration in days (use D, not "day"). Use a very big value if you want the maximum historical data, it will fetch the maximum available automatically.
price_source = 'ASK'  # 'BID', 'ASK', or 'TRADES' (note that for some symbols, (e.g. EURUSD) only 'BID' and 'ASK' are available)

bars = ib.reqHistoricalData(
        contract,
        endDateTime=end_date,
        durationStr=str(request_duration),
        barSizeSetting=str(historical_data_interval),
        whatToShow=price_source,
        useRTH=False, 
        formatDate=2,
        timeout = 0)


#======================================================================
bars[0]
new_df = util.df(bars)

display(new_df.head())
display(new_df.tail())
# Remove the 'volume', 'average', and 'barCount' columns from the DataFrame
# new_df = new_df.drop(columns=['volume', 'average', 'barCount'])
new_df = new_df.drop(columns=['average', 'barCount'])

# Display the updated DataFrame
new_df.head()


#======================================================================
from Helpers import merge_ohlc_dataframes
import os

# Determine first/last timestamps (works whether times are in a 'date' column or the index)
if 'date' in new_df.columns:
    start_date = pd.to_datetime(new_df['date'].iat[0]).strftime('%Y%m%d')
    end_date = pd.to_datetime(new_df['date'].iat[-1]).strftime('%Y%m%d')

    # Load your existing data - use index_col=0 to treat first column as index
    existing_file_path = "../marketData/NG_10secs_20230227_to_20230526_ASK.csv" # Here, enter the correct file path fo the existing csv data file
    existing_df = pd.read_csv(existing_file_path, index_col=0)
    display(existing_df.head())

    # Merge the dataframes
    merged_df = merge_ohlc_dataframes(existing_df, new_df, frequency='10s') # Adjust frequency as needed
    display(merged_df.head())
    display(merged_df.tail())

    # Save the merged dataframe
    save_path = f"../marketData/{contract.symbol}_10secs_{start_date}_to_{end_date}_{price_source}.csv" # Here, enter the correct file path for the new csv data file
    merged_df.to_csv(save_path, index=True)
    print(f"Merged data saved to: {save_path}")
    # Delete the original file if needed
    if os.path.exists(existing_file_path):
        os.remove(existing_file_path)
        print(f"Deleted original file: {existing_file_path}")
else:
    print("The 'date' column is not present in the new DataFrame.")


ib.disconnect()
