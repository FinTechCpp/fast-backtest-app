import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Shapes
import advsofteng.com 1.0


ApplicationWindow {
    visible: true
    title: "ChartDirector Qt/QML Demo"
    minimumWidth: 1000
    minimumHeight: 600

    //
    // The left panel containing the tree to select the demo
    //
    Rectangle {
        id: leftPanel
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 5
        width: 250
        border.color: "#888888"
        border.width: 1

        // The demo selection tree, implemented using a ListVIew
        ListView {
            id: demoListView
            model: ListModel {}
            anchors.fill: parent
            anchors.margins: parent.border.width
            clip: true
            ScrollBar.vertical: ScrollBar { active: true }
            focus: true

            // User click on the node on the tree
            onCurrentIndexChanged: {
                var m = model.get(currentIndex);
                if (m.isFolder) return;  // Clicked on a folder - do nothing
                if (m.demoQML) {  // Clicked on a demo in a qml file - load the qml file
                    demoLoader.source = m.demoQML;
                } else {
                    // It must be a demo generating a number of charts directly. display
                    // the charts in the qmlchartviewer controls in this page.
                    for (var i = 0; i < chartViewers.count; ++i) {
                        if (i < m.chartCount)
                            qmlChartsDemo.drawChart(chartViewers.itemAt(i), m.demoChart, i);
                        chartViewers.itemAt(i).visible = (i < m.chartCount);
                    }
                }
            }

            // The appearance of a node in the tree. Each node contains an icon and text. A
            // folder node also includes triangle at the left side to open/close the folder.
            delegate: ItemDelegate {
                leftPadding: isFolder ? 3 : 40
                width: ListView.view.width
                height: 24
                palette.highlight: "#AACCFF"
                highlighted: ListView.isCurrentItem
                contentItem: Row {
                    spacing: 5
                    Shape {   // The triangle which is visible on for a folder node
                        anchors.verticalCenter: parent.verticalCenter
                        visible: isFolder
                        width: 12; height: 12
                        transform: Rotation { origin.x: 6; origin.y: 6; angle: isOpened ? 0 : -90 }
                        ShapePath {
                            strokeColor: "#666666";
                            fillColor: "#666666"
                            startX: 0; startY: 3
                            PathLine { x: 12; y: 3 }
                            PathLine { x: 6; y: 9 }
                        }
                        MouseArea {
                            x: -4; y: -4
                            width: 20; height: 20
                            onClicked: toggleFolder()  // When clicked, open/close folder
                        }
                    }
                    Image {
                        anchors.verticalCenter: parent.verticalCenter;
                        source: iconSource;
                        width:16;
                        height:16;
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter;
                        text: name;
                        font.pixelSize: 12;
                    }
                }

                onClicked: ListView.view.currentIndex = index;  // Click the node to select it
                onDoubleClicked: toggleFolder();  // Double click a folder also open/close it
                // Folder can also be open close by keyboard too.
                Keys.onRightPressed: if (isFolder && !isOpened) toggleFolder();
                Keys.onLeftPressed: if (isFolder && isOpened) toggleFolder();

                // Open the folder if it is closed, and vice versa.
                function toggleFolder() {
                    if (!isFolder) return;
                    isOpened = !isOpened;
                    if (!isOpened) {
                        ListView.view.model.remove(index + 1, nodes.count);
                    } else {
                        for (var n = 0; n < nodes.count; ++n) {
                            ListView.view.model.insert(index + n + 1, nodes.get(n));
                        }
                        ListView.view.positionViewAtIndex(
                            Math.min(index + nodes.count + 1, ListView.view.model.count - 1), ListView.Visible);
                        ListView.view.positionViewAtIndex(index, ListView.Visible);
                    }
                }
            }
        }
    }

    // Thr right side which holds 8 qmlchartviewer objects to display the charts.
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: leftPanel.right
        anchors.right: parent.right
        anchors.margins: 5
        border.color: "#888888"
        border.width: 1

        Flow {
            clip: true
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Repeater {
                id: chartViewers
                model: 8
                QmlChartViewer {
                    onClicked: {
                        var h = getHotSpot();
                        if (h["path"])
                            hotSpotDialog.showHotSpot(h)
                    }
                }
            }
        }
    }

    // This is use to load demo implemented in a qml file.
    Loader {
        id: demoLoader

        // Remember to clear the loader when the demo qml window closed.
        Connections {
            target: demoLoader.item
            onClosing: demoLoader.source = ""
        }
    }

    // This window a pop-up window to display the hot spot parameters for demo
    // purpose.
    Window {
        id: hotSpotDialog
        width: 450
        height: 300
        modality: Qt.ApplicationModal
        title: "Hot Spot Parameters"
        color: "#eeeeee"
            ListView {
                id: hotSpotList
                model: ListModel {}
                anchors.fill: parent
                spacing: 0

                // Each item displays the name and value of a parameter.
                delegate: ItemDelegate {
                    width: ListView.view.width
                    height: 24
                    padding: 0
                    contentItem: Row {
                        height: 23
                        Rectangle {
                            width: 100
                            height: 23
                            color: "#cce5ff"
                            Text {
                                padding: 3
                                text: name
                                font.pixelSize: size ? size : 13
                            }
                        }
                        Rectangle {
                            width: parent.width - 100
                            height: 23
                            color: "white"
                            Text {
                                padding: 3
                                text: value
                                font.pixelSize: size ? size : 13
                            }
                        }
                    }
                }

        }           
        function showHotSpot(h)
        {
            // Hot spot parameters returned by ChartDirector is an associative map.
            // We need to convert them to a ListModel so they can be displayed in
            // the ListView.
            var avList = hotSpotList.model;
            avList.clear();
            // The first item are used as the header of the list.
            avList.append([{ name: "<b>Name</b>", value:"<b>Value</b>", size: 14 }]);
            for(var attr in h)
                avList.append({ name: attr, value: h[attr] });
            hotSpotDialog.show();
        }
    }

    // The backend implementation of this demo. This contains all the demo charting
    // code in C++.
    QmlChartsDemo {
        id: qmlChartsDemo
    }

    Component.onCompleted: {
        // Obtain the catalog of the demo and convert it to a suitable ListModel to
        // populate the demo selection tree.
        var demoCharts = qmlChartsDemo.getDemoCatalog();
        var demoList = demoListView.model;
        var folder = 0;
        for (var i = 0; i < demoCharts.length; ++i) {
            var demo = demoCharts[i];
            if (demo.isFolder) {
                // The item is not an actual demo, but a folder. Create an array to
                // hold the actual demo in the folder.
                demo["nodes"] = [];
                demo["isOpened"] = false;
                demoList.append(demo);
                folder = demoList.get(demoList.count - 1);
            } else {
                // The item is a demo. Add it to the folder. The demo's icon is same
                // as the folder's icon.
                demo["iconSource"] = folder.iconSource;
                folder.nodes.append(demo);
            }
        }
    }
}
