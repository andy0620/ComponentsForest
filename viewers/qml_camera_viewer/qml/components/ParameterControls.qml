import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import ComponentsForest 1.0

Rectangle {
    id: root
    radius: 5
    color: "#16213e"
    
    property var cameraBridge: null
    property bool advancedMode: false
    
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        
        ColumnLayout {
            width: parent.width
            spacing: 15
            
            // Header with mode toggle
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                
                Label {
                    Layout.fillWidth: true
                    text: "Camera Parameters"
                    font.pixelSize: 14
                    font.bold: true
                    color: Material.accent
                }
                
                Switch {
                    text: "Advanced"
                    checked: advancedMode
                    onToggled: advancedMode = checked
                    Material.accent: Material.Cyan
                }
            }
            
            // Basic Parameters Group
            GroupBox {
                Layout.fillWidth: true
                Layout.margins: 10
                title: "Basic Settings"
                Material.foreground: Material.Cyan
                
                background: Rectangle {
                    color: Qt.rgba(0, 0, 0, 0.3)
                    radius: 5
                    border.color: Material.color(Material.Cyan, Material.Shade800)
                    border.width: 1
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    // Exposure Control
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Label {
                                text: "Exposure"
                                font.pixelSize: 12
                                Layout.preferredWidth: 80
                            }
                            
                            Label {
                                text: exposureSlider.value.toFixed(0) + " μs"
                                font.pixelSize: 11
                                color: Material.color(Material.Grey, Material.Shade300)
                                Layout.preferredWidth: 60
                            }
                            
                            CheckBox {
                                id: autoExposureCheck
                                text: "Auto"
                                checked: cameraBridge ? cameraBridge.autoExposure : false
                                onToggled: {
                                    if (cameraBridge) {
                                        cameraBridge.setAutoExposure(checked)
                                    }
                                }
                            }
                        }
                        
                        Slider {
                            id: exposureSlider
                            Layout.fillWidth: true
                            from: 10
                            to: 1000000
                            value: cameraBridge ? cameraBridge.exposure : 10000
                            enabled: !autoExposureCheck.checked && cameraBridge && cameraBridge.isConnected
                            stepSize: 100
                            
                            onValueChanged: {
                                if (cameraBridge && !autoExposureCheck.checked && pressed) {
                                    cameraBridge.setExposure(value)
                                }
                            }
                            
                            Material.accent: Material.Cyan
                        }
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Material.color(Material.Grey, Material.Shade800)
                    }
                    
                    // Gain Control
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Label {
                                text: "Gain"
                                font.pixelSize: 12
                                Layout.preferredWidth: 80
                            }
                            
                            Label {
                                text: gainSlider.value.toFixed(1) + " dB"
                                font.pixelSize: 11
                                color: Material.color(Material.Grey, Material.Shade300)
                                Layout.preferredWidth: 60
                            }
                            
                            CheckBox {
                                id: autoGainCheck
                                text: "Auto"
                                checked: cameraBridge ? cameraBridge.autoGain : false
                                onToggled: {
                                    if (cameraBridge) {
                                        cameraBridge.setAutoGain(checked)
                                    }
                                }
                            }
                        }
                        
                        Slider {
                            id: gainSlider
                            Layout.fillWidth: true
                            from: 0
                            to: 48
                            value: cameraBridge ? cameraBridge.gain : 0
                            enabled: !autoGainCheck.checked && cameraBridge && cameraBridge.isConnected
                            stepSize: 0.1
                            
                            onValueChanged: {
                                if (cameraBridge && !autoGainCheck.checked && pressed) {
                                    cameraBridge.setGain(value)
                                }
                            }
                            
                            Material.accent: Material.Cyan
                        }
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Material.color(Material.Grey, Material.Shade800)
                    }
                    
                    // Frame Rate Control
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Label {
                                text: "Frame Rate"
                                font.pixelSize: 12
                                Layout.preferredWidth: 80
                            }
                            
                            SpinBox {
                                id: frameRateSpinBox
                                from: 1
                                to: 120
                                value: cameraBridge ? cameraBridge.targetFps : 30
                                editable: true
                                enabled: cameraBridge && cameraBridge.isConnected
                                
                                onValueModified: {
                                    if (cameraBridge) {
                                        cameraBridge.setTargetFps(value)
                                    }
                                }
                                
                                textFromValue: function(value) {
                                    return value + " fps"
                                }
                                
                                valueFromText: function(text) {
                                    return parseInt(text)
                                }
                            }
                        }
                    }
                }
            }
            
            // Image Quality Group
            GroupBox {
                Layout.fillWidth: true
                Layout.margins: 10
                title: "Image Quality"
                Material.foreground: Material.Pink
                
                background: Rectangle {
                    color: Qt.rgba(0, 0, 0, 0.3)
                    radius: 5
                    border.color: Material.color(Material.Pink, Material.Shade800)
                    border.width: 1
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    // Brightness
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Brightness"
                            font.pixelSize: 12
                            Layout.preferredWidth: 80
                        }
                        
                        Slider {
                            id: brightnessSlider
                            Layout.fillWidth: true
                            from: -100
                            to: 100
                            value: cameraBridge ? cameraBridge.brightness : 0
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onValueChanged: {
                                if (cameraBridge && pressed) {
                                    cameraBridge.setBrightness(value)
                                }
                            }
                            
                            Material.accent: Material.Pink
                        }
                        
                        Label {
                            text: brightnessSlider.value.toFixed(0)
                            font.pixelSize: 11
                            color: Material.color(Material.Grey, Material.Shade300)
                            Layout.preferredWidth: 30
                        }
                    }
                    
                    // Contrast
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Contrast"
                            font.pixelSize: 12
                            Layout.preferredWidth: 80
                        }
                        
                        Slider {
                            id: contrastSlider
                            Layout.fillWidth: true
                            from: -100
                            to: 100
                            value: cameraBridge ? cameraBridge.contrast : 0
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onValueChanged: {
                                if (cameraBridge && pressed) {
                                    cameraBridge.setContrast(value)
                                }
                            }
                            
                            Material.accent: Material.Pink
                        }
                        
                        Label {
                            text: contrastSlider.value.toFixed(0)
                            font.pixelSize: 11
                            color: Material.color(Material.Grey, Material.Shade300)
                            Layout.preferredWidth: 30
                        }
                    }
                    
                    // Saturation
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Saturation"
                            font.pixelSize: 12
                            Layout.preferredWidth: 80
                        }
                        
                        Slider {
                            id: saturationSlider
                            Layout.fillWidth: true
                            from: 0
                            to: 200
                            value: cameraBridge ? cameraBridge.saturation : 100
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onValueChanged: {
                                if (cameraBridge && pressed) {
                                    cameraBridge.setSaturation(value)
                                }
                            }
                            
                            Material.accent: Material.Pink
                        }
                        
                        Label {
                            text: saturationSlider.value.toFixed(0) + "%"
                            font.pixelSize: 11
                            color: Material.color(Material.Grey, Material.Shade300)
                            Layout.preferredWidth: 30
                        }
                    }
                    
                    // Gamma
                    RowLayout {
                        Layout.fillWidth: true
                        visible: advancedMode
                        
                        Label {
                            text: "Gamma"
                            font.pixelSize: 12
                            Layout.preferredWidth: 80
                        }
                        
                        Slider {
                            id: gammaSlider
                            Layout.fillWidth: true
                            from: 0.5
                            to: 2.5
                            value: cameraBridge ? cameraBridge.gamma : 1.0
                            enabled: cameraBridge && cameraBridge.isConnected
                            stepSize: 0.01
                            
                            onValueChanged: {
                                if (cameraBridge && pressed) {
                                    cameraBridge.setGamma(value)
                                }
                            }
                            
                            Material.accent: Material.Pink
                        }
                        
                        Label {
                            text: gammaSlider.value.toFixed(2)
                            font.pixelSize: 11
                            color: Material.color(Material.Grey, Material.Shade300)
                            Layout.preferredWidth: 30
                        }
                    }
                }
            }
            
            // Advanced Parameters Group
            GroupBox {
                Layout.fillWidth: true
                Layout.margins: 10
                title: "Advanced Settings"
                visible: advancedMode
                Material.foreground: Material.Purple
                
                background: Rectangle {
                    color: Qt.rgba(0, 0, 0, 0.3)
                    radius: 5
                    border.color: Material.color(Material.Purple, Material.Shade800)
                    border.width: 1
                }
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    // Trigger Mode
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Trigger Mode"
                            font.pixelSize: 12
                            Layout.preferredWidth: 100
                        }
                        
                        ComboBox {
                            id: triggerModeCombo
                            Layout.fillWidth: true
                            model: ["Continuous", "Software", "Hardware", "External"]
                            currentIndex: cameraBridge ? cameraBridge.triggerMode : 0
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onActivated: {
                                if (cameraBridge) {
                                    cameraBridge.setTriggerMode(currentIndex)
                                }
                            }
                        }
                    }
                    
                    // ROI Settings
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "ROI"
                            font.pixelSize: 12
                            Layout.preferredWidth: 100
                        }
                        
                        Button {
                            text: "Configure ROI"
                            Layout.fillWidth: true
                            enabled: cameraBridge && cameraBridge.isConnected
                            onClicked: roiDialog.open()
                        }
                    }
                    
                    // Binning
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Binning"
                            font.pixelSize: 12
                            Layout.preferredWidth: 100
                        }
                        
                        ComboBox {
                            id: binningCombo
                            Layout.fillWidth: true
                            model: ["1x1", "2x2", "3x3", "4x4"]
                            currentIndex: cameraBridge ? cameraBridge.binning - 1 : 0
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onActivated: {
                                if (cameraBridge) {
                                    cameraBridge.setBinning(currentIndex + 1)
                                }
                            }
                        }
                    }
                    
                    // Pixel Format
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Pixel Format"
                            font.pixelSize: 12
                            Layout.preferredWidth: 100
                        }
                        
                        ComboBox {
                            id: pixelFormatCombo
                            Layout.fillWidth: true
                            model: cameraBridge ? cameraBridge.availablePixelFormats : ["Mono8", "RGB8", "Bayer8"]
                            currentIndex: 0
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onActivated: {
                                if (cameraBridge) {
                                    cameraBridge.setPixelFormat(currentText)
                                }
                            }
                        }
                    }
                    
                    // Buffer Count
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Label {
                            text: "Buffer Count"
                            font.pixelSize: 12
                            Layout.preferredWidth: 100
                        }
                        
                        SpinBox {
                            id: bufferCountSpinBox
                            Layout.fillWidth: true
                            from: 1
                            to: 100
                            value: cameraBridge ? cameraBridge.bufferCount : 10
                            editable: true
                            enabled: cameraBridge && cameraBridge.isConnected
                            
                            onValueModified: {
                                if (cameraBridge) {
                                    cameraBridge.setBufferCount(value)
                                }
                            }
                        }
                    }
                }
            }
            
            // Control Buttons
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 10
                
                Button {
                    Layout.fillWidth: true
                    text: "Reset to Defaults"
                    Material.background: Material.color(Material.Orange, Material.Shade700)
                    enabled: cameraBridge && cameraBridge.isConnected
                    onClicked: resetDialog.open()
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Button {
                        Layout.fillWidth: true
                        text: "Save Profile"
                        Material.background: Material.color(Material.Blue, Material.Shade700)
                        enabled: cameraBridge && cameraBridge.isConnected
                        onClicked: saveProfileDialog.open()
                    }
                    
                    Button {
                        Layout.fillWidth: true
                        text: "Load Profile"
                        Material.background: Material.color(Material.Green, Material.Shade700)
                        enabled: cameraBridge && cameraBridge.isConnected
                        onClicked: loadProfileDialog.open()
                    }
                }
            }
            
            // Spacer
            Item {
                Layout.fillHeight: true
            }
        }
    }
    
    // Reset confirmation dialog
    Dialog {
        id: resetDialog
        anchors.centerIn: parent
        title: "Reset Parameters"
        standardButtons: Dialog.Yes | Dialog.No
        
        Label {
            text: "Are you sure you want to reset all parameters to default values?"
        }
        
        onAccepted: {
            if (cameraBridge) {
                cameraBridge.resetParameters()
                // Update all controls
                updateControls()
            }
        }
    }
    
    // ROI configuration dialog
    Dialog {
        id: roiDialog
        anchors.centerIn: parent
        width: 400
        height: 300
        title: "Configure Region of Interest"
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        GridLayout {
            anchors.fill: parent
            columns: 2
            rowSpacing: 10
            columnSpacing: 10
            
            Label { text: "X Offset:" }
            SpinBox {
                id: roiXSpinBox
                from: 0
                to: cameraBridge ? cameraBridge.maxWidth : 1920
                value: cameraBridge ? cameraBridge.roiX : 0
                editable: true
            }
            
            Label { text: "Y Offset:" }
            SpinBox {
                id: roiYSpinBox
                from: 0
                to: cameraBridge ? cameraBridge.maxHeight : 1080
                value: cameraBridge ? cameraBridge.roiY : 0
                editable: true
            }
            
            Label { text: "Width:" }
            SpinBox {
                id: roiWidthSpinBox
                from: 1
                to: cameraBridge ? cameraBridge.maxWidth : 1920
                value: cameraBridge ? cameraBridge.roiWidth : 1920
                editable: true
            }
            
            Label { text: "Height:" }
            SpinBox {
                id: roiHeightSpinBox
                from: 1
                to: cameraBridge ? cameraBridge.maxHeight : 1080
                value: cameraBridge ? cameraBridge.roiHeight : 1080
                editable: true
            }
            
            CheckBox {
                id: roiEnabledCheck
                Layout.columnSpan: 2
                text: "Enable ROI"
                checked: cameraBridge ? cameraBridge.roiEnabled : false
            }
        }
        
        onAccepted: {
            if (cameraBridge) {
                cameraBridge.setROI(roiXSpinBox.value, roiYSpinBox.value,
                                   roiWidthSpinBox.value, roiHeightSpinBox.value,
                                   roiEnabledCheck.checked)
            }
        }
    }
    
    // Save profile dialog
    Dialog {
        id: saveProfileDialog
        anchors.centerIn: parent
        width: 300
        height: 150
        title: "Save Parameter Profile"
        standardButtons: Dialog.Save | Dialog.Cancel
        
        TextField {
            id: profileNameField
            anchors.fill: parent
            placeholderText: "Enter profile name..."
            text: "Profile_" + Qt.formatDateTime(new Date(), "yyyyMMdd_HHmmss")
        }
        
        onAccepted: {
            if (cameraBridge && profileNameField.text) {
                cameraBridge.saveProfile(profileNameField.text)
                showNotification("Profile saved: " + profileNameField.text)
            }
        }
    }
    
    // Load profile dialog
    Dialog {
        id: loadProfileDialog
        anchors.centerIn: parent
        width: 300
        height: 400
        title: "Load Parameter Profile"
        standardButtons: Dialog.Open | Dialog.Cancel
        
        ListView {
            anchors.fill: parent
            model: cameraBridge ? cameraBridge.getProfiles() : []
            delegate: ItemDelegate {
                width: parent.width
                text: modelData
                onClicked: {
                    if (cameraBridge) {
                        cameraBridge.loadProfile(modelData)
                        loadProfileDialog.accept()
                        updateControls()
                        showNotification("Profile loaded: " + modelData)
                    }
                }
            }
        }
    }
    
    // Function to update all controls with current values
    function updateControls() {
        if (!cameraBridge) return
        
        exposureSlider.value = cameraBridge.exposure
        gainSlider.value = cameraBridge.gain
        frameRateSpinBox.value = cameraBridge.targetFps
        brightnessSlider.value = cameraBridge.brightness
        contrastSlider.value = cameraBridge.contrast
        saturationSlider.value = cameraBridge.saturation
        gammaSlider.value = cameraBridge.gamma
        autoExposureCheck.checked = cameraBridge.autoExposure
        autoGainCheck.checked = cameraBridge.autoGain
    }
    
    // Function to show notification
    function showNotification(message) {
        // This would typically emit a signal to the main window
        console.log("Notification: " + message)
    }
    
    // Update controls when camera bridge changes
    onCameraBridgeChanged: {
        if (cameraBridge) {
            updateControls()
        }
    }
    
    // Connections to camera bridge
    Connections {
        target: cameraBridge
        
        function onParametersChanged() {
            updateControls()
        }
        
        function onConnectionStateChanged() {
            updateControls()
        }
    }
}