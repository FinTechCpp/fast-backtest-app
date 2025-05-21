import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import advsofteng.com 1.0

Window {
    title: "Contour Plot Zoom/Scroll"
    width: 680
    height: 500
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
                    padding: 2; leftPadding: 5
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

            // Spacer
            Item {width: 1; height: 50}

            Text {
                width: 100
                bottomPadding: 4
                text: "Zoom Level"
                horizontalAlignment: Text.AlignHCenter
            }
            Slider {
                id: zoomSlider
                width: 110
                onValueChanged: {
                    // The slider and viewport can update each others. To avoid infinite loop,
                    // the slider will update the viewport only if the viewport is not updating
                    // the slider.
                    if (viewer.isInViewPortChangedEvent) return;

                    // Remember the center point
                    var centerX = (viewer.viewPortLeft + viewer.viewPortRight) / 2;
                    var centerY = (viewer.viewPortTop + viewer.viewPortBottom) / 2;

                    // Aspect ratio and zoom factor
                    var aspectRatio = viewer.viewPortWidth / viewer.viewPortHeight;
                    var zoomTo = Math.max(0.01, value);

                    // Zoom by adjusting ViewPortWidth and ViewPortHeight while maintaining the aspect ratio
                    viewer.viewPortWidth = zoomTo * ((aspectRatio < 1) ? 1 : aspectRatio);
                    viewer.viewPortHeight = zoomTo * ((aspectRatio > 1) ? 1 : (1 / aspectRatio));

                    // Adjust ViewPortLeft and ViewPortTop to keep center point unchanged
                    viewer.viewPortLeft = centerX - viewer.viewPortWidth / 2;
                    viewer.viewPortTop = centerY - viewer.viewPortHeight / 2;

                    // Update the chart image only, but no need to update the image map.
                    viewer.updateViewPort(true, false);
                }
            }
        }

        QmlViewPortControl {
            id: vpCtrl;         
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            width: 110
            height: 110

            // Bind the viewport control with the viewer
            viewer: viewer;
            // Set the viewport coloring scheme
            viewPortExternalColor: 0x7f000000
            viewPortBorderColor: 0x7fffffff
            selectionBorderColor: 0x7fffffff
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
        x: 130
        id: viewer

        // Set default mouse usage to scroll and mouse wheel to zoom. Use
        // bidirectional zoom/scroll.
        mouseUsage: QmlChartViewer.MouseUsageScroll
        mouseWheelZoomRatio: 1.1
        scrollDirection: QmlChartViewer.HorizontalVertical
        zoomDirection: QmlChartViewer.KeepAspectRatio

        // Configure the CDML tooltip container to use semi-transparent background and Arial Bold font
        cdmlToolTipPrefix: "<*block,bgColor=60FFFFDD,roundedCorners=5,margin=5,edgeColor=000000*><*font=Arial Bold*>"

        // Update chart on viewport change. Update the slider too.
        onViewPortChanged: {
            if (needUpdateChart) demo.drawChart(this);
            zoomSlider.value = Math.min(viewPortWidth, viewPortHeight)
        }
    }

    // The backend implementation of this demo.
    ContourZoomScroll {
        id: demo
    }

    Component.onCompleted: {
        demo.initChartViewer(viewer, vpCtrl);
        viewer.updateViewPort(true, true);
    }
}
