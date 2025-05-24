import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import advsofteng.com 1.0

Window {
    title: "PDF Report Demonstration"
    visible: true
    modality: Qt.ApplicationModal
    width: 780
    minimumWidth: 780
    maximumWidth: 780
    height: 375
    minimumHeight: 375
    maximumHeight: 375

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
                    Text { text: "  Pointer"; font.pixelSize: 12; }
                }
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageScroll
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageScroll
            }
            Button {
                width: 110
                contentItem: Row {
                    padding: 2; leftPadding: 5;
                    Image { source: "icons/zoomin_icon.png"; width:16; height:16; }
                    Text { text: "  Zoom In"; font.pixelSize: 12; }
                }
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageZoomIn
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageZoomIn
            }
            Button {
                width: 110
                contentItem: Row {
                    padding: 2; leftPadding: 5
                    Image { source: "icons/zoomout_icon.png"; width:16; height:16; }
                    Text { text: "  Zoom Out"; font.pixelSize: 12; }
                }
                checked: viewer.mouseUsage == QmlChartViewer.MouseUsageZoomOut
                onClicked: viewer.mouseUsage = QmlChartViewer.MouseUsageZoomOut
            }

            // Spacer
            Item {width: 1; height: 30}

            Button {
                width: 110
                height: 28
                text: "Save Chart Image"
                onClicked: saveImageDialog.open()
            }

            Button {
                width: 110
                height: 28
                text: "Create PDF Report"
                onClicked: pdfReportDialog.open()
            }
        }
    }

    FileDialog {
        id: saveImageDialog
        title: "Save"
        currentFile: "chartdirector_demo"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG (*.png);", "JPG (*.jpg)", "GIF (*.gif)", "BMP (*.bmp)", "SVG (*.svg)", "PDF (*.pdf)"]
        onAccepted: demo.saveChartImage(viewer, selectedFile)
    }

    FileDialog {
        id: pdfReportDialog
        title: "Create PDF Report"
        currentFile: "chartdirector_demo"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PDF (*.pdf)"]
        onAccepted: demo.createPdfReport(selectedFile)
    }

    // The backend implementation of this demo.
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
            // The scrollbar and viewport can update each others. To avoid infinite loop,
            // the scrollbar will update the viewport only if the viewport is not updating
            // the scrollbar.
            if (viewer.isInViewPortChangedEvent) return;

            // update the viewport
            viewer.viewPortLeft = this.position;
            viewer.updateViewPort(true, false);
        }
    }

    // The backend implementation of this demo.
    ZoomScrollPdfDemo {
        id: demo;
    }

    Component.onCompleted: {
        demo.initChartViewer(viewer);
        viewer.updateViewPort(true, true);
    }
}

