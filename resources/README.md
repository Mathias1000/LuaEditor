# Resources Directory

This directory contains resource files for the LuaAutoCompleteQt6 application:

## Files:

- **luaautocomplete.desktop.in**: Desktop entry template file used by CMake to generate the final .desktop file
- **lua-icon.png**: Application icon (48x48 pixels recommended)
- **README.md**: This file

## Desktop File Variables:

The .desktop.in file uses CMake variables that get replaced during build:
- `@CMAKE_INSTALL_PREFIX@`: Installation prefix path
- `@PROJECT_NAME@`: Project name (LuaAutoCompleteQt6)

## Icon Replacement:

To replace the icon with your own:
1. Create or find a 48x48 pixel PNG image
2. Replace lua-icon.png with your image
3. Rebuild the project

The icon will be installed to `share/pixmaps/` and referenced by the desktop file.
