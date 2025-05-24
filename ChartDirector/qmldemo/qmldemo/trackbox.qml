import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Track Box with Floating Legend"
    visible: true
    modality: Qt.ApplicationModal
    width: viewer.width
    maximumWidth: viewer.width
    minimumWidth: viewer.width
    height: viewer.height
    maximumHeight: viewer.height
    minimumHeight: viewer.height

    // The backend implementation of this demo.
    TrackBoxDemo { id: demo }

    QmlChartViewer {
        id: viewer
        Component.onCompleted: demo.drawChart(this)
        // Update track cursor on mouse move
        onMouseMovePlotArea: demo.drawTrackCursor(this, chartMouseX, chartMouseY)
    }
}
