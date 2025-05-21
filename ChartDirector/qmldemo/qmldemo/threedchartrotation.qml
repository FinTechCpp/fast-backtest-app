import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "3D Chart Interactive Rotation"
    visible: true
    modality: Qt.ApplicationModal
    width: 720
    minimumWidth: 720
    maximumWidth: 720
    height: 630
    minimumHeight: 630
    maximumHeight: 630

    // The backend implementation of this demo.
    ThreeDChartRotation { id: demo }

    Column {
        anchors.fill: parent
        Pane {
            width: parent.width
            CheckBox {
                x: 20
                id: drawFrameOnly
                text: "Draw Frame On Rotate"
            }
        }
        QmlChartViewer {
            id: viewer
            Component.onCompleted: updateViewPort(true, true)
            // Update chart on viewport change
            onViewPortChanged: {
                demo.drawFrameOnly = drawFrameOnly.checked && isDragging
                demo.drawChart(this);
            }

            property bool isDragging: false
            property int lastMouseX
            property int lastMouseY

            onMousePressed: function(button) {
                if (button === Qt.LeftButton) isDragging = true;
            }
            onMouseMoveChart: {
                if (isDragging) {
                    // The chart is configured to rotate by 90 degrees when the mouse moves from
                    // left to right, which is the plot region width (360 pixels). Similarly, the
                    // elevation changes by 90 degrees when the mouse moves from top to buttom,
                    // which is the plot region height (270 pixels).
                    demo.rotationAngle += (lastMouseX - chartMouseX) * 90.0 / 360;
                    demo.elevationAngle += (chartMouseY - lastMouseY) * 90.0 / 270;
                    updateViewPort(true, true);
                }
                lastMouseX = chartMouseX;
                lastMouseY = chartMouseY;
            }
            onMouseReleased: function(button) {
                if (button === Qt.LeftButton) {
                    isDragging = false;
                    viewer.updateViewPort(true, true);
                }
            }
        }
    }
}


