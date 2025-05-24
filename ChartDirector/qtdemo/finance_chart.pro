QT += core gui widgets
TARGET = financechart
TEMPLATE = app

# Paths to include ChartDirector
INCLUDEPATH += ../include
LIBS += -L../lib -lchartdir

# Add rpath to find libraries at runtime
QMAKE_LFLAGS += -Wl,-rpath,../lib

# Source files
SOURCES += main.cpp \
           financechart.cpp \
           ./qtdemo/qchartviewer.cpp

# Header files
HEADERS += financechart.h \
           ./qtdemo/qchartviewer.h

# Resources
RESOURCES += resources.qrc

# Define output directory
DESTDIR = ./