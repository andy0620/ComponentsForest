#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <iostream>
#include <QDir>
#include <QStringList>

int main(int argc, char *argv[])
{
    std::cout << "Qt Minimal Test Starting..." << std::endl;
    
    // Check for platform plugins before creating QApplication
    QString appDir = QCoreApplication::applicationDirPath();
    std::cout << "Application directory: " << appDir.toStdString() << std::endl;
    
    QString platformsDir = appDir + "/platforms";
    QDir pDir(platformsDir);
    if (pDir.exists()) {
        std::cout << "Platforms directory found!" << std::endl;
        QStringList files = pDir.entryList(QStringList() << "*.dll", QDir::Files);
        for (const QString& file : files) {
            std::cout << "  - " << file.toStdString() << std::endl;
        }
    } else {
        std::cout << "WARNING: No platforms directory found at: " << platformsDir.toStdString() << std::endl;
        std::cout << "This will cause QApplication to fail!" << std::endl;
    }
    
    // Print Qt library paths
    QStringList paths = QCoreApplication::libraryPaths();
    std::cout << "Library paths:" << std::endl;
    for (const QString& path : paths) {
        std::cout << "  - " << path.toStdString() << std::endl;
    }
    
    try {
        std::cout << "Creating QApplication..." << std::endl;
        QApplication app(argc, argv);
        std::cout << "QApplication created successfully!" << std::endl;
        
        std::cout << "Platform: " << app.platformName().toStdString() << std::endl;
        
        QMessageBox::information(nullptr, "Qt Test", "Qt is working!\n\nQApplication created successfully.");
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}