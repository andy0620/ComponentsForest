import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    
    property var bridge: null
    property bool isConnected: false
    property bool isScanning: false
    property var availableDevices: []
    property string currentDevice: ""
    
    // Signals
    signal scanRequested()
    signal connectRequested(string deviceName)
    signal disconnectRequested()
    
    implicitWidth: 320
    implicitHeight: 160
    color: "#1A1E2E"
    radius: 8
    border.color: "#2A2E3E"
    border.width: 1
    
    // Connection state management
    states: [
        State {
            name: "disconnected"
            when: !isConnected
            PropertyChanges {
                target: statusIndicator
                color: "#545A6E"
            }
            PropertyChanges {
                target: statusText
                text: "Disconnected"
            }
            PropertyChanges {
                target: connectBtn
                text: "Connect"
                enabled: deviceCombo.currentIndex >= 0 && !isScanning
            }
        },
        State {
            name: "connected"
            when: isConnected
            PropertyChanges {
                target: statusIndicator
                color: "#00D084"
            }
            PropertyChanges {
                target: statusText
                text: "Connected"
            }
            PropertyChanges {
                target: connectBtn
                text: "Disconnect"
                enabled: true
            }
        }
    ]
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        
        // Header with status
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Label {
                text: "Camera Connection"
                font.pixelSize: 14
                font.weight: Font.Medium
                color: "#E8EAED"
            }
            
            Item { Layout.fillWidth: true }
            
            // Connection status indicator
            Rectangle {
                id: statusIndicator
                width: 8
                height: 8
                radius: 4
                color: "#545A6E"
                
                Behavior on color {
                    ColorAnimation { duration: 200 }
                }
            }
            
            Label {
                id: statusText
                text: "Disconnected"
                font.pixelSize: 12
                color: "#A0A4B0"
            }
        }
        
        // Device selection row
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Label {
                text: "Device:"
                font.pixelSize: 12
                color: "#B0B4C0"
                Layout.minimumWidth: 50
            }
            
            ComboBox {
                id: deviceCombo
                Layout.fillWidth: true
                enabled: !isConnected && !isScanning
                model: availableDevices.length > 0 ? availableDevices : ["No devices found"]
                
                background: Rectangle {
                    color: deviceCombo.enabled ? "#242838" : "#1A1E2E"
                    border.color: deviceCombo.hovered ? "#00D084" : "#3A3E4E"
                    border.width: 1
                    radius: 4
                    
                    Behavior on border.color {
                        ColorAnimation { duration: 150 }
                    }
                }
                
                contentItem: Text {
                    text: deviceCombo.displayText
                    font.pixelSize: 12
                    color: deviceCombo.enabled ? "#E8EAED" : "#545A6E"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    leftPadding: 8
                }
                
                delegate: ItemDelegate {
                    width: deviceCombo.width
                    height: 32
                    
                    background: Rectangle {
                        color: hovered ? "#2A2E3E" : "transparent"
                    }
                    
                    contentItem: Text {
                        text: modelData
                        font.pixelSize: 12
                        color: "#E8EAED"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                    }
                }
                
                popup: Popup {
                    y: deviceCombo.height
                    width: deviceCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1
                    
                    background: Rectangle {
                        color: "#242838"
                        border.color: "#3A3E4E"
                        border.width: 1
                        radius: 4
                    }
                    
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: deviceCombo.popup.visible ? deviceCombo.delegateModel : null
                        currentIndex: deviceCombo.highlightedIndex
                    }
                }
                
                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && availableDevices.length > 0) {
                        currentDevice = availableDevices[currentIndex]
                    }
                }
            }
        }
        
        // Control buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Button {
                id: scanBtn
                text: isScanning ? "Scanning..." : "Scan"
                Layout.fillWidth: true
                enabled: !isConnected && !isScanning
                
                background: Rectangle {
                    color: scanBtn.enabled ? 
                           (scanBtn.hovered ? "#3A3E4E" : "#2A2E3E") : 
                           "#1A1E2E"
                    border.color: scanBtn.enabled ? "#545A6E" : "#2A2E3E"
                    border.width: 1
                    radius: 4
                    
                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
                }
                
                contentItem: RowLayout {
                    spacing: 6
                    
                    Item {
                        Layout.fillWidth: true
                    }
                    
                    // Scan icon
                    Item {
                        width: 16
                        height: 16
                        visible: !isScanning
                        
                        Rectangle {
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            color: "transparent"
                            border.color: scanBtn.enabled ? "#B0B4C0" : "#545A6E"
                            border.width: 2
                            radius: 7
                            
                            Rectangle {
                                anchors.centerIn: parent
                                width: 6
                                height: 6
                                color: scanBtn.enabled ? "#B0B4C0" : "#545A6E"
                                radius: 3
                            }
                        }
                    }
                    
                    // Scanning indicator
                    BusyIndicator {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        running: isScanning
                        visible: isScanning
                        
                        contentItem: Item {
                            Rectangle {
                                width: parent.width
                                height: parent.height
                                color: "transparent"
                                border.color: "#00D084"
                                border.width: 2
                                radius: width / 2
                                
                                Rectangle {
                                    x: parent.width / 2 - 2
                                    y: 2
                                    width: 4
                                    height: 4
                                    color: "#00D084"
                                    radius: 2
                                }
                                
                                RotationAnimator {
                                    target: parent
                                    from: 0
                                    to: 360
                                    duration: 1000
                                    loops: Animation.Infinite
                                    running: isScanning
                                }
                            }
                        }
                    }
                    
                    Text {
                        text: scanBtn.text
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: scanBtn.enabled ? "#E8EAED" : "#545A6E"
                    }
                    
                    Item {
                        Layout.fillWidth: true
                    }
                }
                
                onClicked: {
                    isScanning = true
                    scanRequested()
                    
                    // Simulate scan completion after delay
                    scanTimer.restart()
                }
            }
            
            Button {
                id: connectBtn
                text: "Connect"
                Layout.fillWidth: true
                enabled: false
                
                background: Rectangle {
                    color: {
                        if (!connectBtn.enabled) return "#1A1E2E"
                        if (isConnected) {
                            return connectBtn.hovered ? "#8B3A3A" : "#6B2A2A"
                        } else {
                            return connectBtn.hovered ? "#00E894" : "#00D084"
                        }
                    }
                    border.color: {
                        if (!connectBtn.enabled) return "#2A2E3E"
                        if (isConnected) {
                            return "#A04040"
                        } else {
                            return "#00FFA0"
                        }
                    }
                    border.width: 1
                    radius: 4
                    
                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
                }
                
                contentItem: Text {
                    text: connectBtn.text
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: connectBtn.enabled ? 
                           (isConnected ? "#FFD0D0" : "#1A1E2E") : 
                           "#545A6E"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    if (isConnected) {
                        disconnectRequested()
                        isConnected = false
                    } else if (currentDevice) {
                        connectRequested(currentDevice)
                        isConnected = true
                    }
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
    
    // Scan simulation timer
    Timer {
        id: scanTimer
        interval: 2000
        onTriggered: {
            isScanning = false
            // In real implementation, this would be populated by the bridge
            availableDevices = ["Camera_001", "Camera_002", "Do3Think_HD"]
            if (availableDevices.length > 0) {
                deviceCombo.currentIndex = 0
            }
        }
    }
    
    // Bridge connections
    Connections {
        target: bridge
        enabled: bridge !== null
        
        function onDeviceListUpdated(devices) {
            availableDevices = devices
            isScanning = false
        }
        
        function onConnectionStateChanged(connected) {
            isConnected = connected
        }
    }
}