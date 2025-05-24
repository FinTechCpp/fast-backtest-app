QT += quick
CONFIG += c++11

SOURCES += \
        contourcrosssection.cpp \
        contourzoomscroll.cpp \
        crosshair.cpp \
        financedemo.cpp \
        main.cpp \
        megarealtimezoomscroll.cpp \
        megazoomscroll.cpp \
        qmlchartviewer.cpp \
        qmldemo.cpp \
        democharts.cpp \
        realtimedemo.cpp \
        realtimemultichart.cpp \
        realtimetrack.cpp \
        realtimezoomscroll.cpp \
        simplezoomscroll.cpp \
        threedchartrotation.cpp \
        trackaxis.cpp \
        trackbox.cpp \
        trackfinance.cpp \
        tracklabel.cpp \
        tracklegend.cpp \
        viewportcontroldemo.cpp \
        xyzoomscroll.cpp \
        zoomscrollpdf.cpp \
        zoomscrolltrack.cpp \
        zoomscrolltrack2.cpp

RESOURCES += qmldemo.qrc

HEADERS += \
    contourcrosssection.h \
    contourzoomscroll.h \
    crosshair.h \
    financedemo.h \
    megarealtimezoomscroll.h \
    megazoomscroll.h \
    qmlchartviewer.h \
    qmldemo.h \
    democharts.h \
    realtimedemo.h \
    realtimemultichart.h \
    realtimetrack.h \
    realtimezoomscroll.h \
    simplezoomscroll.h \
    threedchartrotation.h \
    trackaxis.h \
    trackbox.h \
    trackfinance.h \
    tracklabel.h \
    tracklegend.h \
    viewportcontroldemo.h \
    xyzoomscroll.h \
    zoomscrollpdf.h \
    zoomscrolltrack.h \
    zoomscrolltrack2.h

INCLUDEPATH += ../../include

DEFINES += CHARTDIR_HIDE_OBSOLETE _CRT_SECURE_NO_WARNINGS

CONFIG += warn_off

#
# On Windows, need to link with chartdir70.lib and also copy the chartdir70.dll
# to the same directory as the compiled executable.
#
win32 {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += ../../lib64/chartdir70.lib
        QMAKE_POST_LINK = copy /Y ..\\..\\lib64\\chartdir70.dll $(DESTDIR)
    } else {
        LIBS += ../../lib32/chartdir70.lib
        QMAKE_POST_LINK = copy /Y ..\\..\\lib32\\chartdir70.dll $(DESTDIR)
    }
}

#
# On macOS, need to link with libchartdir.so. The libchartdir.so can be copied to
# the Frameworks subdirectory in the application bundle.
#
macx:LIBS += -L../../lib -lchartdir
macx:QMAKE_RPATHDIR += @executable_path/../Frameworks
macx:QMAKE_POST_LINK += mkdir -p \"`dirname $(TARGET)`/../Frameworks\";
macx:QMAKE_POST_LINK += cp ../../lib/libchartdir.7.dylib \"`dirname $(TARGET)`/../Frameworks\";

#
# On Linux, the link path and runtime path is configured assuming the libchartdir.so
# is in ../../lib.
#
unix:!macx:LIBS += -L../../lib -lchartdir
unix:!macx:QMAKE_RPATHDIR += ../../lib







