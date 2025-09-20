# LuaAutoCompleteQt6

A modern Qt6-based Lua editor with intelligent autocompletion, syntax highlighting, symbol parsing, and module import system.

## Features

- **Modern C++23 Implementation**: Uses latest C++ features including smart pointers, RAII, string_view, and constexpr
- **Qt6 Interface**: Clean, responsive user interface built with Qt6 Widgets
- **Intelligent Autocompletion**: Context-aware code completion with support for:
  - Lua keywords and built-in functions
  - User-defined symbols (functions, variables, tables)
  - Member access completion (`object.member`, `object:method`)
  - Imported module functions via `require()`
- **Module Import System**: Automatically parses `require()` statements and provides completion for external Lua files
- **Syntax Highlighting**: Full Lua syntax highlighting with customizable color schemes
- **Symbol Navigation**: Real-time symbol parsing with F12/Ctrl+F12 navigation
- **Line Numbers**: Integrated line number display with current line highlighting
- **Auto-Indentation**: Smart indentation for Lua code blocks

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

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install
make -j$(nproc)
make install
```

## Running

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

- **Autocompletion**: Automatically triggered while typing, or manually with Ctrl+Space
- **Symbol Navigation**: 
  - **F12**: Find next reference of symbol under cursor
  - **Ctrl+F12**: Go to definition of symbol under cursor
- **Syntax Highlighting**: Automatic color coding for Lua syntax
- **Line Numbers**: Visible line numbers with current line highlighting
- **Auto-Indentation**: Automatic indentation for `function`, `if`, `for`, `while` blocks

### Advanced Autocompletion Features

#### Context-Aware Completion

The editor provides intelligent autocompletion based on context:

**Global Scope:**
```lua
-- Type: pr
-- Suggests: print, pairs
local mon
-- Suggests: monster (if defined), math, string, table, etc.
```

**Member Access:**
```lua
local monster = {}
monster.health = 100
monster.name = "Goblin"

-- Type: monster.
-- Suggests: health, name
```

**Method Calls:**
```lua
local player = {}
function player:attack() end
function player:defend() end

-- Type: player:
-- Suggests: attack, defend, new, init, update, etc.
```

**Standard Libraries:**
```lua
-- Type: string.
-- Suggests: find, gsub, len, sub, upper, lower, format, etc.

-- Type: math.
-- Suggests: abs, ceil, floor, max, min, random, sqrt, sin, cos, etc.
```

#### Module Import System

The editor automatically detects and parses `require()` statements:

**Setup Module Files:**

`utils.lua`:
```lua
function calculateDistance(x1, y1, x2, y2)
    return math.sqrt((x2-x1)^2 + (y2-y1)^2)
end

function clamp(value, min, max)
    return math.max(min, math.min(max, value))
end

local M = {}
M.helper = function() return "help" end
return M
```

**Use in Main Script:**
```lua
local utils = require("utils")
local helpers = require("game/helpers")

-- Type: utils.
-- Suggests: calculateDistance, clamp, helper

utils.calculateDistance(0, 0, 10, 10)  -- Full completion support
```

**Module Search Paths:**
The editor searches for modules in:
- `.` (current directory)
- `./modules`
- `./lib`
- `./scripts`

**Supported Module Patterns:**
- `function functionName()`
- `local function functionName()`
- `M.methodName = function()`
- `return { exported = exported }`

### Symbol Navigation

- **Real-time Parsing**: Symbols are updated as you type
- **Symbol Panel**: Right sidebar shows all functions, variables, and tables
- **Cross-Reference**: Click on symbols to navigate to definition
- **Multi-file Support**: Imported modules are included in symbol parsing

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
│   ├── LuaEditor.*        # Lua text editor with completion
│   ├── LuaParser.*        # Lua code parser and symbol analyzer
│   ├── AutoCompleter.*    # Intelligent autocompletion engine
│   └── LuaHighlighter.*   # Syntax highlighting
├── modules/               # Place your Lua modules here
├── examples/              # Example Lua scripts
├── build/                 # Build output (generated)
└── install/               # Installation directory (generated)
```

## Development

### Architecture

**Core Components:**

1. **MainWindow**: Main application interface and file management
2. **LuaEditor**: Advanced text editor with:
   - Line numbers and syntax highlighting
   - Context-aware autocompletion
   - Symbol navigation (F12/Ctrl+F12)
   - Auto-indentation
3. **LuaParser**: Lua code analysis and symbol extraction
4. **AutoCompleter**: Qt-based completion popup management
5. **LuaHighlighter**: Syntax highlighting for Lua language

**Key Features:**

- **Member Detection**: Automatically finds `object.member` and `object:method` patterns
- **Import Resolution**: Parses `require()` statements and loads external modules
- **Context Switching**: Different completion modes for global vs. member access
- **Real-time Updates**: All parsing happens as you type

### Adding Features

**New Completion Sources:**
```cpp
// In LuaEditor::buildCompletionItems()
if (parent == "myCustomLib") {
    QStringList customMethods = {"method1", "method2"};
    for (const QString& method : customMethods) {
        members.insert(method);
    }
}
```

**Additional Module Patterns:**
```cpp
// In LuaEditor::parseModuleFunctions()
QRegularExpression customPattern(R"(your_pattern_here)");
// Add pattern matching logic
```

**New Navigation Features:**
```cpp
// In LuaEditor, add new shortcuts and connect to custom methods
auto* customShortcut = new QShortcut(QKeySequence(Qt::Key_F3), this);
connect(customShortcut, &QShortcut::activated, this, &LuaEditor::yourCustomMethod);
```

## Testing

Create test files to verify autocompletion:

**test_completion.lua:**
```lua
local utils = require("utils")  -- Put utils.lua in same directory

local player = {}
player.health = 100
player.mana = 50

function player:attack()
    return "attacking"
end

-- Test completions:
-- Type: player.     → should suggest: health, mana
-- Type: player:     → should suggest: attack, new, init, etc.
-- Type: utils.      → should suggest functions from utils.lua
-- Type: string.     → should suggest: find, gsub, len, etc.
```

Run the test suite:
```bash
cd build
ctest --output-on-failure
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Follow existing code style (C++23, Qt6 patterns)
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Troubleshooting

### Common Issues

**Autocompletion not working:**
- Check debug output in terminal for parsing errors
- Verify module files exist in search paths
- Ensure proper `require()` syntax

**Module imports not found:**
- Check file paths relative to current directory
- Verify module files have `.lua` extension or no extension
- Check module search paths: `.`, `./modules`, `./lib`, `./scripts`

**Qt6 not found:**
- Ensure Qt6 development packages are installed
- Check that `qmake6` is in your PATH

**Build fails:**
- Ensure C++23 compatible compiler (GCC 12+ or Clang 15+)
- Try clean build: `./build.sh clean && ./build.sh`

**Application won't start:**
- Check dependencies: `ldd ./install/bin/LuaAutoCompleteQt6`
- Ensure Qt6 runtime libraries are installed

### Debug Mode

For troubleshooting completion issues, the editor provides debug output:

```bash
./build.sh debug
./install/bin/LuaAutoCompleteQt6
# Watch terminal for completion debug messages
```

## Acknowledgments

Based on modern C++23 and Qt6 architecture, designed for professional Lua development with full IDE-like features.