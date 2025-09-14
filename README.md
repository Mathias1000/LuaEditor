# LuaAutoCompleteQt6

A modern Qt6-based Lua editor with intelligent autocompletion, syntax highlighting, and symbol parsing.

## Features

- **Modern C++23 Implementation**: Uses latest C++ features including smart pointers, RAII, string_view, and constexpr
- **Qt6 Interface**: Clean, responsive user interface built with Qt6 Widgets
- **Intelligent Autocompletion**: Context-aware code completion for Lua keywords, built-in functions, and user-defined symbols
- **Syntax Highlighting**: Full Lua syntax highlighting with customizable color schemes
- **Symbol Navigation**: Real-time symbol parsing and navigation panel
- **Line Numbers**: Integrated line number display
- **Error Detection**: Real-time syntax error detection and reporting

## Requirements

### System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install qt6-base-dev qt6-tools-dev libgl1-mesa-dev
sudo apt install liblua5.4-dev
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake pkgconfig
sudo dnf install qt6-qtbase-devel qt6-qttools-devel mesa-libGL-devel
sudo dnf install lua-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake pkgconf
sudo pacman -S qt6-base qt6-tools mesa
sudo pacman -S lua
```

### Build Requirements

- **CMake**: Version 3.22 or higher
- **Qt6**: Widgets, Core, Gui modules
- **Lua**: Version 5.4 preferred (5.1+ supported)
- **C++ Compiler**: GCC 12+ or Clang 15+ with C++23 support

## Building

### Quick Build

The project includes a comprehensive build script that handles dependency checking and builds the project:

```bash
git clone <repository-url>
cd LuaAutoCompleteQt6
chmod +x build.sh
./build.sh
```

### Build Options

```bash
# Release build (default)
./build.sh release

# Debug build
./build.sh debug

# Clean build
./build.sh clean

# Clean and rebuild
./build.sh release clean
```

### Manual Build

If you prefer to build manually:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install
make -j$(nproc)
make install
```

## Running

After building, you can run the application using:

```bash
# Using the generated run script
./run.sh

# Or directly
./install/bin/LuaAutoCompleteQt6

# Open a specific file
./run.sh /path/to/your/script.lua
```

## Usage

### Basic Operations

- **New File**: Ctrl+N
- **Open File**: Ctrl+O
- **Save File**: Ctrl+S
- **Save As**: Ctrl+Shift+S

### Editor Features

- **Autocompletion**: Start typing and press Ctrl+E or continue typing to see suggestions
- **Symbol Navigation**: View all functions, variables, and tables in the right panel
- **Syntax Highlighting**: Automatic color coding for Lua syntax
- **Line Numbers**: Visible line numbers with current line highlighting
- **Auto-Indentation**: Automatic indentation for code blocks

### Autocompletion Features

The editor provides intelligent autocompletion for:

- **Lua Keywords**: `function`, `local`, `if`, `then`, `else`, etc.
- **Built-in Functions**: `print`, `type`, `pairs`, `ipairs`, etc.
- **Standard Libraries**: `string.*`, `table.*`, `math.*` functions
- **User-Defined Symbols**: Functions, variables, and tables defined in your code

## Project Structure

```
LuaAutoCompleteQt6/
├── CMakeLists.txt          # Main CMake configuration
├── build.sh               # Linux build script
├── README.md              # This file
├── LICENSE                # MIT License
├── src/                   # Source code
│   ├── main.cpp           # Application entry point
│   ├── MainWindow.*       # Main application window
│   ├── LuaEditor.*        # Lua text editor widget
│   ├── LuaParser.*        # Lua code parser and analyzer
│   ├── AutoCompleter.*    # Autocompletion engine
│   └── LuaHighlighter.*   # Syntax highlighting
├── resources/             # Application resources
├── tests/                 # Unit tests
├── docs/                  # Additional documentation
├── examples/              # Example Lua scripts
├── build/                 # Build output (generated)
└── install/               # Installation directory (generated)
```

## Development

### Architecture

The application is built with a modular architecture:

1. **MainWindow**: Coordinates the overall application interface
2. **LuaEditor**: Provides the text editing interface with line numbers
3. **LuaParser**: Parses Lua code and extracts symbols
4. **AutoCompleter**: Manages intelligent code completion
5. **LuaHighlighter**: Handles syntax highlighting

### Adding Features

The modular design makes it easy to extend the application:

- **New completion sources**: Extend `AutoCompleter::getContextualCompletions()`
- **Additional syntax**: Modify `LuaHighlighter::setupHighlightingRules()`
- **Parser enhancements**: Extend `LuaParser` symbol extraction methods

## Testing

Run the test suite:

```bash
cd build
ctest --output-on-failure
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes following the existing code style
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Troubleshooting

### Common Issues

**Qt6 not found:**
- Ensure Qt6 development packages are installed
- Check that `qmake6` or `qmake` is in your PATH

**Lua not found:**
- Install Lua development headers
- Verify with `pkg-config --exists lua5.4` or `pkg-config --exists lua`

**Build fails:**
- Ensure you have a C++23 compatible compiler (GCC 12+ or Clang 15+)
- Check that all dependencies are properly installed
- Try a clean build: `./build.sh clean && ./build.sh`

**Application won't start:**
- Check library dependencies: `ldd ./install/bin/LuaAutoCompleteQt6`
- Ensure Qt6 runtime libraries are installed
- Try running from the installation directory

### Getting Help

If you encounter issues:

1. Check the build output for specific error messages
2. Verify all dependencies are installed correctly
3. Try building in debug mode: `./build.sh debug`
4. Check the [docs/](docs/) directory for additional information

## Acknowledgments

Based on the original LuaAutoComplete project by cguebert, modernized for Qt6 and C++23.# LuaEditor
