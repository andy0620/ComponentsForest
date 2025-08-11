import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentsForest.Bridge 1.0

Rectangle {
    id: root
    color: "#14181f"
    
    property alias cameraBridge: internal.cameraBridge
    property bool isConnected: false
    property bool isAcquiring: false
    property int frameCount: 0
    property real currentFps: 0.0
    property real averageFps: 0.0
    property int droppedFrames: 0
    property var lastFrameTime: 0
    
    QtObject {
        id: internal
        property CameraBridge cameraBridge: null
    }
    
    // FPS calculation timer
    Timer {
        id: fpsTimer
        interval: 1000
        running: root.isAcquiring
        repeat: true
        property int lastFrameCount: 0
        
        onTriggered: {
            root.currentFps = root.frameCount - lastFrameCount
            lastFrameCount = root.frameCount
            if (root.frameCount > 0) {
                var elapsed = (Date.now() - root.lastFrameTime) / 1000.0
                root.averageFps = root.frameCount / elapsed
            }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        
        // Header with connection controls
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#1a1f2a"
            radius: 4
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                Label {
                    text: "Camera:"
                    color: "#9ca3af"
                    font.pixelSize: 14
                }
                
                ComboBox {
                    id: cameraSelector
                    Layout.preferredWidth: 250
                    model: cameraBridge ? cameraBridge.availableCameras : []
                    enabled: !root.isConnected
                    
                    background: Rectangle {
                        color: cameraSelector.enabled ? "#2a2f3a" : "#1a1f2a"
                        border.color: cameraSelector.hovered ? "#4a9eff" : "#3a3f4a"
                        border.width: 1
                        radius: 3
                    }
                    
                    contentItem: Text {
                        text: cameraSelector.displayText
                        color: cameraSelector.enabled ? "#ffffff" : "#6a6f7a"
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                    }
                }
                
                Button {
                    id: connectButton
                    text: root.isConnected ? "Disconnect" : "Connect"
                    Layout.preferredWidth: 100
                    enabled: cameraBridge && (cameraSelector.currentIndex >= 0 || root.isConnected)
                    
                    background: Rectangle {
                        color: {
                            if (!connectButton.enabled) return "#1a1f2a"
                            if (connectButton.pressed) return "#2a5ea0"
                            if (connectButton.hovered) return "#3a7ec0"
                            return root.isConnected ? "#a04040" : "#2a6eb0"
                        }
                        radius: 3
                    }
                    
                    contentItem: Text {
                        text: connectButton.text
                        color: connectButton.enabled ? "#ffffff" : "#6a6f7a"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (root.isConnected) {
                            if (root.isAcquiring) {
                                cameraBridge.stopAcquisition()
                            }
                            cameraBridge.disconnectCamera()
                        } else if (cameraSelector.currentIndex >= 0) {
                            cameraBridge.connectCamera(cameraSelector.currentIndex)
                        }
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: root.isConnected ? "#40ff40" : "#404040"
                    
                    Rectangle {
                        anchors.centerIn: parent
                        width: 6
                        height: 6
                        radius: 3
                        color: root.isConnected ? "#80ff80" : "#202020"
                    }
                }
                
                Label {
                    text: root.isConnected ? "Connected" : "Disconnected"
                    color: root.isConnected ? "#40ff40" : "#9ca3af"
                    font.pixelSize: 13
                }
            }
        }
        
        // Main content area with image and controls
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            
            // Image display area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0a0d12"
                radius: 4
                
                Image {
                    id: cameraImage
                    anchors.fill: parent
                    anchors.margins: 2
                    fillMode: Image.PreserveAspectFit
                    source: cameraBridge ? cameraBridge.currentFrame : ""
                    cache: false
                    smooth: true
                    
                    // Crosshair overlay
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width
                        height: 1
                        color: "#20ffffff"
                        visible: root.isAcquiring
                    }
                    
                    Rectangle {
                        anchors.centerIn: parent
                        width: 1
                        height: parent.height
                        color: "#20ffffff"
                        visible: root.isAcquiring
                    }
                    
                    // No image indicator
                    Label {
                        anchors.centerIn: parent
                        text: root.isConnected ? (root.isAcquiring ? "" : "No Signal") : "Camera Not Connected"
                        color: "#4a4f5a"
                        font.pixelSize: 16
                        visible: cameraImage.source == "" || cameraImage.status === Image.Error
                    }
                }
                
                // Frame info overlay
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 10
                    color: "#80000000"
                    radius: 3
                    padding: 5
                    width: frameInfoColumn.width + 10
                    height: frameInfoColumn.height + 10
                    visible: root.isAcquiring
                    
                    ColumnLayout {
                        id: frameInfoColumn
                        anchors.centerIn: parent
                        spacing: 2
                        
                        Label {
                            text: "FPS: " + root.currentFps.toFixed(1)
                            color: "#40ff40"
                            font.pixelSize: 11
                            font.family: "Consolas, Monaco, monospace"
                        }
                        
                        Label {
                            text: "Frames: " + root.frameCount
                            color: "#ffffff"
                            font.pixelSize: 11
                            font.family: "Consolas, Monaco, monospace"
                        }
                    }
                }
            }
            
            // Right side control panel
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: "#1a1f2a"
                radius: 4
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15
                    
                    // Acquisition controls
                    GroupBox {
                        Layout.fillWidth: true
                        title: "Acquisition"
                        
                        background: Rectangle {
                            color: "transparent"
                            border.color: "#2a3f4a"
                            border.width: 1
                            radius: 3
                        }
                        
                        label: Label {
                            text: parent.title
                            color: "#9ca3af"
                            font.pixelSize: 12
                            font.bold: true
                        }
                        
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                
                                Button {
                                    id: startButton
                                    text: "Start"
                                    Layout.fillWidth: true
                                    enabled: root.isConnected && !root.isAcquiring
                                    
                                    background: Rectangle {
                                        color: {
                                            if (!startButton.enabled) return "#1a1f2a"
                                            if (startButton.pressed) return "#2a7030"
                                            if (startButton.hovered) return "#3a8040"
                                            return "#2a6030"
                                        }
                                        radius: 3
                                    }
                                    
                                    contentItem: Text {
                                        text: startButton.text
                                        color: startButton.enabled ? "#ffffff" : "#6a6f7a"
                                        font.pixelSize: 13
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    
                                    onClicked: cameraBridge.startAcquisition()
                                }
                                
                                Button {
                                    id: stopButton
                                    text: "Stop"
                                    Layout.fillWidth: true
                                    enabled: root.isConnected && root.isAcquiring
                                    
                                    background: Rectangle {
                                        color: {
                                            if (!stopButton.enabled) return "#1a1f2a"
                                            if (stopButton.pressed) return "#7a3030"
                                            if (stopButton.hovered) return "#8a4040"
                                            return "#6a3030"
                                        }
                                        radius: 3
                                    }
                                    
                                    contentItem: Text {
                                        text: stopButton.text
                                        color: stopButton.enabled ? "#ffffff" : "#6a6f7a"
                                        font.pixelSize: 13
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    
                                    onClicked: cameraBridge.stopAcquisition()
                                }
                            }
                            
                            Button {
                                id: captureButton
                                text: "Capture"
                                Layout.fillWidth: true
                                enabled: root.isConnected && root.isAcquiring
                                
                                background: Rectangle {
                                    color: {
                                        if (!captureButton.enabled) return "#1a1f2a"
                                        if (captureButton.pressed) return "#4a5090"
                                        if (captureButton.hovered) return "#5a60a0"
                                        return "#3a4080"
                                    }
                                    radius: 3
                                }
                                
                                contentItem: Text {
                                    text: captureButton.text
                                    color: captureButton.enabled ? "#ffffff" : "#6a6f7a"
                                    font.pixelSize: 13
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: cameraBridge.captureImage()
                            }
                        }
                    }
                    
                    // Camera parameters
                    GroupBox {
                        Layout.fillWidth: true
                        title: "Parameters"
                        
                        background: Rectangle {
                            color: "transparent"
                            border.color: "#2a3f4a"
                            border.width: 1
                            radius: 3
                        }
                        
                        label: Label {
                            text: parent.title
                            color: "#9ca3af"
                            font.pixelSize: 12
                            font.bold: true
                        }
                        
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10
                            
                            // Exposure control
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    
                                    Label {
                                        text: "Exposure (μs):"
                                        color: "#9ca3af"
                                        font.pixelSize: 11
                                    }
                                    
                                    Item { Layout.fillWidth: true }
                                    
                                    Label {
                                        text: exposureSlider.value.toFixed(0)
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.family: "Consolas, Monaco, monospace"
                                    }
                                }
                                
                                Slider {
                                    id: exposureSlider
                                    Layout.fillWidth: true
                                    from: 100
                                    to: 100000
                                    value: 10000
                                    enabled: root.isConnected
                                    
                                    background: Rectangle {
                                        x: exposureSlider.leftPadding
                                        y: exposureSlider.topPadding + exposureSlider.availableHeight / 2 - height / 2
                                        width: exposureSlider.availableWidth
                                        height: 4
                                        radius: 2
                                        color: "#2a3f4a"
                                        
                                        Rectangle {
                                            width: exposureSlider.visualPosition * parent.width
                                            height: parent.height
                                            color: exposureSlider.enabled ? "#4a9eff" : "#3a4f5a"
                                            radius: 2
                                        }
                                    }
                                    
                                    handle: Rectangle {
                                        x: exposureSlider.leftPadding + exposureSlider.visualPosition * exposureSlider.availableWidth - width / 2
                                        y: exposureSlider.topPadding + exposureSlider.availableHeight / 2 - height / 2
                                        width: 16
                                        height: 16
                                        radius: 8
                                        color: exposureSlider.pressed ? "#6abfff" : (exposureSlider.enabled ? "#4a9eff" : "#3a4f5a")
                                    }
                                    
                                    onValueChanged: {
                                        if (cameraBridge && root.isConnected) {
                                            cameraBridge.setExposure(value)
                                        }
                                    }
                                }
                            }
                            
                            // Gain control
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                
                                RowLayout {
                                    Layout.fillWidth: true
                                    
                                    Label {
                                        text: "Gain (dB):"
                                        color: "#9ca3af"
                                        font.pixelSize: 11
                                    }
                                    
                                    Item { Layout.fillWidth: true }
                                    
                                    Label {
                                        text: gainSlider.value.toFixed(1)
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.family: "Consolas, Monaco, monospace"
                                    }
                                }
                                
                                Slider {
                                    id: gainSlider
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 48
                                    value: 0
                                    enabled: root.isConnected
                                    
                                    background: Rectangle {
                                        x: gainSlider.leftPadding
                                        y: gainSlider.topPadding + gainSlider.availableHeight / 2 - height / 2
                                        width: gainSlider.availableWidth
                                        height: 4
                                        radius: 2
                                        color: "#2a3f4a"
                                        
                                        Rectangle {
                                            width: gainSlider.visualPosition * parent.width
                                            height: parent.height
                                            color: gainSlider.enabled ? "#4a9eff" : "#3a4f5a"
                                            radius: 2
                                        }
                                    }
                                    
                                    handle: Rectangle {
                                        x: gainSlider.leftPadding + gainSlider.visualPosition * gainSlider.availableWidth - width / 2
                                        y: gainSlider.topPadding + gainSlider.availableHeight / 2 - height / 2
                                        width: 16
                                        height: 16
                                        radius: 8
                                        color: gainSlider.pressed ? "#6abfff" : (gainSlider.enabled ? "#4a9eff" : "#3a4f5a")
                                    }
                                    
                                    onValueChanged: {
                                        if (cameraBridge && root.isConnected) {
                                            cameraBridge.setGain(value)
                                        }
                                    }
                                }
                            }
                            
                            // Frame rate control
                            RowLayout {
                                Layout.fillWidth: true
                                
                                Label {
                                    text: "Target FPS:"
                                    color: "#9ca3af"
                                    font.pixelSize: 11
                                }
                                
                                SpinBox {
                                    id: fpsSpinBox
                                    Layout.fillWidth: true
                                    from: 1
                                    to: 1000
                                    value: 30
                                    enabled: root.isConnected
                                    
                                    background: Rectangle {
                                        color: fpsSpinBox.enabled ? "#2a2f3a" : "#1a1f2a"
                                        border.color: fpsSpinBox.hovered ? "#4a9eff" : "#3a3f4a"
                                        border.width: 1
                                        radius: 3
                                    }
                                    
                                    contentItem: TextInput {
                                        text: fpsSpinBox.value
                                        color: fpsSpinBox.enabled ? "#ffffff" : "#6a6f7a"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        readOnly: !fpsSpinBox.editable
                                    }
                                    
                                    onValueChanged: {
                                        if (cameraBridge && root.isConnected) {
                                            cameraBridge.setFrameRate(value)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                    
                    // Statistics
                    GroupBox {
                        Layout.fillWidth: true
                        title: "Statistics"
                        
                        background: Rectangle {
                            color: "transparent"
                            border.color: "#2a3f4a"
                            border.width: 1
                            radius: 3
                        }
                        
                        label: Label {
                            text: parent.title
                            color: "#9ca3af"
                            font.pixelSize: 12
                            font.bold: true
                        }
                        
                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: 10
                            
                            Label {
                                text: "Current FPS:"
                                color: "#9ca3af"
                                font.pixelSize: 11
                            }
                            Label {
                                text: root.currentFps.toFixed(1)
                                color: "#40ff40"
                                font.pixelSize: 11
                                font.family: "Consolas, Monaco, monospace"
                            }
                            
                            Label {
                                text: "Average FPS:"
                                color: "#9ca3af"
                                font.pixelSize: 11
                            }
                            Label {
                                text: root.averageFps.toFixed(1)
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.family: "Consolas, Monaco, monospace"
                            }
                            
                            Label {
                                text: "Total Frames:"
                                color: "#9ca3af"
                                font.pixelSize: 11
                            }
                            Label {
                                text: root.frameCount
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.family: "Consolas, Monaco, monospace"
                            }
                            
                            Label {
                                text: "Dropped:"
                                color: "#9ca3af"
                                font.pixelSize: 11
                            }
                            Label {
                                text: root.droppedFrames
                                color: root.droppedFrames > 0 ? "#ff4040" : "#40ff40"
                                font.pixelSize: 11
                                font.family: "Consolas, Monaco, monospace"
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Connect to camera bridge signals
    Connections {
        target: cameraBridge
        
        function onConnectionChanged(connected) {
            root.isConnected = connected
            if (!connected) {
                root.isAcquiring = false
                root.frameCount = 0
                root.currentFps = 0
                root.averageFps = 0
                root.droppedFrames = 0
                fpsTimer.lastFrameCount = 0
            }
        }
        
        function onAcquisitionStateChanged(acquiring) {
            root.isAcquiring = acquiring
            if (acquiring) {
                root.frameCount = 0
                root.droppedFrames = 0
                root.lastFrameTime = Date.now()
                fpsTimer.lastFrameCount = 0
            }
        }
        
        function onFrameReceived() {
            root.frameCount++
            if (root.frameCount === 1) {
                root.lastFrameTime = Date.now()
            }
        }
        
        function onFrameDropped() {
            root.droppedFrames++
        }
        
        function onErrorOccurred(error) {
            console.error("Camera error:", error)
        }
    }
    
    Component.onCompleted: {
        if (cameraBridge) {
            cameraBridge.refreshCameraList()
        }
    }
}