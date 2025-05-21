import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import advsofteng.com 1.0

Window {
    title: "Zooming and Scrolling with Viewport Control"
    width: 770
    height: 480
    visible: true
    modality: Qt.ApplicationModal

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
                contentItem: Row {
                    padding: 2; leftPadding: 5
                    Image { source: "icons/save_icon.png"; width:16; height:16; }
                    Text { text: "  Save"; font.pixelSize: 12; }
                }
                onClicked: saveImageDialog.open()
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

    QmlChartViewer
    {
        x: 140
        id: viewer
        // set default mouse usage to scroll and mouse wheel to zoom
        mouseUsage: QmlChartViewer.MouseUsageScroll
        mouseWheelZoomRatio: 1.1
        // Update track cursor on mouse move
        onMouseMovePlotArea: demo.drawTrackCursor(this, chartMouseX);
        // Update chart on viewport change
        onViewPortChanged: if (needUpdateChart) demo.drawChart(this);
    }

    QmlViewPortControl
    {
        id: vpCtrl;
        anchors.top: viewer.bottom
        anchors.left: viewer.left
        anchors.right: viewer.right

        // Bind the viewport control to the viewer
        viewer: viewer;
    }

    // The backend implementation of this demo.
    ViewPortControlDemo {
        id: demo
    }

    Component.onCompleted: {
        demo.initChartViewer(viewer, vpCtrl);
        viewer.updateViewPort(true, true);
    }
}

