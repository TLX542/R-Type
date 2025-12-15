# ============================================================================
# R-Type Dependency Bootstrap Script for Windows (PowerShell)
# ============================================================================
# This script helps install dependencies for building R-Type on Windows.
# It primarily uses vcpkg as the package manager.
#
# Usage:
#   .\scripts\bootstrap-deps.ps1 [-VcpkgPath <path>] [-SkipInstall]
#
# Parameters:
#   -VcpkgPath   Path to existing vcpkg installation (optional)
#   -SkipInstall Skip dependency installation (only show instructions)
# ============================================================================

param(
    [string]$VcpkgPath = "",
    [switch]$SkipInstall = $false
)

# Script directory and project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Color output functions
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Check if CMake is installed
function Test-CMakeInstalled {
    try {
        $null = Get-Command cmake -ErrorAction Stop
        return $true
    }
    catch {
        return $false
    }
}

# Check if Visual Studio is installed
function Test-VisualStudioInstalled {
    $vsPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019",
        "C:\Program Files\Microsoft Visual Studio\2019"
    )
    
    foreach ($path in $vsPaths) {
        if (Test-Path $path) {
            return $true
        }
    }
    return $false
}

# Install vcpkg
function Install-Vcpkg {
    param([string]$InstallPath)
    
    Write-Info "Installing vcpkg to: $InstallPath"
    
    if (Test-Path $InstallPath) {
        Write-Warning "vcpkg directory already exists at $InstallPath"
        return $InstallPath
    }
    
    try {
        Write-Info "Cloning vcpkg repository..."
        git clone https://github.com/Microsoft/vcpkg.git $InstallPath
        
        Write-Info "Bootstrapping vcpkg..."
        & "$InstallPath\bootstrap-vcpkg.bat"
        
        if ($LASTEXITCODE -ne 0) {
            throw "vcpkg bootstrap failed"
        }
        
        Write-Success "vcpkg installed successfully!"
        return $InstallPath
    }
    catch {
        Write-Error "Failed to install vcpkg: $_"
        return $null
    }
}

# Install dependencies via vcpkg
function Install-Dependencies {
    param([string]$VcpkgExe)
    
    Write-Info "Installing dependencies via vcpkg..."
    
    $triplet = "x64-windows"
    Write-Info "Using triplet: $triplet"
    
    Write-Info "Installing ASIO..."
    & $VcpkgExe install asio:$triplet
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Failed to install ASIO via vcpkg"
        Write-Host "  Troubleshooting:" -ForegroundColor Yellow
        Write-Host "    - Check your internet connection" -ForegroundColor Yellow
        Write-Host "    - Try running: vcpkg update" -ForegroundColor Yellow
        Write-Host "    - Check vcpkg logs for detailed error messages" -ForegroundColor Yellow
    }
    
    Write-Info "Installing raylib..."
    & $VcpkgExe install raylib:$triplet
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Failed to install raylib via vcpkg"
        Write-Host "  Troubleshooting:" -ForegroundColor Yellow
        Write-Host "    - Check your internet connection" -ForegroundColor Yellow
        Write-Host "    - Ensure you have Visual Studio C++ tools installed" -ForegroundColor Yellow
        Write-Host "    - Try running: vcpkg update" -ForegroundColor Yellow
    }
    
    Write-Success "Dependencies installation completed!"
}

# Show build instructions
function Show-BuildInstructions {
    param([string]$VcpkgPath)
    
    Write-Host ""
    Write-Success "Setup complete!"
    Write-Host ""
    Write-Info "Next steps to build R-Type:"
    Write-Host ""
    Write-Host "  1. Open PowerShell or Command Prompt" -ForegroundColor Cyan
    Write-Host "  2. Navigate to the project directory:" -ForegroundColor Cyan
    Write-Host "       cd `"$ProjectRoot`"" -ForegroundColor White
    Write-Host ""
    Write-Host "  3. Create build directory:" -ForegroundColor Cyan
    Write-Host "       mkdir build" -ForegroundColor White
    Write-Host "       cd build" -ForegroundColor White
    Write-Host ""
    Write-Host "  4. Configure with CMake:" -ForegroundColor Cyan
    Write-Host "       cmake -DCMAKE_TOOLCHAIN_FILE=`"$VcpkgPath\scripts\buildsystems\vcpkg.cmake`" .." -ForegroundColor White
    Write-Host ""
    Write-Host "  5. Build the project:" -ForegroundColor Cyan
    Write-Host "       cmake --build . --config Release" -ForegroundColor White
    Write-Host ""
    Write-Host "Alternative: Using CMake GUI" -ForegroundColor Yellow
    Write-Host "  1. Open CMake GUI" -ForegroundColor Cyan
    Write-Host "  2. Set source directory to: $ProjectRoot" -ForegroundColor White
    Write-Host "  3. Set build directory to: $ProjectRoot\build" -ForegroundColor White
    Write-Host "  4. Click 'Add Entry' and add:" -ForegroundColor Cyan
    Write-Host "       Name:  CMAKE_TOOLCHAIN_FILE" -ForegroundColor White
    Write-Host "       Type:  FILEPATH" -ForegroundColor White
    Write-Host "       Value: $VcpkgPath\scripts\buildsystems\vcpkg.cmake" -ForegroundColor White
    Write-Host "  5. Click 'Configure', then 'Generate', then 'Open Project'" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Alternative: Using Visual Studio 2019/2022" -ForegroundColor Yellow
    Write-Host "  1. Open Visual Studio" -ForegroundColor Cyan
    Write-Host "  2. File -> Open -> Folder" -ForegroundColor Cyan
    Write-Host "  3. Select: $ProjectRoot" -ForegroundColor White
    Write-Host "  4. Visual Studio will detect CMakeLists.txt automatically" -ForegroundColor Cyan
    Write-Host "  5. Add CMAKE_TOOLCHAIN_FILE to CMakeSettings.json" -ForegroundColor Cyan
    Write-Host ""
}

# Main script
function Main {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host "R-Type Dependency Bootstrap Script for Windows" -ForegroundColor Cyan
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host ""
    
    # Check prerequisites
    Write-Info "Checking prerequisites..."
    
    if (-not (Test-CMakeInstalled)) {
        Write-Warning "CMake is not installed or not in PATH"
        Write-Host "  Download from: https://cmake.org/download/" -ForegroundColor Yellow
    }
    else {
        $cmakeVersion = & cmake --version | Select-Object -First 1
        Write-Success "CMake found: $cmakeVersion"
    }
    
    if (-not (Test-VisualStudioInstalled)) {
        Write-Warning "Visual Studio does not appear to be installed"
        Write-Host "  Download from: https://visualstudio.microsoft.com/" -ForegroundColor Yellow
        Write-Host "  Make sure to install 'Desktop development with C++' workload" -ForegroundColor Yellow
    }
    else {
        Write-Success "Visual Studio found"
    }
    
    # Check for Git
    try {
        $null = Get-Command git -ErrorAction Stop
        Write-Success "Git found"
    }
    catch {
        Write-Error "Git is not installed or not in PATH"
        Write-Host "  Download from: https://git-scm.com/download/win" -ForegroundColor Red
        return
    }
    
    if ($SkipInstall) {
        Write-Info "Skipping installation (SkipInstall flag set)"
        if ($VcpkgPath -ne "") {
            Show-BuildInstructions -VcpkgPath $VcpkgPath
        }
        else {
            Write-Warning "Please specify -VcpkgPath to show build instructions"
        }
        return
    }
    
    # Determine vcpkg path
    if ($VcpkgPath -eq "") {
        # Default to project root
        $VcpkgPath = Join-Path $ProjectRoot "vcpkg"
        Write-Info "No vcpkg path specified, using default: $VcpkgPath"
    }
    
    # Install or use existing vcpkg
    if (-not (Test-Path (Join-Path $VcpkgPath "vcpkg.exe"))) {
        $VcpkgPath = Install-Vcpkg -InstallPath $VcpkgPath
        if ($null -eq $VcpkgPath) {
            Write-Error "Failed to install vcpkg. Exiting."
            return
        }
    }
    else {
        Write-Success "Using existing vcpkg at: $VcpkgPath"
    }
    
    # Install dependencies
    $vcpkgExe = Join-Path $VcpkgPath "vcpkg.exe"
    Install-Dependencies -VcpkgExe $vcpkgExe
    
    # Show build instructions
    Show-BuildInstructions -VcpkgPath $VcpkgPath
}

# Run main function
Main
