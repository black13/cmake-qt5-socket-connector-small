# NodeGraph Build Instructions

## 🚀 **Windows Build (Recommended)**

### Prerequisites
1. **Visual Studio 2022** with C++ development tools
2. **Qt 5.15.x** installed (any patch version)
3. **CMake** 3.16+ in PATH
4. **Git** for version control

### Quick Build
```cmd
# 1. Open "Developer Command Prompt for VS 2022" (required!)
# 2. Navigate to project directory
cd C:\your\path\to\cmake-qt5-socket-connector-small

# 3. Build debug version (most common)
build.bat debug

# 4. Run the application
build_Debug\NodeGraph.exe
```

### Other Build Options
```cmd
build.bat release          # Optimized release build
build.bat both             # Build both debug and release
build.bat debug clean      # Full clean debug build
build.bat release clean    # Full clean release build  
```

---

## 🔧 **Adapting to Different Computers**

The `build.bat` file contains hardcoded Qt paths that need updating for different machines:

### Current Configuration (This Computer)
```batch
C:\Qt\5.15.2-debug     # Debug Qt installation
C:\Qt\5.15.2-release   # Release Qt installation
```

### To Adapt for Your Computer:

**Option 1: Edit build.bat (Permanent)**
1. Open `build.bat` in any text editor
2. Find these 4 lines and update Qt paths:
   - **Line 29**: `set QT5_PATH=C:\Qt\5.15.2-release`  → Update to your release Qt path
   - **Line 33**: `set QT5_PATH=C:\Qt\5.15.2-debug`    → Update to your debug Qt path  
   - **Line 153**: `set QT5_PATH=C:\Qt\5.15.2-debug`   → Update to your debug Qt path
   - **Line 172**: `set QT5_PATH=C:\Qt\5.15.2-release` → Update to your release Qt path

**Option 2: Environment Variable (Temporary)**
```cmd
# Set before running build.bat
set CMAKE_PREFIX_PATH=C:\Your\Qt\Path\lib\cmake
build.bat debug
```

### Common Qt Installation Paths
- **Qt Online Installer**: `C:\Qt\5.15.x\msvc2019_64`
- **Custom Install**: `C:\Qt\5.15.x-debug` and `C:\Qt\5.15.x-release`  
- **Enterprise**: `E:\Qt\5.15.x\Debug_x64` and `E:\Qt\5.15.x\Release_x64`

---

## 🐧 **Linux Build (Alternative)**

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential cmake qt5-default libxml2-dev

# Build
./build.sh debug clean

# Run
build_linux/NodeGraph
```

---

## ✅ **Testing the Build**

### Available Test Files
```cmd
# Test with sample XML files
build_Debug\NodeGraph.exe autosave.xml
build_Debug\NodeGraph.exe simple_nodes_test.xml

# Start empty (no arguments)
build_Debug\NodeGraph.exe
```

### Visual Studio Debugging
- Open `build_Debug\NodeGraph.sln` in Visual Studio
- Press **F5** to debug (XML test file will load automatically)
- Breakpoints and debugging work as expected

---

## 🔍 **Current Issues Being Fixed**

**Socket Count XML Bug**: All nodes currently serialize as `inputs="1" outputs="1"` regardless of type. Expected behavior:
- `SOURCE`: `inputs="0" outputs="1"`
- `SINK`: `inputs="1" outputs="0"`  
- `SPLIT`: `inputs="1" outputs="2"`
- `MERGE`: `inputs="2" outputs="1"`

**Branch**: `fix/xml-factory-authority` - Making GraphFactory the single XML authority

---

## 📞 **Troubleshooting**

### CMake Configuration Failed
```
By not providing "FindQt5.cmake" in CMAKE_MODULE_PATH...
```
**Solution**: Update Qt paths in `build.bat` to match your Qt installation.

### Build Errors
```
*** Debug Build FAILED ***
```
**Solution**: 
1. Ensure "Developer Command Prompt for VS 2022" is used
2. Check Qt installation paths
3. Try clean build: `build.bat debug clean`

### Missing libxml2
**Linux**: `sudo apt-get install libxml2-dev`
**Windows**: Automatically downloaded via CMake FetchContent

---

## 📁 **Build Output Structure**

```
project/
├── build_Debug/          # Debug build output
│   ├── NodeGraph.exe     # Debug executable
│   └── NodeGraph.sln     # Visual Studio solution
├── build_Release/        # Release build output  
│   ├── NodeGraph.exe     # Release executable
│   └── NodeGraph.sln     # Visual Studio solution
└── build_linux/          # Linux build output
    └── NodeGraph         # Linux executable
```

**Happy Building! 🎉**