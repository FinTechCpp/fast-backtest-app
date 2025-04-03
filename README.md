# IG Trading Bot

This repository contains a trading bot designed to interact with the IG Markets API. The bot is capable of executing automated trading strategies, backtesting historical data, and providing real-time market analysis. It leverages Python and several libraries to simplify interaction with the IG REST and Streaming APIs.

## Features

- **Automated Trading**: Execute trades based on predefined strategies.
- **Backtesting**: Test strategies on historical data to evaluate performance.
- **Real-Time Market Data**: Fetch and analyze live market data.
- **Customizable Strategies**: Implement your own trading strategies.
- **Rate Limiting**: Built-in support for IG API rate limits.

## Project Structure

```
ig-trading-bot/
├── Notebooks/         # Jupyter notebooks for analysis and prototyping
├── database/          # Database scripts and configurations
├── scripts/           # Core scripts for the bot and utilities
├── trading-ig/        # Documentation and configuration files
├── trading_ig/        # Python wrapper for IG Markets API
```

## Prerequisites

- Python 3.8 or higher
- An IG Markets account (Demo or Live)
- API key from IG Markets

## Installation

### Clone the Repository

```bash
git clone http://10.8.0.1:9000/finance/ig-trading-bot.git
cd ig-trading-bot
```

### Set Up the Environment

**Using pip**:
    ```bash
    pip install -r requirements.txt
    ```

### Configuration

1. copy the `.env-example` file to `.env`
2. Write your credentials in `.env`

## Usage

### Running the Bot

To run the bot in live trading mode:
```bash
python3 scripts/bot_v2.py
```

To run the bot in backtesting mode:
```bash
python3 scripts/bot_v2.py --backtest --symbol EURUSD=X --period 1y --interval 1h
```

### Available Scripts
- `scripts/main.py` : Different features to communicate with IG API
- `scripts/bot.py`: Basic trading bot implementation.
- `scripts/bot_v2.py`: Advanced trading bot with strategy customization and backtesting.

### Strategies

The bot includes two example strategies:
1. **Moving Average Strategy**: Based on moving averages and RSI.
2. **Trend Following Strategy**: Follows market trends using EMA and stochastic indicators.

You can create your own strategies by creating a new `Strategy` class in `scripts/bot_v2.py`.

## Documentation

Detailed documentation is available in the `trading-ig/docs/` folder. You can also view the online documentation [here](https://trading-ig.readthedocs.io/en/latest/).

## Contributing

Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a new branch for your feature or bugfix.
3. Submit a pull request with a detailed description of your changes.

## Contributors

We thank the following contributors for their efforts in improving this project:

- [Maxime Deville](https://github.com/maxdoe) - Strategy implementation and backtesting solution development.
- [Hugo Miquel](https://github.com/maks7d) - Project architecture, Initial IG API deployment and Machine learning bot researchs.
- [Alexandre Bianchi](https://www.linkedin.com/in/alexandre-bianchi-010652225/) - Strategies architect and trading specialist support

Want to contribute? Check out the [Contributing](#contributing) section to get started!

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Disclaimer

This bot is provided for educational purposes only. Use it at your own risk. The author is not responsible for any financial losses incurred while using this bot.

## Support

If you encounter any issues, please check the FAQ in the documentation or open an issue on GitHub.
