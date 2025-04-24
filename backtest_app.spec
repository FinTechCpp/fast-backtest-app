import sys
import os
from PyInstaller.utils.hooks import collect_all, collect_submodules
import bokeh

# Base directory of your project
base_path = os.path.abspath(os.getcwd())

# Collect all files and submodules for each folder
backtest_interface = collect_all(os.path.join(base_path, 'backtest_interface'))
igtrader = collect_all(os.path.join(base_path, 'igtrader'))
lightweight_charts = collect_all(os.path.join(base_path, 'lightweight-charts-python'))
market_data = collect_all(os.path.join(base_path, 'marketData'))
bokeh_files = collect_all(os.path.dirname(bokeh.__file__))

# Combine all collected data
datas = (
    backtest_interface[0] +
    igtrader[0] +
    lightweight_charts[0] +
    market_data[0] +
    bokeh_files[0]
)

# Collect all submodules for key packages
hiddenimports = (
    collect_submodules('igtrader') +
    collect_submodules('bokeh') +
    collect_submodules('lightweight_charts') +
    collect_submodules('dotenv') +
    collect_submodules('PyQt5')
)

block_cipher = None

a = Analysis(
    ['backtest_interface/app_pyqt.py'],
    pathex=[base_path],
    binaries=[],
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    runtime_hooks=[],
    excludes=[],   # no excludes: include everything
    cipher=block_cipher,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='backtest_app',
    debug=False,
    strip=False,
    upx=False,
    console=True,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=False,
    name='backtest_app',
)