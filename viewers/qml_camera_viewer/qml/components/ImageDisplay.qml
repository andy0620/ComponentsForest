import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    // Public properties
    property alias source: imageItem.source
    property bool crosshairVisible: false
    property bool gridVisible: false
    property bool roiSelectionEnabled: false
    property real zoomLevel: 1.0
    property real minZoom: 0.1
    property real maxZoom: 10.0
    property real zoomStep: 0.1
    property color crosshairColor: "#00FF00"
    property color gridColor: "#404040"
    property color roiColor: "#FF0000"
    property int gridSpacing: 50
    property bool hasImage: false
    
    // ROI properties
    property rect selectedRoi: Qt.rect(0, 0, 0, 0)
    
    // Signals
    signal roiSelected(rect roi)
    signal zoomChanged(real zoom)
    signal imageClicked(point pos)
    signal imageDoubleClicked(point pos)
    
    // Background
    Rectangle {
        anchors.fill: parent
        color: "#1A1A1A"
    }
    
    // Scrollable container for panning
    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: imageContainer.width
        contentHeight: imageContainer.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        
        // Center content when smaller than viewport
        contentX: contentWidth > width ? contentX : (contentWidth - width) / 2
        contentY: contentHeight > height ? contentY : (contentHeight - height) / 2
        
        Item {
            id: imageContainer
            width: imageItem.width * imageItem.scale
            height: imageItem.height * imageItem.scale
            
            // Main image display
            Image {
                id: imageItem
                anchors.centerIn: parent
                cache: false
                smooth: true
                antialiasing: true
                fillMode: Image.PreserveAspectFit
                scale: root.zoomLevel
                transformOrigin: Item.Center
                
                onStatusChanged: {
                    root.hasImage = (status === Image.Ready && source != "")
                }
                
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    
                    property point startPoint
                    property bool selecting: false
                    
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            root.imageClicked(Qt.point(mouse.x / imageItem.scale, 
                                                      mouse.y / imageItem.scale))
                        } else if (mouse.button === Qt.RightButton) {
                            contextMenu.popup()
                        }
                    }
                    
                    onDoubleClicked: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            root.imageDoubleClicked(Qt.point(mouse.x / imageItem.scale, 
                                                            mouse.y / imageItem.scale))
                            // Auto zoom to point
                            if (root.zoomLevel < 2.0) {
                                root.zoomTo(2.0, mouse.x, mouse.y)
                            } else {
                                root.fitToWindow()
                            }
                        }
                    }
                    
                    onPressed: (mouse) => {
                        if (root.roiSelectionEnabled && mouse.button === Qt.LeftButton) {
                            startPoint = Qt.point(mouse.x, mouse.y)
                            selecting = true
                            roiRect.x = mouse.x
                            roiRect.y = mouse.y
                            roiRect.width = 0
                            roiRect.height = 0
                            roiRect.visible = true
                        }
                    }
                    
                    onPositionChanged: (mouse) => {
                        if (selecting) {
                            var currentPoint = Qt.point(mouse.x, mouse.y)
                            roiRect.x = Math.min(startPoint.x, currentPoint.x)
                            roiRect.y = Math.min(startPoint.y, currentPoint.y)
                            roiRect.width = Math.abs(currentPoint.x - startPoint.x)
                            roiRect.height = Math.abs(currentPoint.y - startPoint.y)
                        }
                    }
                    
                    onReleased: (mouse) => {
                        if (selecting) {
                            selecting = false
                            if (roiRect.width > 5 && roiRect.height > 5) {
                                var roi = Qt.rect(
                                    roiRect.x / imageItem.scale,
                                    roiRect.y / imageItem.scale,
                                    roiRect.width / imageItem.scale,
                                    roiRect.height / imageItem.scale
                                )
                                root.selectedRoi = roi
                                root.roiSelected(roi)
                            }
                            roiRect.visible = false
                        }
                    }
                    
                    onWheel: (wheel) => {
                        var delta = wheel.angleDelta.y / 120
                        if (delta > 0) {
                            root.zoomIn()
                        } else {
                            root.zoomOut()
                        }
                    }
                }
                
                // ROI selection rectangle
                Rectangle {
                    id: roiRect
                    visible: false
                    color: "transparent"
                    border.color: root.roiColor
                    border.width: 2
                    opacity: 0.7
                }
            }
            
            // Grid overlay
            Canvas {
                id: gridCanvas
                anchors.fill: imageItem
                visible: root.gridVisible && root.hasImage
                scale: imageItem.scale
                transformOrigin: Item.Center
                
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = root.gridColor
                    ctx.lineWidth = 1
                    ctx.globalAlpha = 0.5
                    
                    // Vertical lines
                    for (var x = 0; x <= width; x += root.gridSpacing) {
                        ctx.beginPath()
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, height)
                        ctx.stroke()
                    }
                    
                    // Horizontal lines
                    for (var y = 0; y <= height; y += root.gridSpacing) {
                        ctx.beginPath()
                        ctx.moveTo(0, y)
                        ctx.lineTo(width, y)
                        ctx.stroke()
                    }
                }
                
                Connections {
                    target: root
                    function onGridVisibleChanged() { gridCanvas.requestPaint() }
                    function onGridSpacingChanged() { gridCanvas.requestPaint() }
                }
            }
            
            // Crosshair overlay
            Item {
                id: crosshair
                anchors.fill: imageItem
                visible: root.crosshairVisible && root.hasImage
                scale: imageItem.scale
                transformOrigin: Item.Center
                
                Rectangle {
                    width: parent.width
                    height: 1
                    anchors.centerIn: parent
                    color: root.crosshairColor
                    opacity: 0.7
                }
                
                Rectangle {
                    width: 1
                    height: parent.height
                    anchors.centerIn: parent
                    color: root.crosshairColor
                    opacity: 0.7
                }
                
                // Center circle
                Rectangle {
                    width: 20
                    height: 20
                    radius: 10
                    anchors.centerIn: parent
                    color: "transparent"
                    border.color: root.crosshairColor
                    border.width: 1
                    opacity: 0.7
                }
            }
        }
    }
    
    // No signal display
    Rectangle {
        anchors.fill: parent
        color: "#0D0D0D"
        visible: !root.hasImage
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            
            // Camera icon
            Text {
                text: "📷"
                font.pixelSize: 48
                color: "#404040"
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.5
            }
            
            Text {
                text: "No Signal"
                font.pixelSize: 24
                font.weight: Font.Medium
                color: "#606060"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "Waiting for camera input..."
                font.pixelSize: 14
                color: "#404040"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
    
    // Zoom controls overlay
    Row {
        anchors {
            bottom: parent.bottom
            right: parent.right
            margins: 10
        }
        spacing: 5
        
        Button {
            id: zoomOutBtn
            width: 30
            height: 30
            text: "-"
            font.pixelSize: 16
            font.weight: Font.Bold
            
            background: Rectangle {
                color: zoomOutBtn.pressed ? "#404040" : (zoomOutBtn.hovered ? "#303030" : "#202020")
                border.color: "#505050"
                radius: 4
            }
            
            contentItem: Text {
                text: zoomOutBtn.text
                font: zoomOutBtn.font
                color: "#C0C0C0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: root.zoomOut()
        }
        
        Rectangle {
            width: 80
            height: 30
            color: "#202020"
            border.color: "#505050"
            radius: 4
            
            Text {
                anchors.centerIn: parent
                text: Math.round(root.zoomLevel * 100) + "%"
                font.pixelSize: 12
                color: "#C0C0C0"
            }
        }
        
        Button {
            id: zoomInBtn
            width: 30
            height: 30
            text: "+"
            font.pixelSize: 16
            font.weight: Font.Bold
            
            background: Rectangle {
                color: zoomInBtn.pressed ? "#404040" : (zoomInBtn.hovered ? "#303030" : "#202020")
                border.color: "#505050"
                radius: 4
            }
            
            contentItem: Text {
                text: zoomInBtn.text
                font: zoomInBtn.font
                color: "#C0C0C0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: root.zoomIn()
        }
        
        Button {
            id: fitBtn
            width: 30
            height: 30
            text: "⊡"
            font.pixelSize: 16
            
            background: Rectangle {
                color: fitBtn.pressed ? "#404040" : (fitBtn.hovered ? "#303030" : "#202020")
                border.color: "#505050"
                radius: 4
            }
            
            contentItem: Text {
                text: fitBtn.text
                font: fitBtn.font
                color: "#C0C0C0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: root.fitToWindow()
        }
    }
    
    // Status bar
    Rectangle {
        anchors {
            bottom: parent.bottom
            left: parent.left
            margins: 10
        }
        width: statusRow.width + 20
        height: 30
        color: "#202020"
        border.color: "#505050"
        radius: 4
        visible: root.hasImage
        
        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: 15
            
            Text {
                text: "Size: " + imageItem.sourceSize.width + "×" + imageItem.sourceSize.height
                font.pixelSize: 11
                color: "#909090"
            }
            
            Rectangle {
                width: 1
                height: 16
                color: "#505050"
            }
            
            Text {
                text: "Zoom: " + Math.round(root.zoomLevel * 100) + "%"
                font.pixelSize: 11
                color: "#909090"
            }
            
            Rectangle {
                width: 1
                height: 16
                color: "#505050"
                visible: root.selectedRoi.width > 0
            }
            
            Text {
                text: "ROI: " + Math.round(root.selectedRoi.width) + "×" + Math.round(root.selectedRoi.height)
                font.pixelSize: 11
                color: "#909090"
                visible: root.selectedRoi.width > 0
            }
        }
    }
    
    // Context menu
    Menu {
        id: contextMenu
        
        MenuItem {
            text: "Zoom In"
            shortcut: "+"
            onTriggered: root.zoomIn()
        }
        
        MenuItem {
            text: "Zoom Out"
            shortcut: "-"
            onTriggered: root.zoomOut()
        }
        
        MenuItem {
            text: "Fit to Window"
            shortcut: "0"
            onTriggered: root.fitToWindow()
        }
        
        MenuItem {
            text: "Actual Size"
            shortcut: "1"
            onTriggered: root.actualSize()
        }
        
        MenuSeparator {}
        
        MenuItem {
            text: "Show Crosshair"
            checkable: true
            checked: root.crosshairVisible
            onTriggered: root.crosshairVisible = !root.crosshairVisible
        }
        
        MenuItem {
            text: "Show Grid"
            checkable: true
            checked: root.gridVisible
            onTriggered: root.gridVisible = !root.gridVisible
        }
        
        MenuItem {
            text: "Enable ROI Selection"
            checkable: true
            checked: root.roiSelectionEnabled
            onTriggered: root.roiSelectionEnabled = !root.roiSelectionEnabled
        }
        
        MenuSeparator {}
        
        MenuItem {
            text: "Clear ROI"
            enabled: root.selectedRoi.width > 0
            onTriggered: {
                root.selectedRoi = Qt.rect(0, 0, 0, 0)
                roiRect.visible = false
            }
        }
    }
    
    // Keyboard shortcuts
    Shortcut {
        sequence: "+"
        onActivated: root.zoomIn()
    }
    
    Shortcut {
        sequence: "-"
        onActivated: root.zoomOut()
    }
    
    Shortcut {
        sequence: "0"
        onActivated: root.fitToWindow()
    }
    
    Shortcut {
        sequence: "1"
        onActivated: root.actualSize()
    }
    
    Shortcut {
        sequence: "G"
        onActivated: root.gridVisible = !root.gridVisible
    }
    
    Shortcut {
        sequence: "C"
        onActivated: root.crosshairVisible = !root.crosshairVisible
    }
    
    Shortcut {
        sequence: "R"
        onActivated: root.roiSelectionEnabled = !root.roiSelectionEnabled
    }
    
    // Public functions
    function zoomIn() {
        setZoom(Math.min(root.zoomLevel + root.zoomStep, root.maxZoom))
    }
    
    function zoomOut() {
        setZoom(Math.max(root.zoomLevel - root.zoomStep, root.minZoom))
    }
    
    function setZoom(level) {
        root.zoomLevel = Math.max(root.minZoom, Math.min(level, root.maxZoom))
        root.zoomChanged(root.zoomLevel)
        
        // Adjust flickable content position to keep center
        var centerX = flickable.contentX + flickable.width / 2
        var centerY = flickable.contentY + flickable.height / 2
        flickable.contentX = centerX - flickable.width / 2
        flickable.contentY = centerY - flickable.height / 2
    }
    
    function zoomTo(level, centerX, centerY) {
        var oldZoom = root.zoomLevel
        setZoom(level)
        
        // Adjust position to zoom to specific point
        var scaleFactor = root.zoomLevel / oldZoom
        flickable.contentX = centerX * scaleFactor - flickable.width / 2
        flickable.contentY = centerY * scaleFactor - flickable.height / 2
    }
    
    function fitToWindow() {
        if (!imageItem.sourceSize.width || !imageItem.sourceSize.height) return
        
        var widthRatio = flickable.width / imageItem.sourceSize.width
        var heightRatio = flickable.height / imageItem.sourceSize.height
        setZoom(Math.min(widthRatio, heightRatio))
        
        // Center the image
        flickable.contentX = (imageContainer.width - flickable.width) / 2
        flickable.contentY = (imageContainer.height - flickable.height) / 2
    }
    
    function actualSize() {
        setZoom(1.0)
        
        // Center the image
        flickable.contentX = (imageContainer.width - flickable.width) / 2
        flickable.contentY = (imageContainer.height - flickable.height) / 2
    }
    
    function clearImage() {
        imageItem.source = ""
        root.hasImage = false
        root.selectedRoi = Qt.rect(0, 0, 0, 0)
    }
    
    function setImage(source) {
        imageItem.source = source
    }
    
    // Performance optimization
    Timer {
        id: updateTimer
        interval: 16 // ~60fps max update rate
        repeat: false
        
        property var pendingSource: ""
        
        onTriggered: {
            if (pendingSource !== "") {
                imageItem.source = pendingSource
                pendingSource = ""
            }
        }
    }
    
    function updateImage(source) {
        // Throttle updates for performance
        updateTimer.pendingSource = source
        if (!updateTimer.running) {
            updateTimer.start()
        }
    }
    
    Component.onCompleted: {
        // Set initial state
        fitToWindow()
    }
}