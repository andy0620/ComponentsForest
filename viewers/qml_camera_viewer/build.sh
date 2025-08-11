#!/bin/bash

# Local build script for QML Camera Viewer
# Can be run directly from the viewer directory

set -e

# Get the viewer directory (where this script is located)
VIEWER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Get the project root (two levels up)
PROJECT_ROOT="$(cd "$VIEWER_DIR/../.." && pwd)"

# Run the main build script from project root
cd "$PROJECT_ROOT"

# Check if main build script exists
if [ -f "build_qml_viewer.sh" ]; then
    exec ./build_qml_viewer.sh "$@"
else
    echo "Error: Main build script not found at project root"
    echo "Expected location: $PROJECT_ROOT/build_qml_viewer.sh"
    exit 1
fi