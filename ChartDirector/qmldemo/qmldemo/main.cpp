#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "qmldemo.h"
#include "qmlchartviewer.h"
#include "tracklegend.h"
#include "trackaxis.h"
#include "tracklabel.h"
#include "trackbox.h"
#include "trackfinance.h"
#include "crosshair.h"
#include "simplezoomscroll.h"
#include "zoomscrolltrack.h"
#include "zoomscrolltrack2.h"
#include "zoomscrollpdf.h"
#include "megazoomscroll.h"
#include "viewportcontroldemo.h"
#include "xyzoomscroll.h"
#include "contourzoomscroll.h"
#include "realtimedemo.h"
#include "realtimetrack.h"
#include "realtimezoomscroll.h"
#include "megarealtimezoomscroll.h"
#include "realtimemultichart.h"
#include "threedchartrotation.h"
#include "contourcrosssection.h"
#include "financedemo.h"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    //
    // Register QmlChartViewer and QmlViewPortControl to use these controls
    //
    qmlRegisterType<QmlChartViewer>("advsofteng.com", 1, 0, "QmlChartViewer");
    qmlRegisterType<QmlViewPortControl>("advsofteng.com", 1, 0, "QmlViewPortControl");

    //
    // The followings are the sample programs
    //
    qmlRegisterType<QmlDemo>("advsofteng.com", 1, 0, "QmlChartsDemo");
    qmlRegisterType<TrackLegend>("advsofteng.com", 1, 0, "TrackLegendDemo");
    qmlRegisterType<TrackAxis>("advsofteng.com", 1, 0, "TrackAxisDemo");
    qmlRegisterType<TrackLabel>("advsofteng.com", 1, 0, "TrackLabelDemo");
    qmlRegisterType<TrackBox>("advsofteng.com", 1, 0, "TrackBoxDemo");
    qmlRegisterType<TrackFinance>("advsofteng.com", 1, 0, "TrackFinanceDemo");
    qmlRegisterType<CrossHair>("advsofteng.com", 1, 0, "CrossHairDemo");
    qmlRegisterType<SimpleZoomScroll>("advsofteng.com", 1, 0, "SimpleZoomScrollDemo");
    qmlRegisterType<ZoomScrollTrack>("advsofteng.com", 1, 0, "ZoomScrollTrackDemo");
    qmlRegisterType<ZoomScrollTrack2>("advsofteng.com", 1, 0, "ZoomScrollTrack2Demo");
    qmlRegisterType<ZoomScrollPdf>("advsofteng.com", 1, 0, "ZoomScrollPdfDemo");
    qmlRegisterType<MegaZoomScroll>("advsofteng.com", 1, 0, "MegaZoomScroll");
    qmlRegisterType<ViewPortControlDemo>("advsofteng.com", 1, 0, "ViewPortControlDemo");
    qmlRegisterType<XYZoomScroll>("advsofteng.com", 1, 0, "XYZoomScroll");
    qmlRegisterType<ContourZoomScroll>("advsofteng.com", 1, 0, "ContourZoomScroll");
    qmlRegisterType<RealTimeDemo>("advsofteng.com", 1, 0, "RealTimeDemo");
    qmlRegisterType<RealTimeTrack>("advsofteng.com", 1, 0, "RealTimeTrack");
    qmlRegisterType<RealTimeZoomScroll>("advsofteng.com", 1, 0, "RealTimeZoomScroll");
    qmlRegisterType<MegaRealTimeZoomScroll>("advsofteng.com", 1, 0, "MegaRealTimeZoomScroll");
    qmlRegisterType<RealTimeMultiChart>("advsofteng.com", 1, 0, "RealTimeMultiChart");
    qmlRegisterType<ThreeDChartRotation>("advsofteng.com", 1, 0, "ThreeDChartRotation");
    qmlRegisterType<ContourCrossSection>("advsofteng.com", 1, 0, "ContourCrossSection");
    qmlRegisterType<FinanceDemo>("advsofteng.com", 1, 0, "FinanceDemo");

    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
