QT += core widgets charts printsupport concurrent

TARGET = backtest_app_cpp
TEMPLATE = app
CONFIG += c++17

# Obtenir les chemins Python dynamiquement (Conda)
PYTHON_INCLUDES = $$system(python3-config --includes | sed 's/-I//g')
PYTHON_LDFLAGS = $$system(python3-config --ldflags)
PYTHON_LIBS = $$system(python3-config --libs)

# Extraire TOUS les chemins de librairies Python
PYTHON_LIB_DIRS = $$system(python3-config --ldflags | grep -o '\-L[^ ]*' | sed 's/-L//')

# Ajouter RPATH pour ChartDirector ET Python (version explicite)
QMAKE_LFLAGS += -Wl,-rpath,$$PWD/../ChartDirector/lib
QMAKE_LFLAGS += -Wl,-rpath,/home/hugo/miniconda3/envs/IGTradingBot/lib
QMAKE_LFLAGS += -Wl,-rpath,/home/hugo/miniconda3/envs/IGTradingBot/lib/python3.11/config-3.11-x86_64-linux-gnu

# Répertoires d'inclusion
INCLUDEPATH += src \
               /usr/include/pybind11 \
               $$PYTHON_INCLUDES \
               ../ChartDirector/include \
               ../ChartDirector/qtdemo/qtdemo

# Librairies avec support Conda complet
LIBS += -L$$PWD/../ChartDirector/lib -lchartdir \
        $$PYTHON_LDFLAGS \
        $$PYTHON_LIBS \
        -lpython3.11

# Sources
SOURCES += src/main.cpp \
           src/app.cpp \
           src/config_manager.cpp \
           src/metric_widget.cpp \
           src/binding/pybinding.cpp \
           src/views/baseview.cpp \
           src/views/stats_view.cpp \
           src/views/histogram_view.cpp \
           src/views/chart_view.cpp \
           src/views/result_manager.cpp \
           src/panels/base_panel.cpp \
           src/panels/general_params_panel.cpp \
           src/panels/strategy_base_panel.cpp \
           src/panels/profile_panel.cpp \
           src/panels/strategy_specific_panels/buy_heikin_green_panel.cpp \
           src/panels/strategy_specific_panels/sell_heikin_red_panel.cpp \
           src/panels/strategy_specific_panels/cross_ema_panel.cpp \
           src/components/backtest_runner.cpp \
           src/data_loader.cpp \
           ../ChartDirector/qtdemo/qtdemo/qchartviewer.cpp

# Headers avec Q_OBJECT
HEADERS += src/app.h \
           src/config_manager.h \
           src/metric_widget.h \
           src/binding/pybinding.h \
           src/views/baseview.h \
           src/views/stats_view.h \
           src/views/histogram_view.h \
           src/views/chart_view.h \
           src/views/result_manager.h \
           src/panels/base_panel.h \
           src/panels/general_params_panel.h \
           src/panels/strategy_base_panel.h \
           src/panels/profile_panel.h \
           src/panels/strategy_specific_panels/buy_heikin_green_panel.h \
           src/panels/strategy_specific_panels/cross_ema_panel.h \
           src/panels/strategy_specific_panels/sell_heikin_red_panel.h \
           src/components/backtest_runner.h \
           src/data_loader.h \
           ../ChartDirector/qtdemo/qtdemo/qchartviewer.h \
           ../ChartDirector/include/chartdir.h \
           ../ChartDirector/include/FinanceChart.h

# Configuration pour la détection automatique des MOC files
CONFIG += moc
QMAKE_MOC_OPTIONS += -DMOC_PARSING