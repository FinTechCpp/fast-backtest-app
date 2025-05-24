import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Zooming and Scrolling with Track Line (1)"
    visible: true
    modality: Qt.ApplicationModal
    width: 780
    minimumWidth: 780
    maximumWidth: 780
    height: 380
    minimumHeight: 380
    maximumHeight: 380

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
        // Update track cursor on mouse move
        onMouseMovePlotArea: demo.drawTrackCursor(this, chartMouseX);
        // Update chart on viewport change. Update the scrollbar too.
        onViewPortChanged: {
            if (needUpdateChart)
                demo.drawChart(this);

            hScrollBar.size = Math.min(viewer.viewPortWidth, 0.999999999);
            hScrollBar.position = viewer.viewPortLeft;
        }
    }

    // Add scrollbar under the chart viewer
    ScrollBar
    {
        id: hScrollBar
        orientation: Qt.Horizontal
        height: 20
        anchors.bottom: parent.bottom
        anchors.left: viewer.left
        anchors.right: viewer.right

        onPositionChanged: {
            // The scrollbar and viewport can update each others. To avoid
            // infinite loop, the scrollbar will update the viewport only
            // if the viewport is not updating the scrollbar.
            if (!viewer.isInViewPortChangedEvent) {
                // update the viewport
                viewer.viewPortLeft = this.position;
                viewer.updateViewPort(true, false);
            }
        }
    }

    // The backend implementation of this demo.
    ZoomScrollTrackDemo {
        id: demo;
    }

    Component.onCompleted: {
        demo.initChartViewer(viewer);
        viewer.updateViewPort(true, true);
    }
}

