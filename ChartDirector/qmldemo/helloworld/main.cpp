#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "helloworld.h"


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
    // Register the sample program so that it is accessible with QML
    //
    qmlRegisterType<HelloWorld>("advsofteng.com", 1, 0, "HelloWorld");

    // Load the first and only page
    const QUrl url(QStringLiteral("qrc:/helloworld.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
