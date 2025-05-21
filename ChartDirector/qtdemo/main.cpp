#include <QApplication>
#include <QGuiApplication>
#include <QStyleFactory>
#include "financechart.h"  // Our finance chart window class

int main(int argc, char *argv[])
{
    // Enable high DPI support
    #if QT_VERSION >= 0x050600 && QT_VERSION < 0x060000
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif
    #if QT_VERSION >= 0x051400
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    #endif

    QApplication app(argc, argv);

    // Set the fusion style to ensure the GUI looks the same in different OS
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Must set a default font size for consistent UI across platforms
    app.setStyleSheet("QWidget {font-size:12px}");

    // Create and show the finance chart window
    FinanceChartWindow financeWindow;
    financeWindow.show();
    
    return app.exec();
}