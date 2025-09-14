#!/bin/bash

# LuaAutoComplete Qt6 Build Script for Linux
# Usage: ./build.sh [debug|release] [clean]

set -e

PROJECT_NAME="LuaAutoCompleteQt6"
BUILD_TYPE=${1:-Release}
SOURCE_DIR=$(dirname $(readlink -f $0))
BUILD_DIR="${SOURCE_DIR}/build-${BUILD_TYPE,,}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for Qt6
    if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
        print_error "Qt6 development packages not found. Please install qt6-base-dev qt6-tools-dev"
        print_status "On Ubuntu/Debian: sudo apt install qt6-base-dev qt6-tools-dev libgl1-mesa-dev"
        print_status "On Fedora: sudo dnf install qt6-qtbase-devel qt6-qttools-devel mesa-libGL-devel"
        print_status "On Arch: sudo pacman -S qt6-base qt6-tools mesa"
        exit 1
    fi
    
    # Check for Lua
    if ! pkg-config --exists lua5.4; then
        if ! pkg-config --exists lua; then
            print_error "Lua development packages not found."
            print_status "On Ubuntu/Debian: sudo apt install liblua5.4-dev"
            print_status "On Fedora: sudo dnf install lua-devel"
            print_status "On Arch: sudo pacman -S lua"
            exit 1
        else
            print_warning "Using system default Lua version instead of 5.4"
        fi
    fi
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found. Please install cmake"
        exit 1
    fi
    
    print_status "All dependencies satisfied"
}

# Clean build directory
clean_build() {
    if [ -d "${BUILD_DIR}" ]; then
        print_status "Cleaning build directory: ${BUILD_DIR}"
        rm -rf "${BUILD_DIR}"
    fi
}

# Configure build
configure_build() {
    print_status "Configuring build (${BUILD_TYPE})..."
    
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    # Detect Qt6 installation
    QT6_DIR=""
    if [ -d "/usr/lib/x86_64-linux-gnu/cmake/Qt6" ]; then
        QT6_DIR="/usr/lib/x86_64-linux-gnu/cmake/Qt6"
    elif [ -d "/usr/lib64/cmake/Qt6" ]; then
        QT6_DIR="/usr/lib64/cmake/Qt6"
    elif [ -d "/usr/local/Qt6/lib/cmake/Qt6" ]; then
        QT6_DIR="/usr/local/Qt6/lib/cmake/Qt6"
    fi
    
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${SOURCE_DIR}/install"
    
    if [ ! -z "${QT6_DIR}" ]; then
        CMAKE_ARGS="${CMAKE_ARGS} -DQt6_DIR=${QT6_DIR}"
    fi
    
    cmake "${SOURCE_DIR}" ${CMAKE_ARGS}
}

# Build project
build_project() {
    print_status "Building project..."
    
    cd "${BUILD_DIR}"
    
    # Determine number of parallel jobs
    JOBS=$(nproc 2>/dev/null || echo 4)
    print_status "Building with ${JOBS} parallel jobs"
    
    make -j${JOBS}
}

# Install project
install_project() {
    print_status "Installing project..."
    
    cd "${BUILD_DIR}"
    make install
    
    # Create desktop entry
    DESKTOP_FILE="${SOURCE_DIR}/install/share/applications/luaautocomplete.desktop"
    mkdir -p "$(dirname ${DESKTOP_FILE})"
    
    cat > "${DESKTOP_FILE}" << EOF
[Desktop Entry]
Name=Lua AutoComplete
Comment=Lua editor with intelligent autocompletion
Exec=${SOURCE_DIR}/install/bin/${PROJECT_NAME}
Icon=lua-icon
Terminal=false
Type=Application
Categories=Development;TextEditor;
MimeType=text/x-lua;
EOF
    
    print_status "Desktop entry created at ${DESKTOP_FILE}"
    print_status "Installation completed in: ${SOURCE_DIR}/install"
}

# Create run script
create_run_script() {
    RUN_SCRIPT="${SOURCE_DIR}/run.sh"
    
    cat > "${RUN_SCRIPT}" << EOF
#!/bin/bash
# Generated run script for ${PROJECT_NAME}

SCRIPT_DIR=\$(dirname \$(readlink -f \$0))
export LD_LIBRARY_PATH="\${SCRIPT_DIR}/install/lib:\$LD_LIBRARY_PATH"

"\${SCRIPT_DIR}/install/bin/${PROJECT_NAME}" "\$@"
EOF
    
    chmod +x "${RUN_SCRIPT}"
    print_status "Run script created: ${RUN_SCRIPT}"
}

# Main build process
main() {
    print_status "Starting build for ${PROJECT_NAME}"
    print_status "Build type: ${BUILD_TYPE}"
    print_status "Source directory: ${SOURCE_DIR}"
    print_status "Build directory: ${BUILD_DIR}"
    
    # Handle clean option
    if [ "$2" = "clean" ] || [ "$1" = "clean" ]; then
        clean_build
        if [ "$1" = "clean" ]; then
            print_status "Clean completed"
            exit 0
        fi
    fi
    
    check_dependencies
    configure_build
    build_project
    install_project
    create_run_script
    
    print_status "Build completed successfully!"
    print_status "Run the application with: ./run.sh"
    print_status "Or directly: ./install/bin/${PROJECT_NAME}"
}

# Run main function
main "$@"