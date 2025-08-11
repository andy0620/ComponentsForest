import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Item {
    id: root
    height: 80
    
    property var cameraInfo: null
    property bool isSelected: false
    
    signal clicked()
    signal removeRequested()
    signal startRequested()
    signal stopRequested()
    
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 5
        color: isSelected ? Material.color(Material.Blue, Material.Shade900) : 
               mouseArea.containsMouse ? Material.color(Material.Grey, Material.Shade900) : 
               "#1a1f2e"
        border.width: isSelected ? 2 : 0
        border.color: Material.accent
        
        // Smooth transitions
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.clicked()
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            
            // Camera status indicator
            Rectangle {
                width: 50
                height: 50
                radius: 25
                color: {
                    if (!cameraInfo) return Material.color(Material.Grey, Material.Shade800)
                    if (cameraInfo.isAcquiring) return Material.color(Material.Green, Material.Shade700)
                    if (cameraInfo.isConnected) return Material.color(Material.Yellow, Material.Shade700)
                    return Material.color(Material.Grey, Material.Shade800)
                }
                
                Label {
                    anchors.centerIn: parent
                    text: "CAM"
                    font.pixelSize: 12
                    font.bold: true
                    color: "white"
                }
                
                // Pulsing animation when acquiring
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width + 10
                    height: parent.height + 10
                    radius: (width / 2)
                    color: "transparent"
                    border.width: 2
                    border.color: Material.color(Material.Green)
                    opacity: 0
                    visible: cameraInfo && cameraInfo.isAcquiring
                    
                    SequentialAnimation on opacity {
                        running: visible
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.5; duration: 1000 }
                        NumberAnimation { to: 0; duration: 1000 }
                    }
                    
                    SequentialAnimation on scale {
                        running: visible
                        loops: Animation.Infinite
                        NumberAnimation { to: 1.2; duration: 1000 }
                        NumberAnimation { to: 1.0; duration: 1000 }
                    }
                }
            }
            
            // Camera information
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                
                Label {
                    text: cameraInfo ? cameraInfo.id : "Unknown Camera"
                    font.pixelSize: 14
                    font.bold: true
                    color: Material.foreground
                }
                
                Label {
                    text: {
                        if (!cameraInfo) return "No information"
                        if (cameraInfo.deviceName) return cameraInfo.deviceName
                        return "Not connected"
                    }
                    font.pixelSize: 11
                    color: Material.color(Material.Grey, Material.Shade400)
                }
                
                RowLayout {
                    spacing: 5
                    
                    // Status badges
                    Rectangle {
                        width: 60
                        height: 18
                        radius: 9
                        color: cameraInfo && cameraInfo.isConnected ? 
                               Material.color(Material.Green, Material.Shade900) : 
                               Material.color(Material.Red, Material.Shade900)
                        
                        Label {
                            anchors.centerIn: parent
                            text: cameraInfo && cameraInfo.isConnected ? "Online" : "Offline"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                    
                    Rectangle {
                        width: 80
                        height: 18
                        radius: 9
                        color: Material.color(Material.Blue, Material.Shade900)
                        visible: cameraInfo && cameraInfo.isAcquiring
                        
                        Label {
                            anchors.centerIn: parent
                            text: cameraInfo ? (cameraInfo.fps || 0).toFixed(1) + " fps" : "0 fps"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                    
                    Rectangle {
                        width: 60
                        height: 18
                        radius: 9
                        color: Material.color(Material.Purple, Material.Shade900)
                        visible: cameraInfo && cameraInfo.frameCount > 0
                        
                        Label {
                            anchors.centerIn: parent
                            text: cameraInfo ? cameraInfo.frameCount.toString() : "0"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }
            }
            
            // Action buttons
            Row {
                spacing: 5
                
                Button {
                    width: 32
                    height: 32
                    flat: true
                    visible: cameraInfo && cameraInfo.isConnected
                    icon.source: cameraInfo && cameraInfo.isAcquiring ? 
                                 "qrc:/icons/stop.png" : "qrc:/icons/play.png"
                    ToolTip.text: cameraInfo && cameraInfo.isAcquiring ? "Stop" : "Start"
                    
                    onClicked: {
                        if (cameraInfo && cameraInfo.isAcquiring) {
                            root.stopRequested()
                        } else {
                            root.startRequested()
                        }
                    }
                    
                    Material.background: cameraInfo && cameraInfo.isAcquiring ? 
                                        Material.color(Material.Red, Material.Shade800) : 
                                        Material.color(Material.Green, Material.Shade800)
                }
                
                Button {
                    width: 32
                    height: 32
                    flat: true
                    text: "×"
                    font.pixelSize: 20
                    ToolTip.text: "Remove camera"
                    onClicked: root.removeRequested()
                    Material.background: Material.color(Material.Red, Material.Shade900)
                }
            }
        }
        
        // Selection highlight
        Rectangle {
            anchors.fill: parent
            radius: 5
            color: "transparent"
            border.width: 2
            border.color: Material.accent
            opacity: isSelected ? 1.0 : 0.0
            
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }
    }
}