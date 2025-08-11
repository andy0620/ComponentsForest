# PowerShell script to build Do3ThinkCameraComponent.dll

Write-Host "Building Do3ThinkCameraComponent.dll..." -ForegroundColor Green

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "Error: build directory not found. Please run CMake first." -ForegroundColor Red
    exit 1
}

# Path to MSBuild (adjust if your VS installation is different)
$msbuildPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
)

$msbuild = $null
foreach ($path in $msbuildPaths) {
    if (Test-Path $path) {
        $msbuild = $path
        break
    }
}

if (-not $msbuild) {
    Write-Host "Error: MSBuild not found. Please install Visual Studio." -ForegroundColor Red
    exit 1
}

Write-Host "Using MSBuild: $msbuild" -ForegroundColor Yellow

# Build the DVPCamera_stub first
Write-Host "`nBuilding DVPCamera_stub..." -ForegroundColor Cyan
& $msbuild "build\DVPCamera_stub.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Failed to build DVPCamera_stub" -ForegroundColor Red
    exit 1
}

# Build Do3ThinkCameraComponent
Write-Host "`nBuilding Do3ThinkCameraComponent..." -ForegroundColor Cyan
& $msbuild "build\Do3ThinkCameraComponent.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nSUCCESS: Do3ThinkCameraComponent.dll has been built successfully!" -ForegroundColor Green
    Write-Host "Location: build\Release\Do3ThinkCameraComponent.dll" -ForegroundColor Green
    
    # List the generated files
    if (Test-Path "build\Release") {
        Write-Host "`nGenerated files:" -ForegroundColor Yellow
        Get-ChildItem "build\Release\*.dll", "build\Release\*.lib" | ForEach-Object {
            Write-Host "  - $($_.Name)" -ForegroundColor White
        }
    }
} else {
    Write-Host "`nERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "`nPress any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")