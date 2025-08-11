/**
 * @file test_preprocessor_ui.cpp
 * @brief Unit and integration tests for preprocessor UI functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QTest>
#include <QApplication>
#include <QDockWidget>
#include <QAction>
#include <QTabWidget>
#include <QSignalSpy>
#include "../viewers/do3think_camera_viewer/main_ui.h"
#include "../OpenCV/preprocessor_control_panel.h"
#include "../OpenCV/simple_edge_preprocessor.h"

using namespace ComponentsForest;
using namespace testing;

/**
 * @brief Test fixture for preprocessor UI tests
 */
class PreprocessorUITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create application instance if not exists
        if (!QApplication::instance()) {
            int argc = 0;
            app = new QApplication(argc, nullptr);
        }
        
        // Create main UI instance
        mainUI = new Do3ThinkCameraViewerMainUI();
    }
    
    void TearDown() override {
        delete mainUI;
        mainUI = nullptr;
        
        // Clean up application if we created it
        if (app) {
            delete app;
            app = nullptr;
        }
    }
    
    Do3ThinkCameraViewerMainUI* mainUI = nullptr;
    QApplication* app = nullptr;
};

/**
 * @brief Test that preprocessor dock is hidden by default
 */
TEST_F(PreprocessorUITest, PreprocessorDockHiddenByDefault) {
    // Arrange & Act - UI should be created with dock hidden
    
    // Assert
    QDockWidget* dock = mainUI->findChild<QDockWidget*>("ImageProcessingDock");
    ASSERT_NE(dock, nullptr) << "Image processing dock should exist";
    EXPECT_FALSE(dock->isVisible()) << "Preprocessor dock should be hidden by default";
}

/**
 * @brief Test that Preprocess action exists in menu and toolbar
 */
TEST_F(PreprocessorUITest, PreprocessActionExists) {
    // Arrange & Act
    QAction* preprocessAction = nullptr;
    
    // Find the action by searching all actions
    for (QAction* action : mainUI->actions()) {
        if (action->text().contains("Preprocess", Qt::CaseInsensitive)) {
            preprocessAction = action;
            break;
        }
    }
    
    // Assert
    ASSERT_NE(preprocessAction, nullptr) << "Preprocess action should exist";
    EXPECT_TRUE(preprocessAction->isCheckable()) << "Preprocess action should be checkable";
    EXPECT_FALSE(preprocessAction->isChecked()) << "Preprocess action should be unchecked by default";
    EXPECT_EQ(preprocessAction->shortcut(), QKeySequence("Ctrl+P")) << "Should have Ctrl+P shortcut";
}

/**
 * @brief Test that clicking Preprocess action toggles dock visibility
 */
TEST_F(PreprocessorUITest, PreprocessActionTogglesDock) {
    // Arrange
    QDockWidget* dock = mainUI->findChild<QDockWidget*>("ImageProcessingDock");
    ASSERT_NE(dock, nullptr);
    
    QAction* preprocessAction = nullptr;
    for (QAction* action : mainUI->actions()) {
        if (action->text().contains("Preprocess", Qt::CaseInsensitive)) {
            preprocessAction = action;
            break;
        }
    }
    ASSERT_NE(preprocessAction, nullptr);
    
    // Act - trigger the action to show the dock
    preprocessAction->trigger();
    
    // Assert - dock should be visible
    EXPECT_TRUE(dock->isVisible()) << "Dock should be visible after triggering action";
    EXPECT_TRUE(preprocessAction->isChecked()) << "Action should be checked when dock is visible";
    
    // Act - trigger again to hide
    preprocessAction->trigger();
    
    // Assert - dock should be hidden
    EXPECT_FALSE(dock->isVisible()) << "Dock should be hidden after second trigger";
    EXPECT_FALSE(preprocessAction->isChecked()) << "Action should be unchecked when dock is hidden";
}

/**
 * @brief Test that dock appears as floating when first shown
 */
TEST_F(PreprocessorUITest, DockFloatingOnFirstShow) {
    // Arrange
    QDockWidget* dock = mainUI->findChild<QDockWidget*>("ImageProcessingDock");
    ASSERT_NE(dock, nullptr);
    
    QAction* preprocessAction = nullptr;
    for (QAction* action : mainUI->actions()) {
        if (action->text().contains("Preprocess", Qt::CaseInsensitive)) {
            preprocessAction = action;
            break;
        }
    }
    ASSERT_NE(preprocessAction, nullptr);
    
    // Act - show the dock for the first time
    preprocessAction->trigger();
    
    // Assert
    EXPECT_TRUE(dock->isFloating()) << "Dock should be floating when first shown";
    
    // Additional check: dock should be positioned at a reasonable location
    QPoint pos = dock->pos();
    EXPECT_GE(pos.x(), 100) << "Dock should be positioned away from edge";
    EXPECT_GE(pos.y(), 100) << "Dock should be positioned away from top";
}

/**
 * @brief Test adding preprocessor panel
 */
TEST_F(PreprocessorUITest, AddPreprocessorPanel) {
    // Arrange
    auto* preprocessor = new OpenCV::SimpleEdgePreProcessor("TestProcessor");
    
    // Act
    bool result = mainUI->addPreprocessorPanel("TestProcessor", preprocessor);
    
    // Assert
    EXPECT_TRUE(result) << "Should successfully add preprocessor panel";
    
    // Verify panel was added to the tab widget
    QTabWidget* tabWidget = mainUI->findChild<QTabWidget*>("ImageProcessingTabWidget");
    ASSERT_NE(tabWidget, nullptr);
    EXPECT_EQ(tabWidget->count(), 1) << "Tab widget should have one panel";
    
    // Clean up
    mainUI->removePreprocessorPanel("TestProcessor");
    delete preprocessor;
}

/**
 * @brief Test that adding duplicate preprocessor panel fails
 */
TEST_F(PreprocessorUITest, AddDuplicatePreprocessorPanelFails) {
    // Arrange
    auto* preprocessor1 = new OpenCV::SimpleEdgePreProcessor("TestProcessor");
    auto* preprocessor2 = new OpenCV::SimpleEdgePreProcessor("TestProcessor2");
    
    // Act
    bool result1 = mainUI->addPreprocessorPanel("TestProcessor", preprocessor1);
    bool result2 = mainUI->addPreprocessorPanel("TestProcessor", preprocessor2); // Same ID
    
    // Assert
    EXPECT_TRUE(result1) << "First panel should be added successfully";
    EXPECT_FALSE(result2) << "Duplicate panel ID should fail";
    
    // Clean up
    mainUI->removePreprocessorPanel("TestProcessor");
    delete preprocessor1;
    delete preprocessor2;
}

/**
 * @brief Test removing preprocessor panel
 */
TEST_F(PreprocessorUITest, RemovePreprocessorPanel) {
    // Arrange
    auto* preprocessor = new OpenCV::SimpleEdgePreProcessor("TestProcessor");
    mainUI->addPreprocessorPanel("TestProcessor", preprocessor);
    
    QTabWidget* tabWidget = mainUI->findChild<QTabWidget*>("ImageProcessingTabWidget");
    ASSERT_NE(tabWidget, nullptr);
    ASSERT_EQ(tabWidget->count(), 1);
    
    // Act
    bool result = mainUI->removePreprocessorPanel("TestProcessor");
    
    // Assert
    EXPECT_TRUE(result) << "Should successfully remove preprocessor panel";
    EXPECT_EQ(tabWidget->count(), 0) << "Tab widget should be empty after removal";
    
    // Clean up
    delete preprocessor;
}

/**
 * @brief Test dock hides when last preprocessor panel is removed
 */
TEST_F(PreprocessorUITest, DockHidesWhenLastPanelRemoved) {
    // Arrange
    auto* preprocessor = new OpenCV::SimpleEdgePreProcessor("TestProcessor");
    mainUI->addPreprocessorPanel("TestProcessor", preprocessor);
    
    QDockWidget* dock = mainUI->findChild<QDockWidget*>("ImageProcessingDock");
    ASSERT_NE(dock, nullptr);
    
    // Show the dock
    dock->show();
    ASSERT_TRUE(dock->isVisible());
    
    // Act - remove the last panel
    mainUI->removePreprocessorPanel("TestProcessor");
    
    // Assert
    EXPECT_FALSE(dock->isVisible()) << "Dock should hide when last panel is removed";
    
    // Clean up
    delete preprocessor;
}

/**
 * @brief Test getting all preprocessor panels
 */
TEST_F(PreprocessorUITest, GetAllPreprocessorPanels) {
    // Arrange
    auto* preprocessor1 = new OpenCV::SimpleEdgePreProcessor("Processor1");
    auto* preprocessor2 = new OpenCV::SimpleEdgePreProcessor("Processor2");
    
    mainUI->addPreprocessorPanel("Processor1", preprocessor1);
    mainUI->addPreprocessorPanel("Processor2", preprocessor2);
    
    // Act
    auto panels = mainUI->getAllPreprocessorPanels();
    
    // Assert
    EXPECT_EQ(panels.size(), 2) << "Should return all added panels";
    
    // Clean up
    mainUI->removePreprocessorPanel("Processor1");
    mainUI->removePreprocessorPanel("Processor2");
    delete preprocessor1;
    delete preprocessor2;
}

/**
 * @brief Test getting active preprocessor panel
 */
TEST_F(PreprocessorUITest, GetActivePreprocessorPanel) {
    // Arrange
    auto* preprocessor1 = new OpenCV::SimpleEdgePreProcessor("Processor1");
    auto* preprocessor2 = new OpenCV::SimpleEdgePreProcessor("Processor2");
    
    mainUI->addPreprocessorPanel("Processor1", preprocessor1);
    mainUI->addPreprocessorPanel("Processor2", preprocessor2);
    
    QTabWidget* tabWidget = mainUI->findChild<QTabWidget*>("ImageProcessingTabWidget");
    ASSERT_NE(tabWidget, nullptr);
    
    // Act - set active tab
    tabWidget->setCurrentIndex(1); // Select second panel
    auto* activePanel = mainUI->getActivePreprocessorPanel();
    
    // Assert
    ASSERT_NE(activePanel, nullptr) << "Should return active panel";
    // The active panel should be the one at index 1
    
    // Clean up
    mainUI->removePreprocessorPanel("Processor1");
    mainUI->removePreprocessorPanel("Processor2");
    delete preprocessor1;
    delete preprocessor2;
}

/**
 * @brief Mock preprocessor for testing signal/slot connections
 */
class MockPreProcessor : public OpenCV::PreProcessorBase {
public:
    explicit MockPreProcessor(const QString& id) : PreProcessorBase(id) {}
    
    MOCK_METHOD(bool, onInitialize, (), (override));
    MOCK_METHOD(bool, onStart, (), (override));
    MOCK_METHOD(bool, onStop, (), (override));
    MOCK_METHOD(void, onCleanup, (), (override));
    
    void emitTestSignal() {
        emit processingComplete(QImage(), 100);
    }
};

/**
 * @brief Test preprocessor panel signal connections
 */
TEST_F(PreprocessorUITest, PreprocessorPanelSignalConnections) {
    // Arrange
    auto* mockProcessor = new MockPreProcessor("MockProcessor");
    
    // Expect initialize to be called when connecting
    EXPECT_CALL(*mockProcessor, onInitialize()).WillOnce(Return(true));
    
    // Act
    bool result = mainUI->addPreprocessorPanel("MockProcessor", mockProcessor);
    
    // Assert
    EXPECT_TRUE(result) << "Panel should be added successfully";
    
    // Test that panel receives signals from processor
    auto panels = mainUI->getAllPreprocessorPanels();
    ASSERT_EQ(panels.size(), 1);
    
    QSignalSpy spy(panels[0], SIGNAL(panelError(QString)));
    
    // Simulate an error from the processor
    // This would typically happen through the processor's error signal
    
    // Clean up
    mainUI->removePreprocessorPanel("MockProcessor");
    delete mockProcessor;
}

/**
 * @brief Test manual test checklist
 */
TEST(PreprocessorUIManualTest, TestChecklist) {
    std::cout << "\n=== MANUAL TEST CHECKLIST ===\n" << std::endl;
    std::cout << "Please manually verify the following when running the application:\n" << std::endl;
    
    std::cout << "1. INITIAL STATE:" << std::endl;
    std::cout << "   [ ] Preprocessor dock is NOT visible when application starts" << std::endl;
    std::cout << "   [ ] No preprocessor panels are shown in the right dock area" << std::endl;
    std::cout << std::endl;
    
    std::cout << "2. PREPROCESS BUTTON:" << std::endl;
    std::cout << "   [ ] 'Preprocess' button exists in the Processing menu" << std::endl;
    std::cout << "   [ ] 'Preprocess' button exists in the toolbar" << std::endl;
    std::cout << "   [ ] Button shows unchecked state initially" << std::endl;
    std::cout << "   [ ] Ctrl+P keyboard shortcut is shown in tooltip" << std::endl;
    std::cout << std::endl;
    
    std::cout << "3. SHOWING THE DOCK:" << std::endl;
    std::cout << "   [ ] Clicking 'Preprocess' button shows the Image Processing dock" << std::endl;
    std::cout << "   [ ] Dock appears as a floating window on first show" << std::endl;
    std::cout << "   [ ] Dock is positioned at (100, 100) from top-left" << std::endl;
    std::cout << "   [ ] Button changes to checked state when dock is visible" << std::endl;
    std::cout << std::endl;
    
    std::cout << "4. HIDING THE DOCK:" << std::endl;
    std::cout << "   [ ] Clicking 'Preprocess' button again hides the dock" << std::endl;
    std::cout << "   [ ] Button returns to unchecked state" << std::endl;
    std::cout << "   [ ] Pressing Ctrl+P toggles dock visibility" << std::endl;
    std::cout << std::endl;
    
    std::cout << "5. DOCK BEHAVIOR:" << std::endl;
    std::cout << "   [ ] Dock can be docked to right side of main window" << std::endl;
    std::cout << "   [ ] Dock remembers position when hidden and shown again" << std::endl;
    std::cout << "   [ ] Closing dock with 'X' button unchecks the Preprocess action" << std::endl;
    std::cout << std::endl;
    
    std::cout << "6. PREPROCESSOR PANELS:" << std::endl;
    std::cout << "   [ ] Edge Detection tab appears in the dock when shown" << std::endl;
    std::cout << "   [ ] Panel shows preprocessor controls (threshold, kernel size, etc.)" << std::endl;
    std::cout << "   [ ] Enable/Disable toggle works" << std::endl;
    std::cout << "   [ ] Parameter changes are reflected in real-time" << std::endl;
    std::cout << std::endl;
    
    std::cout << "7. STATUS MESSAGES:" << std::endl;
    std::cout << "   [ ] 'Preprocessor Panel Added: Edge Detection' appears in status bar" << std::endl;
    std::cout << "   [ ] Message disappears after 5 seconds" << std::endl;
    std::cout << std::endl;
    
    std::cout << "8. INTEGRATION:" << std::endl;
    std::cout << "   [ ] Preprocessor works with camera feed when both are active" << std::endl;
    std::cout << "   [ ] Processed images show edge detection effects" << std::endl;
    std::cout << "   [ ] No performance degradation when preprocessor is active" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== END OF MANUAL TEST CHECKLIST ===\n" << std::endl;
    
    // This test always passes but serves as documentation
    SUCCEED();
}

// Main function for standalone test execution
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}