import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import QtCharts 2.15
import ComponentsForest 1.0

Rectangle {
    id: root
    radius: 5
    color: "#16213e"
    
    property var cameraBridge: null
    property int updateInterval: 1000
    property int maxDataPoints: 60
    
    // Data storage for charts
    property var fpsHistory: []
    property var frameCountHistory: []
    property var cpuHistory: []
    property var memoryHistory: []
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                Layout.fillWidth: true
                text: "Performance Statistics"
                font.pixelSize: 14
                font.bold: true
                color: Material.accent
            }
            
            Button {
                text: "Reset"
                flat: true
                onClicked: resetStatistics()
            }
        }
        
        // Quick stats grid
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 5
            columnSpacing: 15
            
            // Current FPS
            Label {
                text: "Current FPS:"
                font.pixelSize: 11
                color: Material.color(Material.Grey, Material.Shade300)
            }
            Label {
                text: cameraBridge ? cameraBridge.fps.toFixed(1) : "0.0"
                font.pixelSize: 11
                font.bold: true
                color: {
                    if (!cameraBridge) return Material.foreground
                    if (cameraBridge.fps > 25) return Material.color(Material.Green)
                    if (cameraBridge.fps > 10) return Material.color(Material.Yellow)
                    return Material.color(Material.Red)
                }
            }
            
            // Average FPS
            Label {
                text: "Average FPS:"
                font.pixelSize: 11
                color: Material.color(Material.Grey, Material.Shade300)
            }
            Label {
                text: calculateAverageFps().toFixed(1)
                font.pixelSize: 11
                font.bold: true
            }
            
            // Total frames
            Label {
                text: "Total Frames:"
                font.pixelSize: 11
                color: Material.color(Material.Grey, Material.Shade300)
            }
            Label {
                text: cameraBridge ? cameraBridge.frameCount.toString() : "0"
                font.pixelSize: 11
                font.bold: true
            }
            
            // Dropped frames
            Label {
                text: "Dropped Frames:"
                font.pixelSize: 11
                color: Material.color(Material.Grey, Material.Shade300)
            }
            Label {
                text: cameraBridge ? (cameraBridge.droppedFrames || 0).toString() : "0"
                font.pixelSize: 11
                font.bold: true
                color: cameraBridge && cameraBridge.droppedFrames > 0 ? 
                       Material.color(Material.Orange) : Material.foreground
            }
            
            // Acquisition time
            Label {
                text: "Acquisition Time:"
                font.pixelSize: 11
                color: Material.color(Material.Grey, Material.Shade300)
            }
            Label {
                text: formatTime(cameraBridge ? cameraBridge.acquisitionTime : 0)
                font.pixelSize: 11
                font.bold: true
            }
            
            // Buffer usage
            Label {
                text: "Buffer Usage:"
                font.pixelSize: 11
                color: Material.color(Material.Grey, Material.Shade300)
            }
            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 100
                value: cameraBridge ? cameraBridge.bufferUsage : 0
                
                Material.accent: {
                    if (value < 50) return Material.color(Material.Green)
                    if (value < 80) return Material.color(Material.Yellow)
                    return Material.color(Material.Red)
                }
            }
        }
        
        // Separator
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Material.color(Material.Grey, Material.Shade800)
        }
        
        // Tab view for charts
        TabBar {
            id: chartTabBar
            Layout.fillWidth: true
            
            TabButton {
                text: "FPS"
            }
            TabButton {
                text: "Frames"
            }
            TabButton {
                text: "System"
            }
        }
        
        // Chart area
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: chartTabBar.currentIndex
            
            // FPS Chart
            ChartView {
                id: fpsChart
                antialiasing: true
                theme: ChartView.ChartThemeDark
                backgroundColor: "transparent"
                legend.visible: false
                margins.top: 5
                margins.bottom: 5
                margins.left: 5
                margins.right: 5
                
                ValueAxis {
                    id: fpsXAxis
                    min: 0
                    max: maxDataPoints
                    tickCount: 5
                    labelsVisible: false
                    gridVisible: false
                }
                
                ValueAxis {
                    id: fpsYAxis
                    min: 0
                    max: 120
                    tickCount: 5
                    labelFormat: "%d"
                    titleText: "FPS"
                }
                
                LineSeries {
                    id: fpsSeries
                    name: "FPS"
                    axisX: fpsXAxis
                    axisY: fpsYAxis
                    color: Material.color(Material.Cyan)
                    width: 2
                }
                
                AreaSeries {
                    name: "FPS Area"
                    axisX: fpsXAxis
                    axisY: fpsYAxis
                    color: Material.color(Material.Cyan)
                    opacity: 0.3
                    upperSeries: fpsSeries
                }
            }
            
            // Frame Count Chart
            ChartView {
                id: frameChart
                antialiasing: true
                theme: ChartView.ChartThemeDark
                backgroundColor: "transparent"
                legend.visible: false
                margins.top: 5
                margins.bottom: 5
                margins.left: 5
                margins.right: 5
                
                ValueAxis {
                    id: frameXAxis
                    min: 0
                    max: maxDataPoints
                    tickCount: 5
                    labelsVisible: false
                    gridVisible: false
                }
                
                ValueAxis {
                    id: frameYAxis
                    min: 0
                    max: 1000
                    tickCount: 5
                    labelFormat: "%d"
                    titleText: "Frames"
                }
                
                LineSeries {
                    id: frameSeries
                    name: "Frame Count"
                    axisX: frameXAxis
                    axisY: frameYAxis
                    color: Material.color(Material.Green)
                    width: 2
                }
            }
            
            // System Performance Chart
            ChartView {
                id: systemChart
                antialiasing: true
                theme: ChartView.ChartThemeDark
                backgroundColor: "transparent"
                legend.visible: true
                legend.alignment: Qt.AlignBottom
                margins.top: 5
                margins.bottom: 5
                margins.left: 5
                margins.right: 5
                
                ValueAxis {
                    id: systemXAxis
                    min: 0
                    max: maxDataPoints
                    tickCount: 5
                    labelsVisible: false
                    gridVisible: false
                }
                
                ValueAxis {
                    id: systemYAxis
                    min: 0
                    max: 100
                    tickCount: 5
                    labelFormat: "%d%%"
                    titleText: "Usage"
                }
                
                LineSeries {
                    id: cpuSeries
                    name: "CPU"
                    axisX: systemXAxis
                    axisY: systemYAxis
                    color: Material.color(Material.Orange)
                    width: 2
                }
                
                LineSeries {
                    id: memorySeries
                    name: "Memory"
                    axisX: systemXAxis
                    axisY: systemYAxis
                    color: Material.color(Material.Purple)
                    width: 2
                }
            }
        }
        
        // Statistics summary
        Rectangle {
            Layout.fillWidth: true
            height: 60
            radius: 5
            color: Qt.rgba(0, 0, 0, 0.3)
            
            GridLayout {
                anchors.fill: parent
                anchors.margins: 10
                columns: 3
                
                Column {
                    spacing: 2
                    
                    Label {
                        text: "Min FPS"
                        font.pixelSize: 10
                        color: Material.color(Material.Grey, Material.Shade400)
                    }
                    Label {
                        text: calculateMinFps().toFixed(1)
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
                
                Column {
                    spacing: 2
                    
                    Label {
                        text: "Max FPS"
                        font.pixelSize: 10
                        color: Material.color(Material.Grey, Material.Shade400)
                    }
                    Label {
                        text: calculateMaxFps().toFixed(1)
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
                
                Column {
                    spacing: 2
                    
                    Label {
                        text: "Stability"
                        font.pixelSize: 10
                        color: Material.color(Material.Grey, Material.Shade400)
                    }
                    Label {
                        text: calculateStability().toFixed(0) + "%"
                        font.pixelSize: 12
                        font.bold: true
                        color: {
                            var stability = calculateStability()
                            if (stability > 90) return Material.color(Material.Green)
                            if (stability > 70) return Material.color(Material.Yellow)
                            return Material.color(Material.Red)
                        }
                    }
                }
            }
        }
    }
    
    // Update timer
    Timer {
        interval: updateInterval
        running: cameraBridge && cameraBridge.isAcquiring && visible
        repeat: true
        onTriggered: updateStatistics()
    }
    
    // Functions
    function updateStatistics() {
        if (!cameraBridge) return
        
        // Update FPS history
        fpsHistory.push(cameraBridge.fps)
        if (fpsHistory.length > maxDataPoints) {
            fpsHistory.shift()
        }
        
        // Update frame count history
        frameCountHistory.push(cameraBridge.frameCount)
        if (frameCountHistory.length > maxDataPoints) {
            frameCountHistory.shift()
        }
        
        // Update CPU history (simulated for now)
        cpuHistory.push(Math.random() * 30 + 20) // Simulate 20-50% CPU
        if (cpuHistory.length > maxDataPoints) {
            cpuHistory.shift()
        }
        
        // Update memory history (simulated for now)
        memoryHistory.push(Math.random() * 20 + 30) // Simulate 30-50% memory
        if (memoryHistory.length > maxDataPoints) {
            memoryHistory.shift()
        }
        
        // Update charts
        updateCharts()
    }
    
    function updateCharts() {
        // Update FPS chart
        fpsSeries.clear()
        for (var i = 0; i < fpsHistory.length; i++) {
            fpsSeries.append(i, fpsHistory[i])
        }
        
        // Update frame chart
        frameSeries.clear()
        for (i = 0; i < frameCountHistory.length; i++) {
            frameSeries.append(i, frameCountHistory[i])
        }
        
        // Adjust frame Y axis if needed
        if (frameCountHistory.length > 0) {
            var maxFrames = Math.max.apply(null, frameCountHistory)
            frameYAxis.max = Math.ceil(maxFrames / 100) * 100
        }
        
        // Update system chart
        cpuSeries.clear()
        memorySeries.clear()
        for (i = 0; i < cpuHistory.length; i++) {
            cpuSeries.append(i, cpuHistory[i])
            memorySeries.append(i, memoryHistory[i])
        }
    }
    
    function resetStatistics() {
        fpsHistory = []
        frameCountHistory = []
        cpuHistory = []
        memoryHistory = []
        
        fpsSeries.clear()
        frameSeries.clear()
        cpuSeries.clear()
        memorySeries.clear()
    }
    
    function calculateAverageFps() {
        if (fpsHistory.length === 0) return 0
        var sum = fpsHistory.reduce(function(a, b) { return a + b }, 0)
        return sum / fpsHistory.length
    }
    
    function calculateMinFps() {
        if (fpsHistory.length === 0) return 0
        return Math.min.apply(null, fpsHistory)
    }
    
    function calculateMaxFps() {
        if (fpsHistory.length === 0) return 0
        return Math.max.apply(null, fpsHistory)
    }
    
    function calculateStability() {
        if (fpsHistory.length < 2) return 100
        
        var avg = calculateAverageFps()
        if (avg === 0) return 0
        
        var variance = 0
        for (var i = 0; i < fpsHistory.length; i++) {
            variance += Math.pow(fpsHistory[i] - avg, 2)
        }
        variance /= fpsHistory.length
        
        var stdDev = Math.sqrt(variance)
        var cv = (stdDev / avg) * 100 // Coefficient of variation
        
        return Math.max(0, 100 - cv)
    }
    
    function formatTime(milliseconds) {
        if (!milliseconds) return "00:00:00"
        
        var seconds = Math.floor(milliseconds / 1000)
        var minutes = Math.floor(seconds / 60)
        var hours = Math.floor(minutes / 60)
        
        seconds = seconds % 60
        minutes = minutes % 60
        
        return String(hours).padStart(2, '0') + ":" +
               String(minutes).padStart(2, '0') + ":" +
               String(seconds).padStart(2, '0')
    }
    
    // Initialize on component creation
    Component.onCompleted: {
        resetStatistics()
    }
}