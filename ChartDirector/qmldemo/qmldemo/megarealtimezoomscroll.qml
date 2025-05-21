import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Mega Real-Time Chart with Zoom/Scroll and Track Line"
    visible: true
    modality: Qt.ApplicationModal
    width: 800
    minimumWidth: 800
    maximumWidth: 800
    height: 460
    minimumHeight: 460
    maximumHeight: 460

    Pane {
        id: buttonBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        topPadding: 4
        height: 40

        Row {
            spacing: 5
            Button {
                icon.source: "icons/scroll_icon.png"
                icon.color: "transparent"
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageScroll
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageScroll
            }
            Button {
                icon.source: "icons/zoomin_icon.png"
                icon.color: "transparent"
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageZoomIn
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageZoomIn
            }
            Button {
                icon.source: "icons/zoomout_icon.png"
                icon.color: "transparent"
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageZoomOut
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageZoomOut
            }
        }

        Button {
            id: plotChartButton
            anchors.right: parent.right
            anchors.rightMargin: 5
            width: 200
            height: 32
            text: "Plot Chart"
            onClicked: {
                instructions.visible = false;
                demo.plotChart(viewer);
                chartUpdateTimer.running = true;
            }
        }
    }

    QmlChartViewer
    {
        id: viewer
        anchors.left: parent.left
        anchors.top: buttonBar.bottom

        // Set default mouse usage to scroll and mouse wheel to zoom
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

    Text {
        id: instructions;
        topPadding: 90
        width: 500
        anchors.top: buttonBar.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        textFormat: Text.RichText
        wrapMode: Text.WordWrap
        text: "<div style='font-size:18pt;'><b>Mega Real-Time Chart</b></div>
            <div style='font-size:12pt'>This example demonstrates a chart with huge amount
            of data. We limit it to 3 lines, each with 5 million points, then 1000 points
            are added to each line per second. We limit it to 3 x 10 million points so that
            it needs less than 1G of RAM (320M to store the data, 600M to plot the data and
            handle the GUI).<br><br><br>Press the Plot Chart button to plot the chart."
    }

    ScrollBar
    {
        id: hScrollBar
        orientation: Qt.Horizontal
        height: 18
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        onPositionChanged: {
            // The scrollbar and viewport can update each others. To avoid infinite
            // loop, the scrollbar updates the viewport only if the viewport is not
            // updating the scrollbar.
            if (!viewer.isInViewPortChangedEvent) {
                // update the viewport
                viewer.viewPortLeft = this.position;
                viewer.updateViewPort(true, false);
            }
        }
    }

    // The backend implementation of this demo.
    MegaRealTimeZoomScroll {
        id: demo;
    }

    // The chart update timer
    Timer {
        id: chartUpdateTimer
        interval:100; running: false ; repeat: true
        onTriggered: {
            demo.updateChart(viewer);
            plotChartButton.text = "Point Count = " + demo.pointCount;
        }
    }
}
