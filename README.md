# Screen Rotator

A lightweight Windows system tray application for quickly changing the rotation of any connected monitor.

Right-click the tray icon, pick a monitor, pick a rotation — done.

## Features

- System tray app with minimal footprint
- Multi-monitor support — lists all connected displays
- 0°, 90°, 180°, 270° rotation per monitor
- Checkmarks on current rotation, grayed-out unsupported orientations
- Hotplug aware — detects newly connected monitors automatically
- Single `.exe`, no runtime dependencies

## Prerequisites

- Windows 7 or later
- One of the following compilers:
  - **MSVC** — [Visual Studio 2019+](https://visualstudio.microsoft.com/) with "Desktop development with C++" workload
  - **MinGW** — [MinGW-w64](https://www.mingw-w64.org/) or via [MSYS2](https://www.msys2.org/)
- [CMake 3.15+](https://cmake.org/download/)

## Setup

### 1. Add an icon file

Place an `.ico` file named `app.ico` in the project root. It should contain at least 16x16 and 32x32 variants.

A ready-made `app.ico` is included. To regenerate it (or tweak the design), run the bundled Python script:

```
pip install Pillow
python generate_icon.py
```

This creates a multi-resolution `.ico` (16, 32, 48, 256 px) with a circular rotation arrow on a blue rounded-rectangle background.

If you don't have one, you can skip the custom icon by removing `resource.rc` from `CMakeLists.txt`:

```cmake
add_executable(ScreenRotator WIN32
    main.cpp
    # resource.rc   <-- comment out or remove this line
)
```

The app will fall back to the default Windows application icon.

### 2. Build

#### Option A: MSVC (Visual Studio)

```
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

The executable will be at `build\Release\ScreenRotator.exe`.

#### Option B: MinGW

```
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

The executable will be at `build\ScreenRotator.exe`.

#### Option C: Manual (no CMake)

**MSVC Developer Command Prompt:**

```
rc resource.rc
cl /O2 /MT /DUNICODE /D_UNICODE main.cpp resource.res /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib gdi32.lib
```

**MinGW:**

```
windres resource.rc -o resource.o
g++ -O2 -DUNICODE -D_UNICODE -mwindows main.cpp resource.o -o ScreenRotator.exe -luser32 -lshell32 -lgdi32
```

### 3. Run

Double-click `ScreenRotator.exe`. A rotation icon appears in the system tray (notification area).

- **Right-click** the tray icon to see all connected monitors
- **Select a rotation** from the submenu — the screen rotates immediately
- **Exit** from the bottom of the menu

### 4. Start with Windows (optional)

To launch automatically on login:

1. Press `Win + R`, type `shell:startup`, press Enter
2. Create a shortcut to `ScreenRotator.exe` in the Startup folder

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Rotation option is grayed out | Your display driver doesn't support that orientation |
| "Failed to change rotation" error | Try updating your GPU drivers |
| Tray icon not visible | Click the `^` arrow in the taskbar to check the overflow area |
| App won't start | Another instance may be running — check the system tray |
