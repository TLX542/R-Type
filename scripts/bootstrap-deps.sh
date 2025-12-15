#!/bin/bash
# ============================================================================
# R-Type Dependency Bootstrap Script for Linux/macOS
# ============================================================================
# This script helps install dependencies for building R-Type.
# It supports multiple package managers: apt, pacman, dnf, brew
#
# Usage:
#   ./scripts/bootstrap-deps.sh [--vcpkg]
#
# Options:
#   --vcpkg    Use vcpkg instead of system package manager
# ============================================================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored messages
info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect package manager
detect_package_manager() {
    if command -v apt-get &> /dev/null; then
        echo "apt"
    elif command -v pacman &> /dev/null; then
        echo "pacman"
    elif command -v dnf &> /dev/null; then
        echo "dnf"
    elif command -v brew &> /dev/null; then
        echo "brew"
    else
        echo "unknown"
    fi
}

# Install dependencies using system package manager
install_system_deps() {
    local pkg_manager="$1"
    
    info "Installing dependencies using $pkg_manager..."
    
    case "$pkg_manager" in
        apt)
            info "Updating package lists..."
            sudo apt-get update
            
            info "Installing build tools..."
            sudo apt-get install -y build-essential cmake g++ make
            
            info "Installing ASIO (standalone)..."
            sudo apt-get install -y libasio-dev
            
            info "Installing raylib..."
            # Try to install raylib if available (Ubuntu 22.04+)
            if apt-cache show libraylib-dev &> /dev/null; then
                sudo apt-get install -y libraylib-dev
            else
                warning "raylib not available in apt, will be downloaded by CMake"
            fi
            ;;
            
        pacman)
            info "Installing build tools..."
            sudo pacman -S --needed --noconfirm base-devel cmake gcc make
            
            info "Installing ASIO (standalone)..."
            sudo pacman -S --needed --noconfirm asio
            
            info "Installing raylib..."
            if pacman -Si raylib &> /dev/null; then
                sudo pacman -S --needed --noconfirm raylib
            else
                warning "raylib not available in pacman, will be downloaded by CMake"
            fi
            ;;
            
        dnf)
            info "Installing build tools..."
            sudo dnf install -y gcc-c++ cmake make
            
            info "Installing ASIO (standalone)..."
            sudo dnf install -y asio-devel
            
            info "Installing raylib..."
            if dnf info raylib-devel &> /dev/null; then
                sudo dnf install -y raylib-devel
            else
                warning "raylib not available in dnf, will be downloaded by CMake"
            fi
            ;;
            
        brew)
            info "Updating Homebrew..."
            brew update
            
            info "Installing build tools..."
            brew install cmake
            
            info "Installing ASIO (standalone)..."
            brew install asio
            
            info "Installing raylib..."
            brew install raylib
            ;;
            
        *)
            error "Unknown package manager. Please install dependencies manually."
            echo ""
            echo "Required dependencies:"
            echo "  - CMake >= 3.16"
            echo "  - C++17 compiler (g++, clang++)"
            echo "  - ASIO (standalone)"
            echo "  - raylib (optional, for graphical client)"
            exit 1
            ;;
    esac
    
    success "System dependencies installed!"
}

# Install dependencies using vcpkg
install_vcpkg_deps() {
    info "Setting up vcpkg..."
    
    if [ ! -d "$PROJECT_ROOT/vcpkg" ]; then
        info "Cloning vcpkg..."
        git clone https://github.com/Microsoft/vcpkg.git "$PROJECT_ROOT/vcpkg"
        
        info "Bootstrapping vcpkg..."
        "$PROJECT_ROOT/vcpkg/bootstrap-vcpkg.sh"
    else
        info "vcpkg already exists at $PROJECT_ROOT/vcpkg"
    fi
    
    info "Installing ASIO via vcpkg..."
    "$PROJECT_ROOT/vcpkg/vcpkg" install asio
    
    info "Installing raylib via vcpkg..."
    "$PROJECT_ROOT/vcpkg/vcpkg" install raylib
    
    success "vcpkg dependencies installed!"
    echo ""
    info "To use vcpkg with CMake, run:"
    echo "  cmake -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/vcpkg/scripts/buildsystems/vcpkg.cmake .."
}

# Main script
main() {
    echo ""
    echo "============================================================"
    echo "R-Type Dependency Bootstrap Script"
    echo "============================================================"
    echo ""
    
    USE_VCPKG=0
    
    # Parse arguments
    for arg in "$@"; do
        case "$arg" in
            --vcpkg)
                USE_VCPKG=1
                ;;
            --help|-h)
                echo "Usage: $0 [--vcpkg]"
                echo ""
                echo "Options:"
                echo "  --vcpkg    Use vcpkg to install dependencies"
                echo "  --help     Show this help message"
                exit 0
                ;;
        esac
    done
    
    if [ "$USE_VCPKG" -eq 1 ]; then
        install_vcpkg_deps
    else
        PKG_MANAGER=$(detect_package_manager)
        info "Detected package manager: $PKG_MANAGER"
        
        if [ "$PKG_MANAGER" = "unknown" ]; then
            error "Could not detect package manager!"
            echo ""
            echo "You can try using vcpkg instead:"
            echo "  $0 --vcpkg"
            exit 1
        fi
        
        install_system_deps "$PKG_MANAGER"
    fi
    
    echo ""
    success "Dependencies setup complete!"
    echo ""
    info "Next steps:"
    echo "  1. Configure the build:"
    echo "       mkdir build && cd build"
    if [ "$USE_VCPKG" -eq 1 ]; then
        echo "       cmake -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/vcpkg/scripts/buildsystems/vcpkg.cmake .."
    else
        echo "       cmake .."
    fi
    echo "  2. Build the project:"
    echo "       cmake --build . -j\$(nproc)"
    echo ""
}

main "$@"
