import QtQuick
import QtQuick.Window
import QtQuick.Controls
import advsofteng.com 1.0

Window {
    title: "ChartDirector Interactive Financial Chart Demonstration"
    visible: true
    modality: Qt.ApplicationModal
    width: 930
    minimumWidth: 930
    maximumWidth: 930
    height: 490
    minimumHeight: 490
    maximumHeight: 490

    Pane {
        id: leftPane
        width: 140
        padding: 5
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;

        Column {
            width: parent.width
            spacing: 1

            Row {
                spacing: 17
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

            Item { width: 1; height: 8 } //spacer

            Text { text: "Ticker Symbol" }
            Rectangle {
                width: parent.width
                height: childrenRect.height
                border.color: "#888888"

                TextInput {
                    id: tickerSymbol
                    width: parent.width
                    padding: 2
                    onEditingFinished: {
                        if (text === "") text = "ASE"
                        demo.drawChart(viewer)
                    }
                }
            }

            Item { width: 1; height: 8 } //spacer

            Text { text: "Compare With" }
            Rectangle {
                width: parent.width
                height: childrenRect.height
                border.color: "#888888"

                TextInput {
                    id: compareWith
                    width: parent.width
                    padding: 2
                    onEditingFinished: demo.drawChart(viewer)
                }
            }

            Item { width: 1; height: 8 } //spacer

            Text { text: "Time Period" }
            ComboBox {
                id: timeRange
                width: parent.width
                textRole: "text"
                valueRole: "value"
                model: [
                    { value:30, text:"1 month" },
                    { value:60, text:"2 months" },
                    { value:90, text:"3 months" },
                    { value:180, text:"6 months" },
                    { value:360, text:"1 year" },
                    { value:720, text:"2 years" },
                    { value:1080, text:"3 years" },
                    { value:1440, text:"4 years" },
                    { value:1800, text:"5 years" },
                    { value:3600, text:"10 years" }
                ]
                onActivated: demo.drawChart(viewer)
            }

            Item { width: 1; height: 8 } //spacer

            CheckBox {
                id: volumeBars;
                text: "Show Volume Bars"
                onClicked: demo.drawChart(viewer)
            }
            CheckBox {
                id: parabolicSAR;
                text: "Parabolic SAR"
                onClicked: demo.drawChart(viewer)
            }
            CheckBox {
                id: logScale;
                text: "Log Scale"
                onClicked: demo.drawChart(viewer)
            }
            CheckBox {
                id: percentageScale;
                text: "Percentage Scale"
                onClicked: demo.drawChart(viewer)
            }

            Item { width: 1; height: 8 } //spacer

            Text { text: "Chart Type" }
            ComboBox {
                width: parent.width
                id: chartType
                textRole: "text"
                valueRole: "value"
                model: [
                    { value:"None", text:"None" },
                    { value:"CandleStick", text:"CandleStick" },
                    { value:"Close", text:"Closing Price" },
                    { value:"Median", text:"Median Price" },
                    { value:"OHLC", text:"OHLC" },
                    { value:"TP", text:"Typical Price" },
                    { value:"WC", text:"Weighted Close" }
                ]
                onActivated: demo.drawChart(viewer)
            }

            Item { width: 1; height: 8 } //spacer

            Text { text: "Price Band" }
            ComboBox {
                width: parent.width
                id: priceBand
                textRole: "text"
                valueRole: "value"
                model: [
                    { value:"None", text:"None" },
                    { value:"BB", text:"Bollinger Band" },
                    { value:"DC", text:"Donchain Channel" },
                    { value:"Envelop", text:"Envelop (SMA 20 +/- 10%)" }
                ]
                onActivated: demo.drawChart(viewer)
            }

            Item { width: 1; height: 8 } //spacer

            Text { text: "Moving Averages" }
            Row {
                width: parent.width

                ComboBox {
                    id: avgType1
                    width: parent.width - 32
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { value:"None", text:"None" },
                        { value:"SMA", text:"Simple" },
                        { value:"EMA", text:"Exponential" },
                        { value:"TMA", text:"Triangular" },
                        { value:"WMA", text:"Weighted" }
                    ]
                    onActivated: demo.drawChart(viewer)
                 }
                Item { width: 2; height: 1} // spacer
                Rectangle {
                    width: 30
                    height: childrenRect.height
                    border.color: "#888888"

                    TextInput {
                        id: movAvg1
                        width: parent.width
                        padding: 2
                        onEditingFinished: demo.drawChart(viewer)
                    }
                }
            }
            Row {
                width: parent.width

                ComboBox {
                    id: avgType2
                    width: parent.width - 32
                    textRole: "text"
                    valueRole: "value"
                    model: avgType1.model
                    onActivated: demo.drawChart(viewer)
                }
                Item { width: 2; height: 1} // spacer
                Rectangle {
                    width: 30
                    height: childrenRect.height
                    border.color: "#888888"

                    TextInput {
                        id: movAvg2
                        width: parent.width
                        padding: 2
                        onEditingFinished: demo.drawChart(viewer)
                    }
                }
            }

            Item { width: 1; height: 8 } //spacer

            Text { text: "Technical Indicators" }
            ComboBox {
                id: indicator1
                width: parent.width
                textRole: "text"
                valueRole: "value"
                model:[
                    { value:"None", text:"None" },
                    { value:"AccDist", text:"Accumulation/Distribution" },
                    { value:"AroonOsc", text:"Aroon Oscillator" },
                    { value:"Aroon", text:"Aroon Up/Down" },
                    { value:"ADX", text:"Avg Directional Index" },
                    { value:"ATR", text:"Avg True Range" },
                    { value:"BBW", text:"Bollinger Band Width" },
                    { value:"CMF", text:"Chaikin Money Flow" },
                    { value: "COscillator", text:"Chaikin Oscillator" },
                    { value: "CVolatility", text:"Chaikin Volatility" },
                    { value:"CLV", text:"Close Location Value" },
                    { value:"CCI", text:"Commodity Channel Index" },
                    { value:"DPO", text:"Detrended Price Osc" },
                    { value:"DCW", text:"Donchian Channel Width" },
                    { value:"EMV", text:"Ease of Movement" },
                    { value:"FStoch", text:"Fast Stochastic" },
                    { value:"MACD", text:"MACD" },
                    { value:"MDX", text:"Mass Index" },
                    { value:"Momentum", text:"Momentum" },
                    { value:"MFI", text:"Money Flow Index" },
                    { value:"NVI", text:"Neg Volume Index" },
                    { value:"OBV", text:"On Balance Volume" },
                    { value:"Performance", text:"Performance" },
                    { value: "PPO", text:"% Price Oscillator" },
                    { value:"PVO", text:"% Volume Oscillator" },
                    { value:"PVI", text:"Pos Volume Index" },
                    { value:"PVT", text:"Price Volume Trend" },
                    { value:"ROC", text:"Rate of Change" },
                    { value:"RSI", text:"RSI" },
                    { value:"SStoch", text:"Slow Stochastic" },
                    { value:"StochRSI", text:"StochRSI" },
                    { value:"TRIX", text:"TRIX" },
                    { value:"UO", text:"Ultimate Oscillator" },
                    { value:"Vol", text:"Volume" },
                    { value:"WilliamR", text:"William's %R" }
                ]
                onActivated: demo.drawChart(viewer)
            }
            ComboBox {
                id: indicator2
                width: parent.width
                textRole: "text"
                valueRole: "value"
                model: indicator1.model
                onActivated: demo.drawChart(viewer)
            }
        }
    }

    // The QML ChartViewer
    QmlChartViewer {
        id: viewer
        anchors.left: leftPane.right
        anchors.leftMargin: 5

        // Set default mouse usage to scroll and mouse wheel to zoom
        mouseUsage: QmlChartViewer.MouseUsageScroll
        mouseWheelZoomRatio: 1.1
        // Update track cursor on mouse move
        onMouseMovePlotArea: demo.drawTrackCursor(this, plotAreaMouseX)
        // Update chart on viewport change
        onViewPortChanged: demo.drawChart(this)
    }

    // The backend implementation of this demo with parameters binded to QML controls.
    FinanceDemo {
        id: demo
        tickerSymbol: tickerSymbol.text
        compareWith: compareWith.text
        timeRange: timeRange.currentValue
        volumeBars: volumeBars.checked
        parabolicSAR: parabolicSAR.checked
        logScale: logScale.checked
        percentageScale: percentageScale.checked
        chartType: chartType.currentValue
        priceBand: priceBand.currentValue
        avgType1: avgType1.currentValue
        movAvg1: movAvg1.text
        avgType2: avgType2.currentValue
        movAvg2: movAvg2.text
        indicator1: indicator1.currentValue
        indicator2: indicator2.currentValue
    }

    // Set the default parameters and display the chart
    Component.onCompleted: {
        tickerSymbol.text = "ASE"
        compareWith.text = ""
        timeRange.currentIndex = timeRange.indexOfValue(90)
        volumeBars.checked = true
        parabolicSAR.checked = false
        logScale.checked = false
        percentageScale.checked = false
        chartType.currentIndex = chartType.indexOfValue("CandleStick")
        priceBand.currentIndex = priceBand.indexOfValue("BB")
        avgType1.currentIndex = avgType1.indexOfValue("SMA")
        movAvg1.text = "5"
        avgType2.currentIndex = avgType1.indexOfValue("SMA")
        movAvg2.text = "20"
        indicator1.currentIndex = indicator1.indexOfValue("RSI")
        indicator2.currentIndex = indicator1.indexOfValue("MACD")

        demo.drawChart(viewer)
    }
}
