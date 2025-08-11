import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window
import ComponentsForest.Bridge 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    title: qsTr("ComponentsForest Camera Viewer")
    
    // Industrial dark theme colors
    readonly property color primaryColor: "#1a1f2e"
    readonly property color secondaryColor: "#2a3447"
    readonly property color accentColor: "#4a9eff"
    readonly property color warningColor: "#ff9800"
    readonly property color errorColor: "#f44336"
    readonly property color successColor: "#4caf50"
    readonly property color textPrimary: "#e0e0e0"
    readonly property color textSecondary: "#9e9e9e"
    readonly property color borderColor: "#3a4557"
    
    // Layout mode enumeration
    enum LayoutMode {
        SingleView,
        GridView,
        TabView,
        SplitView
    }
    
    property int currentLayoutMode: mainWindow.LayoutMode.GridView
    property int gridColumns: 2
    property int gridRows: 2
    
    Material.theme: Material.Dark
    Material.primary: accentColor
    Material.accent: accentColor
    Material.background: primaryColor
    Material.foreground: textPrimary
    
    // Bridge connection
    MachineBridge {
        id: machineBridge
        onMachineStateChanged: {
            statusBar.machineState = state
        }
        onCameraCountChanged: {
            statusBar.cameraCount = count
        }
    }
    
    // Menu Bar
    menuBar: MenuBar {
        background: Rectangle {
            color: primaryColor
            border.color: borderColor
            border.width: 1
        }
        
        Menu {
            title: qsTr("File")
            
            Action {
                text: qsTr("New Configuration")
                shortcut: "Ctrl+N"
                onTriggered: machineBridge.newConfiguration()
            }
            
            Action {
                text: qsTr("Open Configuration...")
                shortcut: "Ctrl+O"
                onTriggered: machineBridge.openConfiguration()
            }
            
            Action {
                text: qsTr("Save Configuration")
                shortcut: "Ctrl+S"
                onTriggered: machineBridge.saveConfiguration()
            }
            
            MenuSeparator {
                contentItem: Rectangle {
                    implicitHeight: 1
                    color: borderColor
                }
            }
            
            Action {
                text: qsTr("Exit")
                shortcut: "Ctrl+Q"
                onTriggered: Qt.quit()
            }
        }
        
        Menu {
            title: qsTr("View")
            
            Action {
                text: qsTr("Single View")
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.SingleView
                onTriggered: currentLayoutMode = mainWindow.LayoutMode.SingleView
            }
            
            Action {
                text: qsTr("Grid View")
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.GridView
                onTriggered: currentLayoutMode = mainWindow.LayoutMode.GridView
            }
            
            Action {
                text: qsTr("Tab View")
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.TabView
                onTriggered: currentLayoutMode = mainWindow.LayoutMode.TabView
            }
            
            Action {
                text: qsTr("Split View")
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.SplitView
                onTriggered: currentLayoutMode = mainWindow.LayoutMode.SplitView
            }
            
            MenuSeparator {
                contentItem: Rectangle {
                    implicitHeight: 1
                    color: borderColor
                }
            }
            
            Menu {
                title: qsTr("Grid Layout")
                enabled: currentLayoutMode === mainWindow.LayoutMode.GridView
                
                Action {
                    text: qsTr("2x2")
                    checkable: true
                    checked: gridColumns === 2 && gridRows === 2
                    onTriggered: {
                        gridColumns = 2
                        gridRows = 2
                    }
                }
                
                Action {
                    text: qsTr("3x3")
                    checkable: true
                    checked: gridColumns === 3 && gridRows === 3
                    onTriggered: {
                        gridColumns = 3
                        gridRows = 3
                    }
                }
                
                Action {
                    text: qsTr("4x4")
                    checkable: true
                    checked: gridColumns === 4 && gridRows === 4
                    onTriggered: {
                        gridColumns = 4
                        gridRows = 4
                    }
                }
            }
            
            MenuSeparator {
                contentItem: Rectangle {
                    implicitHeight: 1
                    color: borderColor
                }
            }
            
            Action {
                text: qsTr("Full Screen")
                shortcut: "F11"
                checkable: true
                checked: mainWindow.visibility === Window.FullScreen
                onTriggered: {
                    if (mainWindow.visibility === Window.FullScreen) {
                        mainWindow.visibility = Window.Windowed
                    } else {
                        mainWindow.visibility = Window.FullScreen
                    }
                }
            }
        }
        
        Menu {
            title: qsTr("Cameras")
            
            Action {
                text: qsTr("Add Camera...")
                shortcut: "Ctrl+Shift+A"
                onTriggered: machineBridge.addCamera()
            }
            
            Action {
                text: qsTr("Remove Camera")
                shortcut: "Ctrl+Shift+R"
                enabled: machineBridge.hasSelectedCamera
                onTriggered: machineBridge.removeSelectedCamera()
            }
            
            MenuSeparator {
                contentItem: Rectangle {
                    implicitHeight: 1
                    color: borderColor
                }
            }
            
            Action {
                text: qsTr("Start All")
                shortcut: "F5"
                onTriggered: machineBridge.startAllCameras()
            }
            
            Action {
                text: qsTr("Stop All")
                shortcut: "Shift+F5"
                onTriggered: machineBridge.stopAllCameras()
            }
            
            MenuSeparator {
                contentItem: Rectangle {
                    implicitHeight: 1
                    color: borderColor
                }
            }
            
            Action {
                text: qsTr("Camera Settings...")
                shortcut: "Ctrl+Shift+S"
                enabled: machineBridge.hasSelectedCamera
                onTriggered: machineBridge.showCameraSettings()
            }
        }
        
        Menu {
            title: qsTr("Tools")
            
            Action {
                text: qsTr("Performance Monitor")
                shortcut: "Ctrl+P"
                onTriggered: machineBridge.showPerformanceMonitor()
            }
            
            Action {
                text: qsTr("Log Viewer")
                shortcut: "Ctrl+L"
                onTriggered: machineBridge.showLogViewer()
            }
            
            MenuSeparator {
                contentItem: Rectangle {
                    implicitHeight: 1
                    color: borderColor
                }
            }
            
            Action {
                text: qsTr("Options...")
                shortcut: "Ctrl+Alt+O"
                onTriggered: machineBridge.showOptions()
            }
        }
        
        Menu {
            title: qsTr("Help")
            
            Action {
                text: qsTr("Documentation")
                shortcut: "F1"
                onTriggered: machineBridge.showDocumentation()
            }
            
            Action {
                text: qsTr("About...")
                onTriggered: machineBridge.showAbout()
            }
        }
    }
    
    // Tool Bar
    header: ToolBar {
        height: 48
        
        background: Rectangle {
            color: secondaryColor
            border.color: borderColor
            border.width: 1
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8
            
            ToolButton {
                icon.source: "qrc:/icons/add_camera.svg"
                icon.width: 24
                icon.height: 24
                ToolTip.text: qsTr("Add Camera")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: machineBridge.addCamera()
            }
            
            ToolButton {
                icon.source: "qrc:/icons/start_all.svg"
                icon.width: 24
                icon.height: 24
                ToolTip.text: qsTr("Start All Cameras")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: machineBridge.startAllCameras()
            }
            
            ToolButton {
                icon.source: "qrc:/icons/stop_all.svg"
                icon.width: 24
                icon.height: 24
                ToolTip.text: qsTr("Stop All Cameras")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: machineBridge.stopAllCameras()
            }
            
            ToolSeparator {
                contentItem: Rectangle {
                    implicitWidth: 1
                    color: borderColor
                }
            }
            
            ToolButton {
                icon.source: "qrc:/icons/single_view.svg"
                icon.width: 24
                icon.height: 24
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.SingleView
                ToolTip.text: qsTr("Single View")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: currentLayoutMode = mainWindow.LayoutMode.SingleView
            }
            
            ToolButton {
                icon.source: "qrc:/icons/grid_view.svg"
                icon.width: 24
                icon.height: 24
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.GridView
                ToolTip.text: qsTr("Grid View")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: currentLayoutMode = mainWindow.LayoutMode.GridView
            }
            
            ToolButton {
                icon.source: "qrc:/icons/tab_view.svg"
                icon.width: 24
                icon.height: 24
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.TabView
                ToolTip.text: qsTr("Tab View")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: currentLayoutMode = mainWindow.LayoutMode.TabView
            }
            
            ToolButton {
                icon.source: "qrc:/icons/split_view.svg"
                icon.width: 24
                icon.height: 24
                checkable: true
                checked: currentLayoutMode === mainWindow.LayoutMode.SplitView
                ToolTip.text: qsTr("Split View")
                ToolTip.visible: hovered
                ToolTip.delay: 500
                onClicked: currentLayoutMode = mainWindow.LayoutMode.SplitView
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Label {
                text: qsTr("Layout:")
                color: textSecondary
            }
            
            ComboBox {
                id: gridLayoutCombo
                model: ["2x2", "3x3", "4x4"]
                currentIndex: 0
                enabled: currentLayoutMode === mainWindow.LayoutMode.GridView
                implicitWidth: 80
                
                onCurrentIndexChanged: {
                    switch(currentIndex) {
                        case 0:
                            gridColumns = 2
                            gridRows = 2
                            break
                        case 1:
                            gridColumns = 3
                            gridRows = 3
                            break
                        case 2:
                            gridColumns = 4
                            gridRows = 4
                            break
                    }
                }
            }
        }
    }
    
    // Main Content Area
    contentItem: Rectangle {
        color: primaryColor
        
        StackLayout {
            id: layoutStack
            anchors.fill: parent
            currentIndex: currentLayoutMode
            
            // Single View Layout
            Item {
                CameraViewContainer {
                    id: singleViewContainer
                    anchors.fill: parent
                    anchors.margins: 8
                    viewMode: "single"
                }
            }
            
            // Grid View Layout
            Item {
                GridLayout {
                    id: gridViewContainer
                    anchors.fill: parent
                    anchors.margins: 8
                    columns: gridColumns
                    rows: gridRows
                    columnSpacing: 8
                    rowSpacing: 8
                    
                    Repeater {
                        model: gridColumns * gridRows
                        
                        CameraViewContainer {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            viewMode: "grid"
                            cameraIndex: index
                        }
                    }
                }
            }
            
            // Tab View Layout
            TabBar {
                id: tabViewContainer
                
                Repeater {
                    model: machineBridge.cameraModel
                    
                    TabButton {
                        text: model.name || qsTr("Camera %1").arg(index + 1)
                        width: implicitWidth
                    }
                }
            }
            
            // Split View Layout
            SplitView {
                id: splitViewContainer
                orientation: Qt.Horizontal
                
                CameraViewContainer {
                    SplitView.minimumWidth: 200
                    SplitView.preferredWidth: parent.width / 2
                    viewMode: "split"
                    cameraIndex: 0
                }
                
                CameraViewContainer {
                    SplitView.minimumWidth: 200
                    SplitView.fillWidth: true
                    viewMode: "split"
                    cameraIndex: 1
                }
            }
        }
    }
    
    // Status Bar
    footer: ToolBar {
        id: statusBar
        height: 32
        
        property string machineState: "Idle"
        property int cameraCount: 0
        property real fps: 0.0
        property int frameCount: 0
        property string memoryUsage: "0 MB"
        property string cpuUsage: "0%"
        
        background: Rectangle {
            color: secondaryColor
            border.color: borderColor
            border.width: 1
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 20
            
            // Machine State Indicator
            Row {
                spacing: 8
                
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    anchors.verticalCenter: parent.verticalCenter
                    
                    color: {
                        switch(statusBar.machineState) {
                            case "Running": return successColor
                            case "Starting":
                            case "Stopping": return warningColor
                            case "Error": return errorColor
                            default: return textSecondary
                        }
                    }
                    
                    Behavior on color {
                        ColorAnimation { duration: 200 }
                    }
                }
                
                Label {
                    text: qsTr("Machine: %1").arg(statusBar.machineState)
                    color: textPrimary
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            Rectangle {
                width: 1
                height: 20
                color: borderColor
            }
            
            // Camera Count
            Label {
                text: qsTr("Cameras: %1").arg(statusBar.cameraCount)
                color: textPrimary
                font.pixelSize: 12
            }
            
            Rectangle {
                width: 1
                height: 20
                color: borderColor
            }
            
            // FPS Display
            Label {
                text: qsTr("FPS: %1").arg(statusBar.fps.toFixed(1))
                color: textPrimary
                font.pixelSize: 12
            }
            
            Rectangle {
                width: 1
                height: 20
                color: borderColor
            }
            
            // Frame Count
            Label {
                text: qsTr("Frames: %1").arg(statusBar.frameCount)
                color: textPrimary
                font.pixelSize: 12
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            // Performance Metrics
            Row {
                spacing: 16
                
                Label {
                    text: qsTr("CPU: %1").arg(statusBar.cpuUsage)
                    color: textSecondary
                    font.pixelSize: 12
                }
                
                Label {
                    text: qsTr("Memory: %1").arg(statusBar.memoryUsage)
                    color: textSecondary
                    font.pixelSize: 12
                }
            }
            
            Rectangle {
                width: 1
                height: 20
                color: borderColor
            }
            
            // Timestamp
            Label {
                id: timestampLabel
                color: textSecondary
                font.pixelSize: 12
                
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: {
                        timestampLabel.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss")
                    }
                }
            }
        }
    }
    
    // Performance update timer
    Timer {
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            statusBar.fps = machineBridge.currentFPS
            statusBar.frameCount = machineBridge.totalFrameCount
            statusBar.memoryUsage = machineBridge.memoryUsage
            statusBar.cpuUsage = machineBridge.cpuUsage
        }
    }
    
    // Dialogs
    Component {
        id: aboutDialog
        
        Dialog {
            title: qsTr("About ComponentsForest Camera Viewer")
            standardButtons: Dialog.Ok
            modal: true
            anchors.centerIn: parent
            
            contentItem: Column {
                spacing: 12
                padding: 20
                
                Label {
                    text: qsTr("ComponentsForest Camera Viewer")
                    font.pixelSize: 18
                    font.bold: true
                }
                
                Label {
                    text: qsTr("Version 1.0.0")
                    color: textSecondary
                }
                
                Label {
                    text: qsTr("Industrial AOI Equipment Component Ecosystem")
                    wrapMode: Text.WordWrap
                }
                
                Label {
                    text: qsTr("Built with Qt 6.9 and QML")
                    color: textSecondary
                    font.pixelSize: 12
                }
            }
        }
    }
}