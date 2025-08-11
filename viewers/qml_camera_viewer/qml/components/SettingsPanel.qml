import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import ComponentsForest 1.0

ScrollView {
    id: root
    contentWidth: availableWidth
    
    property var machineBridge: null
    
    ColumnLayout {
        width: parent.width
        spacing: 20
        
        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.margins: 10
            height: 50
            radius: 5
            color: Material.color(Material.Blue, Material.Shade900)
            
            Label {
                anchors.centerIn: parent
                text: "Application Settings"
                font.pixelSize: 18
                font.bold: true
            }
        }
        
        // Display Settings
        GroupBox {
            Layout.fillWidth: true
            Layout.margins: 10
            title: "Display Settings"
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                // Theme selection
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Theme:"
                        Layout.preferredWidth: 120
                    }
                    
                    ComboBox {
                        id: themeCombo
                        Layout.fillWidth: true
                        model: ["Dark Industrial", "Light Industrial", "High Contrast", "Blue Theme"]
                        currentIndex: 0
                        
                        onActivated: {
                            applyTheme(currentIndex)
                        }
                    }
                }
                
                // Font size
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Font Size:"
                        Layout.preferredWidth: 120
                    }
                    
                    SpinBox {
                        id: fontSizeSpinBox
                        from: 8
                        to: 20
                        value: 12
                        
                        onValueModified: {
                            // Apply font size globally
                        }
                    }
                }
                
                // Show tooltips
                CheckBox {
                    text: "Show tooltips"
                    checked: true
                }
                
                // Enable animations
                CheckBox {
                    text: "Enable animations"
                    checked: true
                }
                
                // Grid settings
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Default Grid Size:"
                        Layout.preferredWidth: 120
                    }
                    
                    SpinBox {
                        from: 10
                        to: 200
                        value: 50
                        stepSize: 10
                    }
                }
            }
        }
        
        // Performance Settings
        GroupBox {
            Layout.fillWidth: true
            Layout.margins: 10
            title: "Performance"
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                // UI refresh rate
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "UI Refresh Rate:"
                        Layout.preferredWidth: 120
                    }
                    
                    SpinBox {
                        from: 10
                        to: 60
                        value: 30
                        stepSize: 5
                        
                        textFromValue: function(value) {
                            return value + " fps"
                        }
                    }
                }
                
                // Image quality
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Display Quality:"
                        Layout.preferredWidth: 120
                    }
                    
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Low", "Medium", "High", "Maximum"]
                        currentIndex: 2
                    }
                }
                
                // Hardware acceleration
                CheckBox {
                    text: "Enable hardware acceleration"
                    checked: true
                }
                
                // Multi-threading
                CheckBox {
                    text: "Enable multi-threading"
                    checked: true
                }
                
                // Memory limit
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Memory Limit:"
                        Layout.preferredWidth: 120
                    }
                    
                    SpinBox {
                        from: 256
                        to: 8192
                        value: 2048
                        stepSize: 256
                        
                        textFromValue: function(value) {
                            return value + " MB"
                        }
                    }
                }
            }
        }
        
        // Acquisition Settings
        GroupBox {
            Layout.fillWidth: true
            Layout.margins: 10
            title: "Default Acquisition Settings"
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                // Default buffer size
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Buffer Size:"
                        Layout.preferredWidth: 120
                    }
                    
                    SpinBox {
                        from: 1
                        to: 100
                        value: 10
                    }
                }
                
                // Auto-connect
                CheckBox {
                    text: "Auto-connect to first available device"
                    checked: false
                }
                
                // Auto-start
                CheckBox {
                    text: "Auto-start acquisition on connect"
                    checked: false
                }
                
                // Save format
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Save Format:"
                        Layout.preferredWidth: 120
                    }
                    
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["PNG", "JPEG", "BMP", "TIFF"]
                        currentIndex: 0
                    }
                }
                
                // Compression quality
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "JPEG Quality:"
                        Layout.preferredWidth: 120
                    }
                    
                    Slider {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: 85
                        stepSize: 5
                    }
                    
                    Label {
                        text: "85%"
                        Layout.preferredWidth: 40
                    }
                }
            }
        }
        
        // Advanced Settings
        GroupBox {
            Layout.fillWidth: true
            Layout.margins: 10
            title: "Advanced"
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                // Debug mode
                CheckBox {
                    text: "Enable debug mode"
                    checked: false
                    
                    onToggled: {
                        if (machineBridge) {
                            machineBridge.setDebugMode(checked)
                        }
                    }
                }
                
                // Logging level
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Logging Level:"
                        Layout.preferredWidth: 120
                    }
                    
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Error", "Warning", "Info", "Debug", "Verbose"]
                        currentIndex: 2
                    }
                }
                
                // Log to file
                CheckBox {
                    text: "Save logs to file"
                    checked: false
                }
                
                // Developer options
                CheckBox {
                    text: "Show developer options"
                    checked: false
                }
            }
        }
        
        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 10
            
            Button {
                Layout.fillWidth: true
                text: "Reset to Defaults"
                Material.background: Material.color(Material.Orange, Material.Shade700)
                onClicked: resetDialog.open()
            }
            
            Button {
                Layout.fillWidth: true
                text: "Export Settings"
                Material.background: Material.color(Material.Blue, Material.Shade700)
                onClicked: exportSettings()
            }
            
            Button {
                Layout.fillWidth: true
                text: "Import Settings"
                Material.background: Material.color(Material.Green, Material.Shade700)
                onClicked: importSettings()
            }
        }
        
        // About section
        GroupBox {
            Layout.fillWidth: true
            Layout.margins: 10
            title: "About"
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 5
                
                Label {
                    text: "ComponentsForest Camera Viewer"
                    font.bold: true
                }
                
                Label {
                    text: "Version: 1.0.0"
                    font.pixelSize: 11
                }
                
                Label {
                    text: "Qt Version: " + qtVersion
                    font.pixelSize: 11
                }
                
                Label {
                    text: "Build Date: " + Qt.formatDateTime(new Date(), "yyyy-MM-dd")
                    font.pixelSize: 11
                }
                
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Material.color(Material.Grey, Material.Shade800)
                }
                
                Label {
                    text: "Industrial AOI Equipment Component Ecosystem"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                
                Button {
                    text: "View License"
                    flat: true
                    onClicked: licenseDialog.open()
                }
            }
        }
        
        // Spacer
        Item {
            Layout.fillHeight: true
        }
    }
    
    // Reset dialog
    Dialog {
        id: resetDialog
        anchors.centerIn: parent
        title: "Reset Settings"
        standardButtons: Dialog.Yes | Dialog.No
        
        Label {
            text: "Are you sure you want to reset all settings to default values?"
        }
        
        onAccepted: resetSettings()
    }
    
    // License dialog
    Dialog {
        id: licenseDialog
        anchors.centerIn: parent
        width: 500
        height: 400
        title: "License Information"
        standardButtons: Dialog.Ok
        
        ScrollView {
            anchors.fill: parent
            
            TextArea {
                text: "ComponentsForest Camera Viewer\n\n" +
                      "Copyright (c) 2024\n\n" +
                      "This software is part of the ComponentsForest industrial automation ecosystem.\n\n" +
                      "Licensed under the MIT License."
                readOnly: true
                wrapMode: Text.WordWrap
            }
        }
    }
    
    // Functions
    function applyTheme(themeIndex) {
        switch(themeIndex) {
        case 0: // Dark Industrial
            Material.theme = Material.Dark
            Material.primary = "#00ACC1"
            Material.accent = "#FF6E40"
            break
        case 1: // Light Industrial
            Material.theme = Material.Light
            Material.primary = "#0288D1"
            Material.accent = "#FF5722"
            break
        case 2: // High Contrast
            Material.theme = Material.Dark
            Material.primary = "#FFFFFF"
            Material.accent = "#FFFF00"
            break
        case 3: // Blue Theme
            Material.theme = Material.Dark
            Material.primary = "#2196F3"
            Material.accent = "#00BCD4"
            break
        }
    }
    
    function resetSettings() {
        themeCombo.currentIndex = 0
        fontSizeSpinBox.value = 12
        applyTheme(0)
        // Reset other settings...
    }
    
    function exportSettings() {
        // Export settings to file
        console.log("Exporting settings...")
    }
    
    function importSettings() {
        // Import settings from file
        console.log("Importing settings...")
    }
}