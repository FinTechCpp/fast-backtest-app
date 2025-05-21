import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Hello World"
    visible: true
    width: 270
    minimumWidth: 270
    maximumWidth: 270
    height: 270
    minimumHeight: 270
    maximumHeight: 270

    // The QML Chart Viewer
    QmlChartViewer
    {
        id: viewer
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
    }

    // The implementation of this example.
    HelloWorld {
        id: demo
    }

    // Draw chart after QML loaded
    Component.onCompleted: {
        demo.drawChart(viewer);
    }
}
