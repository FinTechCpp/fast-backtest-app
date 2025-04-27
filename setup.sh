
sudo apt update && sudo apt upgrade -y
sudo apt install -y libxcb-xinerama0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-xkb1 libxkbcommon-x11-0
if ! command -v conda &> /dev/null; then
    echo "Conda not found. Installing Miniconda..."
    mkdir -p ~/miniconda3
    wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
    bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
    rm ~/miniconda3/miniconda.sh
    source ~/miniconda3/bin/activate
    conda init --all
    source ~/.bashrc
else
    echo "Conda is already installed."
fi

if ! conda info --envs | grep -q "^trading "; then
    echo "Conda environment 'trading' not found. Creating it..."
    conda create -n trading python=3.12 -y
else
    echo "Conda environment 'trading' already exists."
fi
conda activate trading
cd ~/ig-trading-bot
git checkout develop
pip install -r requirements.txt
pip install -e .
if ! python -c "import talib" &> /dev/null; then
    echo "ta-lib not found. Installing ta-lib..."
    conda install -c conda-forge ta-lib -y
else
    echo "ta-lib is already installed."
fi

if ! command -v nvm &> /dev/null; then
    echo "nvm not found. Installing nvm..."
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.5/install.sh | bash
else
    echo "nvm is already installed."
fi

source ~/.bashrc && conda activate trading
if ! command -v nvm &> /dev/null; then
    echo "nvm not found. Installing nvm..."
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.5/install.sh | bash
    source ~/.bashrc
    nvm install --lts
else
    echo "nvm is already installed. Version: $(nvm --version)"
fi
source ~/.bashrc && conda activate trading
node -v
npm -v
cd ~/ig-trading-bot/lightweight-charts-python
pip install -e .
npm install -g npm@11.3.0
./build.sh
cd ~/ig-trading-bot
