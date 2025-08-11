#include <gtest/gtest.h>
#include <QApplication>
#include <QJsonObject>
#include <QSignalSpy>
#include "components/base_component.h"

using namespace ComponentsForest;

// Test fixture for BaseComponent tests
class BaseComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Qt requires a QApplication instance for signals/slots
        if (!QApplication::instance()) {
            // Using a static variable to hold argc is a common trick for tests
            static int argc = 0;
            app = new QApplication(argc, nullptr);
        }
        component = new BaseComponent();
    }

    void TearDown() override {
        delete component;
        // Do not delete app here if it's shared across multiple test fixtures
    }

    BaseComponent* component;
    static QApplication* app;
};

QApplication* BaseComponentTest::app = nullptr;

TEST_F(BaseComponentTest, InitialStateIsUninitialized) {
    ASSERT_EQ(component->state(), ComponentState::Uninitialized);
}

TEST_F(BaseComponentTest, InitializeTransitionsToInitializedState) {
    QJsonObject config; // Empty config
    bool success = component->initialize(config);
    ASSERT_TRUE(success);
    EXPECT_EQ(component->state(), ComponentState::Initialized);
}

TEST_F(BaseComponentTest, InitializeEmitsCorrectSignals) {
    QSignalSpy stateSpy(component, &BaseComponent::stateChanged);
    QSignalSpy initializedSpy(component, &BaseComponent::initialized);

    QJsonObject config;
    component->initialize(config);

    EXPECT_EQ(stateSpy.count(), 1);
    QList<QVariant> stateArgs = stateSpy.takeFirst();
    EXPECT_EQ(qvariant_cast<ComponentState>(stateArgs.at(0)), ComponentState::Initialized);

    EXPECT_EQ(initializedSpy.count(), 1);
}

TEST_F(BaseComponentTest, StartTransitionsToRunningState) {
    QJsonObject config;
    component->initialize(config);

    bool success = component->start();
    ASSERT_TRUE(success);
    EXPECT_EQ(component->state(), ComponentState::Running);
}

TEST_F(BaseComponentTest, StartEmitsCorrectSignals) {
    component->initialize(QJsonObject());

    QSignalSpy stateSpy(component, &BaseComponent::stateChanged);
    QSignalSpy startedSpy(component, &BaseComponent::started);

    component->start();

    EXPECT_EQ(stateSpy.count(), 1);
    QList<QVariant> stateArgs = stateSpy.takeFirst();
    EXPECT_EQ(qvariant_cast<ComponentState>(stateArgs.at(0)), ComponentState::Running);

    EXPECT_EQ(startedSpy.count(), 1);
}

TEST_F(BaseComponentTest, StopTransitionsToStoppedState) {
    component->initialize(QJsonObject());
    component->start();

    bool success = component->stop();
    ASSERT_TRUE(success);
    EXPECT_EQ(component->state(), ComponentState::Stopped);
}

TEST_F(BaseComponentTest, StopEmitsCorrectSignals) {
    component->initialize(QJsonObject());
    component->start();

    QSignalSpy stateSpy(component, &BaseComponent::stateChanged);
    QSignalSpy stoppedSpy(component, &BaseComponent::stopped);

    component->stop();

    EXPECT_EQ(stateSpy.count(), 1);
    QList<QVariant> stateArgs = stateSpy.takeFirst();
    EXPECT_EQ(qvariant_cast<ComponentState>(stateArgs.at(0)), ComponentState::Stopped);

    EXPECT_EQ(stoppedSpy.count(), 1);
}

TEST_F(BaseComponentTest, CannotStartWhenUninitialized) {
    bool success = component->start();
    ASSERT_FALSE(success);
    EXPECT_EQ(component->state(), ComponentState::Uninitialized);
}

TEST_F(BaseComponentTest, CannotStopWhenNotRunning) {
    component->initialize(QJsonObject());
    bool success = component->stop();
    ASSERT_FALSE(success);
    EXPECT_EQ(component->state(), ComponentState::Initialized);
}
