import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Simple Zooming and Scrolling"
    visible: true
    modality: Qt.ApplicationModal
    width: 730
    minimumWidth: 730
    maximumWidth: 730
    height: 310
    minimumHeight: 310
    maximumHeight: 310

    Pane {
        id: leftPane
        width: 120
        padding: 5
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        Column {
            Button {
                width: 110
                contentItem: Row {
                    padding: 2; leftPadding: 5
                    Image { source: "icons/scroll_icon.png"; width:16; height:16; }
                    Text { text: "  Pointer"; font.pixelSize: 13; }
                }
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageScroll
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageScroll
            }
            Button
            {
                width: 110
                contentItem: Row {
                    padding: 2; leftPadding: 5;
                    Image { source: "icons/zoomin_icon.png"; width:16; height:16; }
                    Text { text: "  Zoom In"; font.pixelSize: 13; }
                }
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageZoomIn
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageZoomIn
            }
            Button {
                width: 110
                contentItem: Row {
                    padding: 2; leftPadding: 5
                    Image { source: "icons/zoomout_icon.png"; width:16; height:16; }
                    Text { text: "  Zoom Out"; font.pixelSize: 13; }
                }
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageZoomOut
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageZoomOut
            }
        }
    }

    QmlChartViewer
    {
        id: viewer
        anchors.left: leftPane.right
        anchors.leftMargin: 5
        y: 5
        // set default mouse usage to scroll and mouse wheel to zoom
        mouseUsage: QmlChartViewer.MouseUsageScroll
        mouseWheelZoomRatio: 1.1
        // Update chart on viewport change
        onViewPortChanged: {
            if (needUpdateChart) demo.drawChart(this);
            if (needUpdateImageMap) demo.updateImageMap(this);
        }
    }

    // The backend implementation of this demo.
    SimpleZoomScrollDemo {
        id: demo;
    }

    Component.onCompleted: {
        demo.initChartViewer(viewer);
        viewer.updateViewPort(true, true);
    }
}

