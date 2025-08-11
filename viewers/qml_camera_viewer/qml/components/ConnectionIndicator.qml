import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Rectangle {
    id: root
    width: 120
    height: 40
    radius: 20
    color: statusColor
    
    property bool isConnected: false
    property bool isAcquiring: false
    
    readonly property color statusColor: {
        if (isAcquiring) return Material.color(Material.Green, Material.Shade800)
        if (isConnected) return Material.color(Material.Yellow, Material.Shade800)
        return Material.color(Material.Grey, Material.Shade800)
    }
    
    readonly property string statusText: {
        if (isAcquiring) return "Acquiring"
        if (isConnected) return "Connected"
        return "Disconnected"
    }
    
    RowLayout {
        anchors.centerIn: parent
        spacing: 8
        
        // Status indicator dot
        Rectangle {
            width: 10
            height: 10
            radius: 5
            color: {
                if (isAcquiring) return Material.color(Material.Green)
                if (isConnected) return Material.color(Material.Yellow)
                return Material.color(Material.Grey)
            }
            
            // Pulsing animation when acquiring
            SequentialAnimation on opacity {
                running: isAcquiring
                loops: Animation.Infinite
                NumberAnimation { to: 0.3; duration: 500; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 1.0; duration: 500; easing.type: Easing.InOutQuad }
            }
            
            // Glowing effect when connected
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 2
                height: parent.height * 2
                radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: parent.color
                opacity: 0.3
                visible: isConnected || isAcquiring
                
                SequentialAnimation on scale {
                    running: isConnected && !isAcquiring
                    loops: Animation.Infinite
                    NumberAnimation { to: 1.5; duration: 1500; easing.type: Easing.OutQuad }
                    NumberAnimation { to: 1.0; duration: 0 }
                }
                
                SequentialAnimation on opacity {
                    running: isConnected && !isAcquiring
                    loops: Animation.Infinite
                    NumberAnimation { to: 0; duration: 1500; easing.type: Easing.OutQuad }
                    NumberAnimation { to: 0.3; duration: 0 }
                }
            }
        }
        
        // Status text
        Label {
            text: statusText
            font.pixelSize: 12
            font.bold: true
            color: Material.foreground
        }
    }
    
    // Smooth color transitions
    Behavior on color {
        ColorAnimation { duration: 200 }
    }
}