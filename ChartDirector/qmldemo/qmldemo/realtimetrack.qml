import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Real-Time Chart with Track Line"
    visible: true
    modality: Qt.ApplicationModal
    width: 730
    minimumWidth: 730
    maximumWidth: 700
    height: 280
    minimumHeight: 280
    maximumHeight: 280

    Pane {
        id: leftPane
        width: 120
        padding: 5
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        Column {
            ButtonGroup { id: runGroup }
            Button {
                id: runPB
                width: 110
                contentItem: Row {
                    padding:2; leftPadding: 5
                    Image { source: "icons/play.png"; width:16; height:16; }
                    Text { text: "  Run"; font.pixelSize: 12; }
                }
                checked: chartUpdateTimer.running
                onClicked: chartUpdateTimer.running = true;
              }
            Button {
                id: freezePB
                width: 110
                contentItem: Row {
                    padding:2; leftPadding: 5;
                    Image { source: "icons/pause.png"; width:16; height:16; }
                    Text { text: "  Freeze"; font.pixelSize: 12; }
                }
                checked: !chartUpdateTimer.running
                onClicked: chartUpdateTimer.running = false;
            }

            // Spacer
            Item {width: 1; height: 30}

            Text {
                text: "Update Period (ms)"
                bottomPadding: 3
            }

            ComboBox {
                width: 110
                model: ["100ms", "200ms", "300ms", "500ms", "700ms", "1000ms"]
                onActivated: chartUpdateTimer.interval = parseInt(currentText)
            }
        }

        Column {
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 20
            spacing: 4

            Row {
                Text { text: "Alpha:"; width: 55 }
                Rectangle {
                    width: 55
                    height: childrenRect.height + 4
                    border.color: "#888888"
                    Text { x: 3; id: alphaValue; }
                }
            }
            Row {
                Text { text: "Beta:"; width: 55 }
                Rectangle {
                    width: 55
                    height: childrenRect.height + 4
                    border.color: "#888888"
                    Text { x: 3; id: betaValue; }
                }
            }
            Row {
                Text { text: "Gamma:"; width: 55 }
                Rectangle {
                    width: 55
                    height: childrenRect.height + 4
                    border.color: "#888888"
                    Text { x: 3; id: gammaValue; }
                }
            }
        }
    }

    QmlChartViewer
    {
        id: viewer
        anchors.left: leftPane.right
        anchors.leftMargin: 5
        y: 5
        // Update chart on viewport change
        onViewPortChanged: demo.drawChart(this);
        // Update track cursor on mouse move
        onMouseMovePlotArea: demo.drawTrackCursor(this, plotAreaMouseX)
    }

    // The backend implementation of this demo.
    RealTimeTrack {
        id: demo;
    }

    // This example uses a random number generator that generates a random
    // number every 100ms. In real applications, the data can be generated
    // by other means.
    Timer {
        interval:100; running: true; repeat: true
        onTriggered: {
            demo.getData();
            alphaValue.text = demo.ValueA.toFixed(2);
            betaValue.text = demo.ValueB.toFixed(2);
            gammaValue.text = demo.ValueC.toFixed(2);
        }
    }

    // The chart update timer. The chart can update at a different rate from
    // the data, that is, asychronous update.
    Timer {
        id: chartUpdateTimer
        interval:100; running: runPB.checked ; repeat: true
        onTriggered: viewer.updateViewPort(true, true);
    }

    Component.onCompleted: {
        chartUpdateTimer.running = true;
    }
}

