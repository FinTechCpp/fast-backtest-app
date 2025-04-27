
sudo apt update && sudo apt upgrade -y
sudo apt install -y libxcb-xinerama0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-xkb1 libxkbcommon-x11-0
mkdir -p ~/miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
rm ~/miniconda3/miniconda.sh
source ~/miniconda3/bin/activate
conda init --all
source ~/.bashrc
conda create -n trading python=3.12 -y
conda activate trading
cd ~/ig-trading-bot
git checkout develop
pip install -r requirements.txt
pip install -e .
conda install -c conda-forge ta-lib -y
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.5/install.sh | bash
source ~/.bashrc && conda activate trading
nvm --version
nvm install --lts
node -v
npm -v
cd lightweight-charts-python
pip install -e .
npm install 
npm install -g npm@11.3.0
./build.sh
cd ~/ig-trading-bot
