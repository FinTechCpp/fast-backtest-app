QT += core widgets charts printsupport concurrent

TARGET = backtest_app_cpp
TEMPLATE = app
CONFIG += c++17

# Détecter Python de manière simple et robuste
PYTHON_INCLUDES = $$system(python3-config --includes)
PYTHON_LDFLAGS = $$system(python3-config --ldflags)
PYTHON_LIBS = $$system(python3-config --libs --embed 2>/dev/null || python3-config --libs)

# Ajouter RPATH pour ChartDirector
QMAKE_LFLAGS += -Wl,-rpath,$$PWD/../ChartDirector/lib

# Répertoires d'inclusion
INCLUDEPATH += src \
               /usr/include/pybind11 \
               ../ChartDirector/include \
               ../ChartDirector/qtdemo/qtdemo

# Ajouter les includes Python directement
QMAKE_CXXFLAGS += $$PYTHON_INCLUDES

# Librairies
LIBS += -L$$PWD/../ChartDirector/lib -lchartdir
LIBS += $$PYTHON_LDFLAGS $$PYTHON_LIBS

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