# Linux/WSL Build Guide

## Qt5 Auto-Detection

This project now automatically detects Qt5 installations in `/usr/local/qt*` directories.

### Supported Qt Installation Patterns

The build system searches for Qt in the following locations:

1. **Version-specific directories**: `/usr/local/qt-5.15.17`, `/usr/local/qt5.15.17`, etc.
2. **Debug/Release builds**: `/usr/local/qt-5.15.17-debug`, `/usr/local/qt-5.15.17-release`
3. **Fallback locations**: `/usr/local/Qt`, `/usr/local/qt5`, `/usr/lib/qt5`, `/opt/qt5`

### Build Type Detection

- **Debug builds** prefer Qt debug installations (paths containing `-debug`)
- **Release builds** prefer Qt release installations (paths containing `-release`)
- If no specific build type is found, uses the first available Qt installation

### Installation Example

```bash
# Example Qt5 installation structure
/usr/local/qt5.15.17-debug/
  ├── bin/
  ├── lib/
  │   └── cmake/
  │       └── Qt5/         # CMake looks for this
  └── include/

/usr/local/qt5.15.17-release/
  ├── bin/
  ├── lib/
  │   └── cmake/
  │       └── Qt5/         # CMake looks for this
  └── include/
```

## Building

### Using build.sh (Recommended)

```bash
# Debug build with auto-detection
./build.sh debug

# Release build with auto-detection  
./build.sh release

# Clean debug build
./build.sh debug clean
```

The script will:
1. Auto-detect all Qt installations in `/usr/local/qt*`
2. Select the best match based on build type
3. Configure and build the project

### Manual CMake

```bash
mkdir build_linux && cd build_linux

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build  
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --parallel $(nproc)
```

## Qt Installation Tips

### From Qt Installer

If installing Qt with the official installer:

```bash
# Install to version-specific directory
sudo mkdir -p /usr/local/qt5.15.17-debug
sudo mkdir -p /usr/local/qt5.15.17-release

# Copy from Qt installer location
sudo cp -r ~/Qt/5.15.17/gcc_64/* /usr/local/qt5.15.17-release/
```

### From Package Manager

```bash
# Ubuntu/Debian
sudo apt-get install qt5-default qtbase5-dev qtdeclarative5-dev

# The build system will find these automatically in system paths
```

## Troubleshooting

### No Qt Found

```
[ERROR] No Qt installations found in /usr/local/qt-*
```

**Solution**: Install Qt5 to `/usr/local/qt-VERSION` or use system packages

### CMake Can't Find Qt5

```
[ERROR] No valid Qt5 installation found with CMake support
```

**Solution**: Ensure Qt installation includes development files and CMake modules:
- `/usr/local/qt*/lib/cmake/Qt5/` directory must exist
- Qt5 development packages installed

### Build Type Mismatch

**Issue**: Debug build using Release Qt libraries

**Solution**: Install debug Qt libraries or let the system use the available build type:
```bash
# Force release libraries for debug build (acceptable for development)
cmake -DCMAKE_BUILD_TYPE=Debug -DQt5_DIR=/usr/local/qt5.15.17-release/lib/cmake/Qt5 ..
```

## Current Detection Results

The build system will show detected Qt installations:

```
[INFO] Found Qt installation: /usr/local/qt5.15.17-release
[INFO] Found Qt installation: /usr/local/qt5.15.17-debug
[SUCCESS] Selected Qt5 Debug build: /usr/local/qt5.15.17-debug
```

This ensures you're always using the optimal Qt build for your configuration.