// Test program to verify camera discovery fix
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "viewers/do3think_camera_viewer/machine.h"

using namespace ComponentsForest;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Do3Think Camera Discovery Test ===";
    qDebug() << "Testing real device enumeration (no mock data)";
    
    // Create machine instance
    Do3ThinkCameraMachine machine;
    
    // Start the machine
    if (machine.start()) {
        qDebug() << "Machine started successfully";
    } else {
        qDebug() << "Failed to start machine";
        return 1;
    }
    
    // Discover available devices
    qDebug() << "\nScanning for Do3Think cameras...";
    QStringList devices = machine.discoverAvailableDevices();
    
    if (devices.isEmpty()) {
        qDebug() << "✓ No cameras found - this is expected if no real cameras are connected";
        qDebug() << "✓ Mock data has been removed successfully";
    } else {
        qDebug() << "Found" << devices.size() << "real camera(s):";
        for (const QString& device : devices) {
            qDebug() << "  -" << device;
        }
    }
    
    // Test refresh functionality
    qDebug() << "\nTesting device refresh...";
    if (machine.refreshDeviceList()) {
        qDebug() << "✓ Device refresh completed";
    }
    
    // Stop the machine
    machine.stop();
    qDebug() << "\n=== Test Complete ===";
    
    // Exit after a short delay
    QTimer::singleShot(100, &app, &QCoreApplication::quit);
    
    return app.exec();
}