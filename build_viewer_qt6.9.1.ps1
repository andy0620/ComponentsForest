#############################################################################
# Do3Think Camera Viewer Build Script for Qt 6.9.1 (Windows)
# Author: ComponentsForest Build System
# Date: 2025-08-09
#############################################################################

param(
    [string]$QtPath = "",
    [switch]$Clean = $false,
    [string]$BuildType = "Release",
    [switch]$Help = $false
)

# Color functions
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

function Write-Info { Write-Host $args -ForegroundColor Blue }
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }
function Write-Error { Write-Host $args -ForegroundColor Red }

# Show help
if ($Help) {
    Write-Host @"
Usage: .\build_viewer_qt6.9.1.ps1 [OPTIONS]

Options:
    -QtPath <path>     Specify Qt 6.9.1 installation path
    -Clean             Clean build (remove existing build directories)
    -BuildType <type>  Build type: Debug or Release (default: Release)
    -Help              Show this help message

Examples:
    .\build_viewer_qt6.9.1.ps1
    .\build_viewer_qt6.9.1.ps1 -QtPath "C:\Qt\6.9.1\msvc2019_64"
    .\build_viewer_qt6.9.1.ps1 -Clean -BuildType Debug
"@
    exit 0
}

Write-Info "========================================"
Write-Info "Do3Think Camera Viewer Build Script"
Write-Info "Qt 6.9.1 Configuration for Windows"
Write-Info "========================================"

# Function to find Qt installation
function Find-Qt {
    param([string]$CustomPath)
    
    Write-Warning "Searching for Qt 6.9.1 installation..."
    
    $searchPaths = @(
        "$env:USERPROFILE\Qt\6.9.1\msvc2019_64",
        "$env:USERPROFILE\Qt\6.9.1\mingw_64",
        "C:\Qt\6.9.1\msvc2019_64",
        "C:\Qt\6.9.1\mingw_64",
        "D:\Qt\6.9.1\msvc2019_64",
        "D:\Qt\6.9.1\mingw_64",
        "C:\Qt6\6.9.1\msvc2019_64",
        "$env:ProgramFiles\Qt\6.9.1\msvc2019_64"
    )
    
    if ($CustomPath) {
        $searchPaths = @($CustomPath) + $searchPaths
    }
    
    foreach ($path in $searchPaths) {
        if (Test-Path "$path\bin\qmake.exe") {
            Write-Success "Found Qt 6.9.1 at: $path"
            return $path
        }
    }
    
    # If not found, ask user
    Write-Warning "Qt 6.9.1 not found in standard locations."
    $userPath = Read-Host "Please enter the full path to your Qt 6.9.1 installation (e.g., C:\Qt\6.9.1\msvc2019_64)"
    
    if (Test-Path "$userPath\bin\qmake.exe") {
        Write-Success "Using Qt installation at: $userPath"
        return $userPath
    } else {
        Write-Error "Error: Qt not found at $userPath"
        return $null
    }
}

# Function to check dependencies
function Check-Dependencies {
    Write-Warning "Checking build dependencies..."
    
    $missing = @()
    
    # Check for CMake
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) {
        $cmakeVersion = & cmake --version | Select-Object -First 1
        Write-Success "✓ CMake found: $cmakeVersion"
    } else {
        $missing += "CMake"
    }
    
    # Check for Visual Studio or MinGW
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $hasVS = $false
    $hasMinGW = $false
    
    if (Test-Path $vsWhere) {
        $vsInstalls = & $vsWhere -latest -property installationPath
        if ($vsInstalls) {
            Write-Success "✓ Visual Studio found: $vsInstalls"
            $hasVS = $true
        }
    }
    
    $mingw = Get-Command g++ -ErrorAction SilentlyContinue
    if ($mingw) {
        Write-Success "✓ MinGW found"
        $hasMinGW = $true
    }
    
    if (-not $hasVS -and -not $hasMinGW) {
        $missing += "Visual Studio or MinGW"
    }
    
    # Check for Git (optional)
    $git = Get-Command git -ErrorAction SilentlyContinue
    if ($git) {
        Write-Success "✓ Git found"
    } else {
        Write-Warning "⚠ Git not found (optional)"
    }
    
    if ($missing.Count -gt 0) {
        Write-Error "Missing dependencies: $($missing -join ', ')"
        Write-Warning "Please install missing dependencies and try again."
        return $false
    }
    
    return $true
}

# Function to determine generator
function Get-CMakeGenerator {
    param([string]$QtPath)
    
    # Check Qt compiler type
    if ($QtPath -match "msvc") {
        # Visual Studio
        $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vsWhere) {
            $vsVersion = & $vsWhere -latest -property catalog_productLineVersion
            switch ($vsVersion) {
                "2022" { return "Visual Studio 17 2022" }
                "2019" { return "Visual Studio 16 2019" }
                "2017" { return "Visual Studio 15 2017" }
                default { return "Visual Studio 16 2019" }
            }
        }
        return "Visual Studio 16 2019"
    } elseif ($QtPath -match "mingw") {
        return "MinGW Makefiles"
    } else {
        # Try to detect from system
        if (Get-Command cl -ErrorAction SilentlyContinue) {
            return "NMake Makefiles"
        } elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
            return "MinGW Makefiles"
        } else {
            return "Visual Studio 16 2019"
        }
    }
}

# Function to build core components
function Build-CoreComponents {
    param([string]$QtPath, [string]$Generator, [string]$BuildType)
    
    Write-Warning "Building ComponentsForest Core library..."
    
    $buildDir = "build_core_win"
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    
    Push-Location $buildDir
    
    try {
        # Configure with CMake
        $cmakeArgs = @(
            "..",
            "-G", "`"$Generator`"",
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DCMAKE_PREFIX_PATH=`"$QtPath`"",
            "-DQt6_DIR=`"$QtPath\lib\cmake\Qt6`"",
            "-DBUILD_SHARED_LIBS=ON",
            "-DBUILD_EXAMPLES=OFF"
        )
        
        if ($Generator -match "Visual Studio") {
            $cmakeArgs += "-A", "x64"
        }
        
        Write-Info "Running CMake configuration..."
        & cmake $cmakeArgs
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
        
        # Build
        Write-Info "Building ComponentsForestCore..."
        & cmake --build . --config $BuildType --target ComponentsForestCore --parallel
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed for ComponentsForestCore"
        }
        
        Write-Success "✓ ComponentsForest Core built successfully"
    }
    catch {
        Write-Error "Failed to build ComponentsForest Core: $_"
        Pop-Location
        return $false
    }
    
    Pop-Location
    return $true
}

# Function to build Do3Think component
function Build-Do3ThinkComponent {
    param([string]$BuildType)
    
    Write-Warning "Building Do3Think Camera Component..."
    
    Push-Location "build_core_win"
    
    try {
        & cmake --build . --config $BuildType --target Do3ThinkCameraComponent --parallel
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed for Do3ThinkCameraComponent"
        }
        
        Write-Success "✓ Do3Think Camera Component built successfully"
    }
    catch {
        Write-Error "Failed to build Do3Think Camera Component: $_"
        Pop-Location
        return $false
    }
    
    Pop-Location
    return $true
}

# Function to build viewer application
function Build-Viewer {
    param([string]$QtPath, [string]$Generator, [string]$BuildType)
    
    Write-Warning "Building Do3Think Camera Viewer application..."
    
    $viewerBuildDir = "build_viewer_win"
    if (Test-Path $viewerBuildDir) {
        Remove-Item -Recurse -Force $viewerBuildDir
    }
    New-Item -ItemType Directory -Path $viewerBuildDir | Out-Null
    
    Push-Location $viewerBuildDir
    
    try {
        # Configure viewer with CMake
        $cmakeArgs = @(
            "..\viewers\do3think_camera_viewer",
            "-G", "`"$Generator`"",
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DCMAKE_PREFIX_PATH=`"$QtPath`"",
            "-DQt6_DIR=`"$QtPath\lib\cmake\Qt6`"",
            "-DComponentsForestCore_DIR=`"..\build_core_win`"",
            "-DDo3ThinkCameraComponent_DIR=`"..\build_core_win`""
        )
        
        if ($Generator -match "Visual Studio") {
            $cmakeArgs += "-A", "x64"
        }
        
        Write-Info "Configuring viewer with CMake..."
        & cmake $cmakeArgs
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed for viewer"
        }
        
        # Build viewer
        Write-Info "Building viewer..."
        & cmake --build . --config $BuildType --parallel
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed for viewer"
        }
        
        Write-Success "✓ Do3Think Camera Viewer built successfully"
        
        $exePath = if ($BuildType -eq "Debug") { "Debug\Do3ThinkCameraViewer.exe" } else { "Release\Do3ThinkCameraViewer.exe" }
        if (Test-Path $exePath) {
            Write-Success "Executable location: $(Get-Location)\$exePath"
        }
    }
    catch {
        Write-Error "Failed to build viewer: $_"
        Pop-Location
        return $false
    }
    
    Pop-Location
    return $true
}

# Function to copy runtime dependencies
function Copy-RuntimeDependencies {
    param([string]$QtPath, [string]$BuildType)
    
    Write-Warning "Copying runtime dependencies..."
    
    $viewerBinDir = "build_viewer_win\$BuildType"
    if (-not (Test-Path $viewerBinDir)) {
        $viewerBinDir = "build_viewer_win"
    }
    
    # Copy core libraries
    $coreLibDir = "build_core_win\$BuildType"
    if (-not (Test-Path $coreLibDir)) {
        $coreLibDir = "build_core_win"
    }
    
    $filesToCopy = @(
        "ComponentsForestCore.dll",
        "Do3ThinkCameraComponent.dll"
    )
    
    foreach ($file in $filesToCopy) {
        $sourcePath = Join-Path $coreLibDir $file
        if (Test-Path $sourcePath) {
            Copy-Item $sourcePath $viewerBinDir -Force
            Write-Success "✓ Copied $file"
        }
    }
    
    # Copy DVPCamera DLL
    $dvpCameraPaths = @(
        "Do3ThinkCamera\SDK\DVPCamera64.dll",
        "package\Release\DVPCamera64.dll"
    )
    
    foreach ($path in $dvpCameraPaths) {
        if (Test-Path $path) {
            Copy-Item $path $viewerBinDir -Force
            Write-Success "✓ Copied DVPCamera64.dll"
            break
        }
    }
    
    # Deploy Qt dependencies
    $windeployqt = Join-Path $QtPath "bin\windeployqt.exe"
    if (Test-Path $windeployqt) {
        Write-Info "Deploying Qt dependencies..."
        $exePath = Join-Path $viewerBinDir "Do3ThinkCameraViewer.exe"
        if (Test-Path $exePath) {
            & $windeployqt --$($BuildType.ToLower()) --no-translations --no-system-d3d-compiler --no-opengl-sw $exePath
            Write-Success "✓ Qt dependencies deployed"
        }
    } else {
        Write-Warning "windeployqt not found. You may need to manually copy Qt DLLs."
    }
    
    # Create run batch file
    $runScript = @"
@echo off
setlocal
set PATH=%~dp0;%PATH%
start Do3ThinkCameraViewer.exe %*
endlocal
"@
    
    $runScriptPath = Join-Path $viewerBinDir "run_viewer.bat"
    Set-Content -Path $runScriptPath -Value $runScript
    Write-Success "✓ Created run script: $runScriptPath"
}

# Function to create development environment setup
function Create-DevSetup {
    param([string]$QtPath)
    
    Write-Warning "Creating development environment setup..."
    
    $setupScript = @"
@echo off
rem Qt 6.9.1 Environment Setup
set QT_PATH=$QtPath
set PATH=%QT_PATH%\bin;%PATH%
set CMAKE_PREFIX_PATH=%QT_PATH%;%CMAKE_PREFIX_PATH%

echo Qt 6.9.1 environment configured
echo Qt Path: %QT_PATH%
echo You can now build Qt applications
"@
    
    Set-Content -Path "setup_qt_env.bat" -Value $setupScript
    Write-Success "✓ Created environment setup script: setup_qt_env.bat"
}

# Main build process
function Main {
    # Clean existing builds if requested
    if ($Clean) {
        Write-Warning "Cleaning existing build directories..."
        @("build_core_win", "build_viewer_win", "build") | ForEach-Object {
            if (Test-Path $_) {
                Remove-Item -Recurse -Force $_
            }
        }
        Write-Success "✓ Build directories cleaned"
    }
    
    # Step 1: Find Qt
    $qt = Find-Qt -CustomPath $QtPath
    if (-not $qt) {
        Write-Error "Failed to find Qt 6.9.1 installation"
        exit 1
    }
    
    # Step 2: Check dependencies
    if (-not (Check-Dependencies)) {
        Write-Error "Missing required dependencies"
        exit 1
    }
    
    # Step 3: Determine CMake generator
    $generator = Get-CMakeGenerator -QtPath $qt
    Write-Info "Using CMake generator: $generator"
    
    # Step 4: Build core components
    if (-not (Build-CoreComponents -QtPath $qt -Generator $generator -BuildType $BuildType)) {
        Write-Error "Failed to build core components"
        exit 1
    }
    
    # Step 5: Build Do3Think component
    if (-not (Build-Do3ThinkComponent -BuildType $BuildType)) {
        Write-Error "Failed to build Do3Think component"
        exit 1
    }
    
    # Step 6: Build viewer application
    if (-not (Build-Viewer -QtPath $qt -Generator $generator -BuildType $BuildType)) {
        Write-Error "Failed to build viewer application"
        exit 1
    }
    
    # Step 7: Copy runtime dependencies
    Copy-RuntimeDependencies -QtPath $qt -BuildType $BuildType
    
    # Step 8: Create development setup
    Create-DevSetup -QtPath $qt
    
    Write-Host ""
    Write-Success "========================================"
    Write-Success "Build completed successfully!"
    Write-Success "========================================"
    Write-Host ""
    Write-Info "To run the viewer:"
    Write-Host "  cd build_viewer_win\$BuildType"
    Write-Host "  .\run_viewer.bat"
    Write-Host ""
    Write-Info "To set up Qt environment for development:"
    Write-Host "  .\setup_qt_env.bat"
    Write-Host ""
    Write-Info "Build artifacts:"
    Write-Host "  Core library: build_core_win\$BuildType\ComponentsForestCore.dll"
    Write-Host "  Do3Think component: build_core_win\$BuildType\Do3ThinkCameraComponent.dll"
    Write-Host "  Viewer application: build_viewer_win\$BuildType\Do3ThinkCameraViewer.exe"
    Write-Host ""
}

# Run main function
Main