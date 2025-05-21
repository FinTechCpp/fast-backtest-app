import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Finance Chart Track Line"
    visible: true
    modality: Qt.ApplicationModal
    width: viewer.width
    minimumWidth: viewer.width
    maximumWidth: viewer.width
    height: viewer.height
    minimumHeight: viewer.height
    maximumHeight: viewer.height

    // The backend implementation of this demo.
    TrackFinanceDemo { id: demo }

    QmlChartViewer {
        id: viewer
        Component.onCompleted: demo.drawChart(this)
        // Update track cursor on mouse move
        onMouseMovePlotArea: demo.drawTrackCursor(this, chartMouseX)
    }
}
