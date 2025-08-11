import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property var bridge: null
    property bool isConnected: false
    property bool isAcquiring: false
    property bool isRecording: false
    property int frameCount: 0
    property real fps: 0.0
    property int recordedFrames: 0
    property string recordingPath: ""
    
    // Signals
    signal startAcquisitionRequested()
    signal stopAcquisitionRequested()
    signal captureFrameRequested()
    signal startRecordingRequested(string path)
    signal stopRecordingRequested()
    
    implicitWidth: 320
    implicitHeight: 240
    color: "#1A1E2E"
    radius: 8
    border.color: "#2A2E3E"
    border.width: 1
    
    // Acquisition state management
    states: [
        State {
            name: "idle"
            when: !isAcquiring && isConnected
            PropertyChanges {
                target: acquisitionBtn
                text: "Start Acquisition"
                iconColor: "#00D084"
                enabled: true
            }
            PropertyChanges {
                target: captureBtn
                enabled: false
            }
            PropertyChanges {
                target: recordBtn
                enabled: false
            }
        },
        State {
            name: "acquiring"
            when: isAcquiring && isConnected
            PropertyChanges {
                target: acquisitionBtn
                text: "Stop Acquisition"
                iconColor: "#FF6B6B"
                enabled: true
            }
            PropertyChanges {
                target: captureBtn
                enabled: true
            }
            PropertyChanges {
                target: recordBtn
                enabled: true
            }
        },
        State {
            name: "disconnected"
            when: !isConnected
            PropertyChanges {
                target: acquisitionBtn
                text: "Start Acquisition"
                enabled: false
            }
            PropertyChanges {
                target: captureBtn
                enabled: false
            }
            PropertyChanges {
                target: recordBtn
                enabled: false
            }
        }
    ]
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        
        // Header
        Label {
            text: "Acquisition Controls"
            font.pixelSize: 14
            font.weight: Font.Medium
            color: "#E8EAED"
        }
        
        // Main acquisition button
        Button {
            id: acquisitionBtn
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            enabled: false
            
            property color iconColor: "#00D084"
            
            background: Rectangle {
                color: {
                    if (!acquisitionBtn.enabled) return "#1A1E2E"
                    if (isAcquiring) {
                        return acquisitionBtn.hovered ? "#FF8B8B" : "#FF6B6B"
                    } else {
                        return acquisitionBtn.hovered ? "#00E894" : "#00D084"
                    }
                }
                border.color: {
                    if (!acquisitionBtn.enabled) return "#2A2E3E"
                    return isAcquiring ? "#FF9B9B" : "#00FFA0"
                }
                border.width: 1
                radius: 6
                
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }
            
            contentItem: RowLayout {
                spacing: 8
                
                Item { Layout.fillWidth: true }
                
                // Play/Stop icon
                Item {
                    width: 16
                    height: 16
                    
                    // Play icon
                    Canvas {
                        anchors.fill: parent
                        visible: !isAcquiring
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.fillStyle = acquisitionBtn.enabled ? "#1A1E2E" : "#545A6E"
                            ctx.beginPath()
                            ctx.moveTo(4, 2)
                            ctx.lineTo(14, 8)
                            ctx.lineTo(4, 14)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }
                    
                    // Stop icon
                    Rectangle {
                        width: 12
                        height: 12
                        anchors.centerIn: parent
                        visible: isAcquiring
                        color: "#1A1E2E"
                        radius: 2
                    }
                }
                
                Text {
                    text: acquisitionBtn.text
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: acquisitionBtn.enabled ? "#1A1E2E" : "#545A6E"
                }
                
                Item { Layout.fillWidth: true }
            }
            
            onClicked: {
                if (isAcquiring) {
                    stopAcquisitionRequested()
                } else {
                    startAcquisitionRequested()
                }
            }
        }
        
        // Status display
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#242838"
            radius: 4
            border.color: "#3A3E4E"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16
                
                // FPS indicator
                RowLayout {
                    spacing: 4
                    
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: isAcquiring ? "#00D084" : "#545A6E"
                        
                        SequentialAnimation on opacity {
                            running: isAcquiring
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                    }
                    
                    Label {
                        text: "FPS:"
                        font.pixelSize: 11
                        color: "#A0A4B0"
                    }
                    
                    Label {
                        text: fps.toFixed(1)
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        color: "#E8EAED"
                        Layout.minimumWidth: 35
                    }
                }
                
                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#3A3E4E"
                }
                
                // Frame counter
                RowLayout {
                    spacing: 4
                    
                    Label {
                        text: "Frames:"
                        font.pixelSize: 11
                        color: "#A0A4B0"
                    }
                    
                    Label {
                        text: frameCount.toString()
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        color: "#E8EAED"
                        Layout.minimumWidth: 50
                    }
                }
                
                Item { Layout.fillWidth: true }
            }
        }
        
        // Secondary controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            // Capture frame button
            Button {
                id: captureBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                enabled: false
                
                background: Rectangle {
                    color: captureBtn.enabled ? 
                           (captureBtn.pressed ? "#3A3E4E" : 
                            (captureBtn.hovered ? "#2E3244" : "#242838")) : 
                           "#1A1E2E"
                    border.color: captureBtn.enabled ? "#545A6E" : "#2A2E3E"
                    border.width: 1
                    radius: 4
                    
                    Behavior on color {
                        ColorAnimation { duration: 100 }
                    }
                }
                
                contentItem: RowLayout {
                    spacing: 6
                    
                    Item { Layout.fillWidth: true }
                    
                    // Camera icon
                    Item {
                        width: 14
                        height: 14
                        
                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.color: captureBtn.enabled ? "#B0B4C0" : "#545A6E"
                            border.width: 2
                            radius: 2
                            
                            Rectangle {
                                anchors.centerIn: parent
                                width: 6
                                height: 6
                                color: "transparent"
                                border.color: captureBtn.enabled ? "#B0B4C0" : "#545A6E"
                                border.width: 2
                                radius: 3
                            }
                        }
                    }
                    
                    Text {
                        text: "Capture"
                        font.pixelSize: 12
                        color: captureBtn.enabled ? "#E8EAED" : "#545A6E"
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                onClicked: {
                    captureFrameRequested()
                    // Visual feedback
                    captureFlash.start()
                }
            }
            
            // Record button
            Button {
                id: recordBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                enabled: false
                
                background: Rectangle {
                    color: {
                        if (!recordBtn.enabled) return "#1A1E2E"
                        if (isRecording) {
                            return recordBtn.hovered ? "#8B3A3A" : "#6B2A2A"
                        } else {
                            return recordBtn.hovered ? "#2E3244" : "#242838"
                        }
                    }
                    border.color: {
                        if (!recordBtn.enabled) return "#2A2E3E"
                        return isRecording ? "#FF6B6B" : "#545A6E"
                    }
                    border.width: 1
                    radius: 4
                    
                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
                }
                
                contentItem: RowLayout {
                    spacing: 6
                    
                    Item { Layout.fillWidth: true }
                    
                    // Record icon
                    Rectangle {
                        width: 12
                        height: 12
                        radius: isRecording ? 2 : 6
                        color: isRecording ? "#FF6B6B" : 
                               (recordBtn.enabled ? "#FF6B6B" : "#545A6E")
                        
                        SequentialAnimation on opacity {
                            running: isRecording
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.5; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                        
                        Behavior on radius {
                            NumberAnimation { duration: 150 }
                        }
                    }
                    
                    Text {
                        text: isRecording ? "Stop" : "Record"
                        font.pixelSize: 12
                        color: recordBtn.enabled ? "#E8EAED" : "#545A6E"
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                onClicked: {
                    if (isRecording) {
                        stopRecordingRequested()
                    } else {
                        // In real implementation, would show file dialog
                        var timestamp = new Date().toISOString().replace(/[:.]/g, '-')
                        var path = "/recordings/capture_" + timestamp + ".avi"
                        startRecordingRequested(path)
                    }
                }
            }
        }
        
        // Recording status
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "transparent"
            visible: isRecording
            
            RowLayout {
                anchors.fill: parent
                spacing: 8
                
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: "#FF6B6B"
                    
                    SequentialAnimation on opacity {
                        running: isRecording
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 1000 }
                        NumberAnimation { to: 1.0; duration: 1000 }
                    }
                }
                
                Label {
                    text: "Recording:"
                    font.pixelSize: 11
                    color: "#FF6B6B"
                }
                
                Label {
                    text: recordedFrames + " frames"
                    font.pixelSize: 11
                    color: "#E8EAED"
                    Layout.fillWidth: true
                }
                
                Label {
                    text: formatDuration(recordedFrames / Math.max(fps, 1))
                    font.pixelSize: 11
                    font.family: "monospace"
                    color: "#A0A4B0"
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
    
    // Capture flash effect
    Rectangle {
        id: captureFlashRect
        anchors.fill: parent
        color: "white"
        opacity: 0
        radius: 8
        
        SequentialAnimation {
            id: captureFlash
            NumberAnimation {
                target: captureFlashRect
                property: "opacity"
                to: 0.3
                duration: 50
            }
            NumberAnimation {
                target: captureFlashRect
                property: "opacity"
                to: 0
                duration: 150
            }
        }
    }
    
    // Helper function to format duration
    function formatDuration(seconds) {
        var mins = Math.floor(seconds / 60)
        var secs = Math.floor(seconds % 60)
        return mins.toString().padStart(2, '0') + ":" + 
               secs.toString().padStart(2, '0')
    }
    
    // Bridge connections
    Connections {
        target: bridge
        enabled: bridge !== null
        
        function onConnectionStateChanged(connected) {
            isConnected = connected
            if (!connected) {
                isAcquiring = false
                isRecording = false
            }
        }
        
        function onAcquisitionStateChanged(acquiring) {
            isAcquiring = acquiring
            if (!acquiring) {
                isRecording = false
                frameCount = 0
                fps = 0
            }
        }
        
        function onFrameReceived(metadata) {
            frameCount++
            if (metadata && metadata.fps) {
                fps = metadata.fps
            }
            if (isRecording) {
                recordedFrames++
            }
        }
        
        function onRecordingStateChanged(recording, path) {
            isRecording = recording
            recordingPath = path
            if (recording) {
                recordedFrames = 0
            }
        }
    }
    
    // Simulated FPS counter for demo
    Timer {
        interval: 100
        running: isAcquiring
        repeat: true
        onTriggered: {
            if (isAcquiring) {
                // Simulate varying FPS
                fps = 25 + Math.random() * 10
                frameCount++
                if (isRecording) {
                    recordedFrames++
                }
            }
        }
    }
}