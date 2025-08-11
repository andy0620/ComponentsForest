# ComponentsForest QML Camera Viewer Docker Build
# Multi-stage build for optimized container size

# Build stage
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    libgl1-mesa-dev \
    libxkbcommon-dev \
    libxcb-xinerama0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xfixes0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Qt6 from official installer
ENV QT_VERSION=6.9.1
ENV QT_PATH=/opt/Qt

# Download and install Qt (non-interactive)
RUN mkdir -p /tmp/qt && cd /tmp/qt \
    && wget -q https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt6_691/qt.qt6.691.gcc_64/6.9.1-0-202407310945qtbase-Linux-RHEL_8_8-GCC-Linux-RHEL_8_8-X86_64.7z \
    && apt-get update && apt-get install -y p7zip-full \
    && 7z x *.7z -o${QT_PATH} \
    && rm -rf /tmp/qt

# Alternative: Use system Qt packages (if available)
# RUN apt-get update && apt-get install -y \
#     qt6-base-dev \
#     qt6-declarative-dev \
#     qt6-quickcontrols2-dev \
#     qt6-charts-dev \
#     && rm -rf /var/lib/apt/lists/*

# Set Qt environment
ENV PATH=${QT_PATH}/6.9.1/gcc_64/bin:${PATH}
ENV LD_LIBRARY_PATH=${QT_PATH}/6.9.1/gcc_64/lib:${LD_LIBRARY_PATH}
ENV QT_PLUGIN_PATH=${QT_PATH}/6.9.1/gcc_64/plugins
ENV QML2_IMPORT_PATH=${QT_PATH}/6.9.1/gcc_64/qml

# Copy source code
WORKDIR /app
COPY . .

# Build the application
RUN mkdir -p build && cd build \
    && cmake ../viewers/qml_camera_viewer \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=${QT_PATH}/6.9.1/gcc_64 \
        -DBUILD_STANDALONE=ON \
        -DDEPLOY_QT_RUNTIME=OFF \
        -GNinja \
    && ninja

# Runtime stage
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libgl1-mesa-glx \
    libxkbcommon0 \
    libxcb-xinerama0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xfixes0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libdbus-1-3 \
    libfontconfig1 \
    libfreetype6 \
    && rm -rf /var/lib/apt/lists/*

# Copy Qt libraries from builder
COPY --from=builder /opt/Qt/6.9.1/gcc_64/lib /opt/Qt/6.9.1/gcc_64/lib
COPY --from=builder /opt/Qt/6.9.1/gcc_64/plugins /opt/Qt/6.9.1/gcc_64/plugins
COPY --from=builder /opt/Qt/6.9.1/gcc_64/qml /opt/Qt/6.9.1/gcc_64/qml

# Copy built application
COPY --from=builder /app/build/bin /app/bin
COPY --from=builder /app/viewers/qml_camera_viewer/qml /app/qml

# Set environment
ENV LD_LIBRARY_PATH=/opt/Qt/6.9.1/gcc_64/lib:${LD_LIBRARY_PATH}
ENV QT_PLUGIN_PATH=/opt/Qt/6.9.1/gcc_64/plugins
ENV QML2_IMPORT_PATH=/app/qml:/opt/Qt/6.9.1/gcc_64/qml
ENV QT_QUICK_CONTROLS_STYLE=Material
ENV QSG_RENDER_LOOP=threaded

# Create non-root user
RUN useradd -m -s /bin/bash appuser && \
    chown -R appuser:appuser /app

USER appuser
WORKDIR /app

# Entry point
ENTRYPOINT ["/app/bin/QMLCameraViewer"]

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD pgrep QMLCameraViewer || exit 1