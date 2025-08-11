# ComponentsForest QML Camera Viewer Makefile
# Provides convenient build targets for development

.PHONY: all build clean rebuild run debug release help

# Default target
all: build

# Build in release mode
build: release

# Build in release mode
release:
	@echo "Building QML Camera Viewer (Release)..."
	@if [ -f "../../build_qml_viewer.sh" ]; then \
		cd ../.. && ./build_qml_viewer.sh --no-clean; \
	else \
		echo "Error: Build script not found"; \
		exit 1; \
	fi

# Build in debug mode
debug:
	@echo "Building QML Camera Viewer (Debug)..."
	@if [ -f "../../build_qml_viewer.sh" ]; then \
		cd ../.. && ./build_qml_viewer.sh --debug --no-clean; \
	else \
		echo "Error: Build script not found"; \
		exit 1; \
	fi

# Clean build directories
clean:
	@echo "Cleaning build directories..."
	@rm -rf ../../build_qml_release ../../build_qml_debug
	@echo "Clean complete."

# Clean and rebuild
rebuild: clean build

# Run the application
run:
	@if [ -f "../../build_qml_release/run_qml_viewer.sh" ]; then \
		cd ../../build_qml_release && ./run_qml_viewer.sh; \
	elif [ -f "../../build_qml_release/QMLCameraViewer" ]; then \
		cd ../../build_qml_release && ./QMLCameraViewer; \
	elif [ -f "../../build_qml_release/bin/QMLCameraViewer" ]; then \
		cd ../../build_qml_release && ./bin/QMLCameraViewer; \
	else \
		echo "Error: QMLCameraViewer executable not found. Run 'make build' first."; \
		exit 1; \
	fi

# Run debug version
run-debug:
	@if [ -f "../../build_qml_debug/run_qml_viewer.sh" ]; then \
		cd ../../build_qml_debug && ./run_qml_viewer.sh; \
	elif [ -f "../../build_qml_debug/QMLCameraViewer" ]; then \
		cd ../../build_qml_debug && ./QMLCameraViewer; \
	elif [ -f "../../build_qml_debug/bin/QMLCameraViewer" ]; then \
		cd ../../build_qml_debug && ./bin/QMLCameraViewer; \
	else \
		echo "Error: Debug executable not found. Run 'make debug' first."; \
		exit 1; \
	fi

# Build with OpenCV support
opencv:
	@echo "Building QML Camera Viewer with OpenCV support..."
	@if [ -f "../../build_qml_viewer.sh" ]; then \
		cd ../.. && ./build_qml_viewer.sh --with-opencv --no-clean; \
	else \
		echo "Error: Build script not found"; \
		exit 1; \
	fi

# Build as integrated (not standalone)
integrated:
	@echo "Building QML Camera Viewer (Integrated mode)..."
	@if [ -f "../../build_qml_viewer.sh" ]; then \
		cd ../.. && ./build_qml_viewer.sh --integrated --no-clean; \
	else \
		echo "Error: Build script not found"; \
		exit 1; \
	fi

# Configure only (don't build)
configure:
	@echo "Configuring QML Camera Viewer..."
	@mkdir -p ../../build_qml_release
	@cd ../../build_qml_release && cmake ../viewers/qml_camera_viewer \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_STANDALONE=ON

# Show help
help:
	@echo "ComponentsForest QML Camera Viewer - Make Targets"
	@echo "================================================="
	@echo ""
	@echo "  make [all|build]  - Build in release mode (default)"
	@echo "  make release      - Build in release mode"
	@echo "  make debug        - Build in debug mode"
	@echo "  make clean        - Clean all build directories"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make run          - Run the release build"
	@echo "  make run-debug    - Run the debug build"
	@echo "  make opencv       - Build with OpenCV support"
	@echo "  make integrated   - Build as integrated (not standalone)"
	@echo "  make configure    - Configure only (don't build)"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              - Build release version"
	@echo "  make debug run-debug  - Build and run debug version"
	@echo "  make clean release run  - Clean, rebuild, and run"
	@echo ""

# Check dependencies
check-deps:
	@echo "Checking build dependencies..."
	@command -v cmake >/dev/null 2>&1 || { echo "Error: cmake not found"; exit 1; }
	@command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "Error: C++ compiler not found"; exit 1; }
	@echo "Checking for Qt6..."
	@cmake --find-package -DNAME=Qt6 -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST >/dev/null 2>&1 || \
		{ echo "Warning: Qt6 not found in system paths. Set CMAKE_PREFIX_PATH if needed."; }
	@echo "Dependencies check complete."

# Install dependencies (Ubuntu/Debian)
install-deps-ubuntu:
	@echo "Installing dependencies for Ubuntu/Debian..."
	sudo apt-get update
	sudo apt-get install -y \
		build-essential cmake ninja-build \
		qt6-base-dev qt6-declarative-dev \
		qt6-quickcontrols2-dev qt6-charts-dev \
		libgl1-mesa-dev libxkbcommon-dev
	@echo "Dependencies installed."

# Install dependencies (Fedora)
install-deps-fedora:
	@echo "Installing dependencies for Fedora..."
	sudo dnf install -y \
		gcc-c++ cmake ninja-build \
		qt6-qtbase-devel qt6-qtdeclarative-devel \
		qt6-qtquickcontrols2-devel qt6-qtcharts-devel \
		mesa-libGL-devel libxkbcommon-devel
	@echo "Dependencies installed."