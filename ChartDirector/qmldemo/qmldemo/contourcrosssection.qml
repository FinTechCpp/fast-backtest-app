import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "Contour Cross Section"
    width: 690
    minimumWidth: 690
    maximumWidth: 690
    height: 640
    minimumHeight: 690
    maximumHeight: 690
    visible: true
    modality: Qt.ApplicationModal


    QmlChartViewer {
        id: mainViewer
        x: 10
        y: 10

        // Keeps track of mouse dragging of the crosshair cursor
        property string isDragging: "";
        property int dragOffset: 0;
        property int crossHairX: -1;
        property int crossHairY: -1;

        // Update the crosshair and the cross sections
        function drawCrossHair(x, y) {
            if (x !== crossHairX)
                demo.drawCrossSectionX(crossSectionXViewer, x);
            if (y !== crossHairY)
                demo.drawCrossSectionY(crossSectionYViewer, y);
            if ((x !== crossHairY) || (y !== crossHairY))
                demo.drawCrossHair(this, x, y);
            crossHairX = x;
            crossHairY = y;
        }

        onMousePressed: function(button) {
            if ((button !== Qt.LeftButton) || !isMouseOnPlotArea) return;

            // If mouse is near a line when button is pressed, it drags the line.
            if (Math.abs(dragOffset = chartMouseX - crossHairX) <= 8)
                isDragging = "x";  // Drags the vertical line
            else if (Math.abs(dragOffset = chartMouseY - crossHairY) <= 8)
                isDragging = "y";  // Drags the horizontal line
            else
                isDragging = "";
        }
        onMouseMoveChart: {
            if (isDragging == "x") {
                // Is dragging the vertical crosshair line
                var newX = Math.max(plotAreaLeft, Math.min(plotAreaLeft + plotAreaWidth,
                                                           chartMouseX - dragOffset));
                drawCrossHair(newX, crossHairY);
            }
            if (isDragging == "y") {
                // Is dragging the horizontal crosshair line
                var newY = Math.max(plotAreaTop, Math.min(plotAreaTop + plotAreaHeight,
                                                          chartMouseY - dragOffset));
                drawCrossHair(crossHairX, newY);
            }
        }
        onMouseReleased: function(button) {
            if (button === Qt.LeftButton) isDragging = "";
        }
    }

    QmlChartViewer {
        id: crossSectionXViewer
        anchors.left: mainViewer.right
        anchors.top: mainViewer.top
    }

    QmlChartViewer {
        id: crossSectionYViewer
        anchors.top: mainViewer.bottom
        anchors.left: mainViewer.left
    }

    // The backend implementation of this demo.
    ContourCrossSection {
        id: demo
    }

    Component.onCompleted: {
        demo.drawChart(mainViewer);
        mainViewer.drawCrossHair(mainViewer.plotAreaLeft + mainViewer.plotAreaWidth/2,
                                 mainViewer.plotAreaTop + mainViewer.plotAreaHeight/2);
    }
}
