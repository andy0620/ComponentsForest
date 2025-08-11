/**
 * @file dothink_camera_control_panel_methods.cpp
 * @brief Primary control interface methods for Do3ThinkCameraControlPanel
 * 
 * Implements the camera control methods that make the control panel
 * the primary interface for camera operations.
 */

#include "dothink_camera_control_panel.h"
#include <QDebug>
#include <QComboBox>

namespace ComponentsForest {

// ============================================================================
// Primary Camera Control Interface Implementation
// ============================================================================

bool Do3ThinkCameraControlPanel::startAcquisition()
{
    // Emit signal to request camera to start acquisition
    // This maintains complete decoupling via Signal/Slot
    emit requestStartAcquisition();
    
    // Also emit Do3Think specific signal if needed
    emit acquisitionStartRequested();
    
    // Note: We don't know if it succeeded immediately since it's async
    // The actual state will be updated via onAcquisitionStateChanged slot
    return true;  // Return true to indicate request was sent
}

bool Do3ThinkCameraControlPanel::stopAcquisition()
{
    // Emit signal to request camera to stop acquisition
    emit requestStopAcquisition();
    
    // Also emit Do3Think specific signal if needed
    emit acquisitionStopRequested();
    
    return true;  // Return true to indicate request was sent
}

bool Do3ThinkCameraControlPanel::isAcquiring() const
{
    // This should query the internal state that's updated by
    // onAcquisitionStateChanged slot from the base class
    return CameraControlPanel::isAcquiring();
}

bool Do3ThinkCameraControlPanel::connectCamera()
{
    // Get selected device from UI (assuming we have a device combo box)
    QString deviceId;
    
    // If we have a device selection combo in the base controls
    if (m_controls.deviceCombo) {
        deviceId = m_controls.deviceCombo->currentText();
    }
    
    if (deviceId.isEmpty()) {
        qWarning() << "No device selected for connection";
        return false;
    }
    
    // Emit signal to request connection
    emit requestConnect(deviceId);
    
    // Also emit Do3Think specific signal if needed
    emit connectionRequested();
    
    return true;  // Return true to indicate request was sent
}

bool Do3ThinkCameraControlPanel::disconnectCamera()
{
    // Emit signal to request disconnection
    emit requestDisconnect();
    
    // Also emit Do3Think specific signal if needed
    emit disconnectionRequested();
    
    return true;  // Return true to indicate request was sent
}

bool Do3ThinkCameraControlPanel::isConnected() const
{
    // This should query the internal state that's updated by
    // onDeviceConnected/onDeviceDisconnected slots from the base class
    return CameraControlPanel::isConnected();
}

void Do3ThinkCameraControlPanel::closeEvent(QCloseEvent* event)
{
    // Before closing, ensure camera is stopped
    if (isAcquiring()) {
        stopAcquisition();
    }
    
    // Emit signal that panel is closing
    emit panelClosing();
    
    // Call base class implementation
    QWidget::closeEvent(event);
}

// ============================================================================
// Image Processing Public API Implementation
// ============================================================================

void Do3ThinkCameraControlPanel::enableImageProcessing(bool enable)
{
    if (m_do3thinkControls.processingEnableCheck) {
        m_do3thinkControls.processingEnableCheck->setChecked(enable);
        // This will trigger onProcessingEnableToggled which handles the rest
    }
}

bool Do3ThinkCameraControlPanel::isImageProcessingEnabled() const
{
    return m_do3thinkState.processingEnabled;
}

void Do3ThinkCameraControlPanel::enableBlurPreprocessor(bool enable)
{
    if (m_do3thinkControls.blurEnableCheck) {
        m_do3thinkControls.blurEnableCheck->setChecked(enable);
        // This will trigger onBlurEnableToggled which handles the rest
    }
}

void Do3ThinkCameraControlPanel::enableEdgePreprocessor(bool enable)
{
    if (m_do3thinkControls.edgeEnableCheck) {
        m_do3thinkControls.edgeEnableCheck->setChecked(enable);
        // This will trigger onEdgeEnableToggled which handles the rest
    }
}

void Do3ThinkCameraControlPanel::enableDenoisePreprocessor(bool enable)
{
    if (m_do3thinkControls.denoiseEnableCheck) {
        m_do3thinkControls.denoiseEnableCheck->setChecked(enable);
        // This will trigger onDenoiseEnableToggled which handles the rest
    }
}

void Do3ThinkCameraControlPanel::setBlurKernelSize(int size)
{
    if (m_do3thinkControls.blurKernelSpinBox) {
        m_do3thinkControls.blurKernelSpinBox->setValue(size);
        // This will trigger onBlurKernelChanged which handles the rest
    }
}

void Do3ThinkCameraControlPanel::setEdgeThreshold(double threshold)
{
    if (m_do3thinkControls.edgeThresholdSpinBox) {
        m_do3thinkControls.edgeThresholdSpinBox->setValue(threshold);
        // This will trigger onEdgeThresholdChanged which handles the rest
    }
}

void Do3ThinkCameraControlPanel::setDenoiseStrength(double strength)
{
    if (m_do3thinkControls.denoiseStrengthSpinBox) {
        m_do3thinkControls.denoiseStrengthSpinBox->setValue(strength);
        // This will trigger onDenoiseStrengthChanged which handles the rest
    }
}

void Do3ThinkCameraControlPanel::setProcessingOrder(const QStringList& order)
{
    m_do3thinkState.processingOrder = order;
    
    if (!m_do3thinkControls.processingOrderList) return;
    
    // Clear and rebuild the list with the new order
    m_do3thinkControls.processingOrderList->clear();
    
    for (const QString& processor : order) {
        QListWidgetItem* item = new QListWidgetItem(processor);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_do3thinkControls.processingOrderList->addItem(item);
    }
    
    emit processingOrderChanged(order);
}

void Do3ThinkCameraControlPanel::showProcessingConfiguration()
{
    // This could open a detailed configuration dialog
    // For now, just emit the signal
    emit processingConfigurationRequested();
    
    qDebug() << "Processing configuration dialog requested";
    
    // Future implementation could show a dialog with advanced settings:
    // - Algorithm selection for edge detection (Canny, Sobel, etc.)
    // - Denoise algorithm selection (Bilateral, NLMeans, etc.) 
    // - Advanced blur parameters (sigma values, border types)
    // - Performance settings (GPU usage, thread count)
    // - Pipeline configuration (execution mode, buffer sizes)
}

// Slots for receiving updates from the camera component
void Do3ThinkCameraControlPanel::onProcessingEnabledChanged(bool enabled)
{
    m_do3thinkState.processingEnabled = enabled;
    if (m_do3thinkControls.processingEnableCheck) {
        m_do3thinkControls.processingEnableCheck->setChecked(enabled);
    }
    updateProcessingDisplay();
}

void Do3ThinkCameraControlPanel::onPreprocessorEnabledChanged(const QString& type, bool enabled)
{
    if (type == "blur") {
        m_do3thinkState.blurEnabled = enabled;
        if (m_do3thinkControls.blurEnableCheck) {
            m_do3thinkControls.blurEnableCheck->setChecked(enabled);
        }
    } else if (type == "edge") {
        m_do3thinkState.edgeEnabled = enabled;
        if (m_do3thinkControls.edgeEnableCheck) {
            m_do3thinkControls.edgeEnableCheck->setChecked(enabled);
        }
    } else if (type == "denoise") {
        m_do3thinkState.denoiseEnabled = enabled;
        if (m_do3thinkControls.denoiseEnableCheck) {
            m_do3thinkControls.denoiseEnableCheck->setChecked(enabled);
        }
    }
    
    updateProcessingOrderList();
    updateProcessingDisplay();
}

void Do3ThinkCameraControlPanel::onProcessingOrderChanged(const QStringList& order)
{
    m_do3thinkState.processingOrder = order;
    
    // Update the UI if order was changed externally
    if (m_do3thinkControls.processingOrderList) {
        m_do3thinkControls.processingOrderList->clear();
        for (const QString& processor : order) {
            QListWidgetItem* item = new QListWidgetItem(processor);
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            m_do3thinkControls.processingOrderList->addItem(item);
        }
    }
}

} // namespace ComponentsForest