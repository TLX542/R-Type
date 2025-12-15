# Windows Build Guide - CMake with Automatic Dependencies

This guide provides detailed instructions for building R-Type on Windows using the new CMake build system with automatic dependency management.

## Prerequisites Checklist

Before starting, ensure you have:
- [ ] **Windows 7 or later** (Windows 10/11 recommended)
- [ ] **CMake 3.16 or later** - [Download here](https://cmake.org/download/)
- [ ] **Git for Windows** - [Download here](https://git-scm.com/download/win)
- [ ] **Visual Studio 2019 or 2022** with "Desktop development with C++" workload - [Download here](https://visualstudio.microsoft.com/downloads/)
  - OR **MinGW-w64** - [Download here](https://www.mingw-w64.org/downloads/)

## Method 1: Automatic Dependencies (Recommended - No vcpkg needed)

This is the simplest method that requires no manual dependency installation:

### Step 1: Clone the Repository
```powershell
git clone https://github.com/TLX542/R-Type.git
cd R-Type
```

### Step 2: Configure with CMake
```powershell
mkdir build
cd build
cmake ..
```

CMake will automatically:
- Download ASIO 1.28.0 from GitHub
- Download raylib 5.0 from GitHub
- Configure the build for your compiler

### Step 3: Build
```powershell
# For Visual Studio
cmake --build . --config Release

# Or for Debug
cmake --build . --config Debug
```

### Step 4: Find Your Executables
After building, executables will be in:
- Release build: `build\Release\`
- Debug build: `build\Debug\`

Files created:
- `r-type_server.exe` - Game server
- `render_client.exe` - Graphical client (raylib)
- `game_client.exe` - Console client
- `client_test.exe` - Simple test client
- `test_headless.exe` - Automated test client

## Method 2: Using vcpkg (Alternative)

### Step 1: Run Bootstrap Script
```powershell
.\scripts\bootstrap-deps.ps1
```

This will:
- Install vcpkg to `.\vcpkg\`
- Install ASIO and raylib via vcpkg
- Show you the next steps

### Step 2: Configure with vcpkg Toolchain
```powershell
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake ..
```

### Step 3: Build
```powershell
cmake --build . --config Release
```

## Method 3: Using CMake GUI

### Step 1: Open CMake GUI
1. Launch CMake (cmake-gui)
2. Set "Where is the source code" to: `C:\path\to\R-Type`
3. Set "Where to build the binaries" to: `C:\path\to\R-Type\build`

### Step 2: Configure
1. Click **Configure**
2. Select generator:
   - "Visual Studio 17 2022" (for VS 2022)
   - "Visual Studio 16 2019" (for VS 2019)
   - "MinGW Makefiles" (for MinGW)
3. Click **Finish**

CMake will automatically download dependencies.

### Step 3: Optional - Use vcpkg
If you want to use vcpkg instead:
1. Click **Add Entry**
2. Set:
   - Name: `CMAKE_TOOLCHAIN_FILE`
   - Type: `FILEPATH`
   - Value: `C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake`
3. Click **Configure** again

### Step 4: Generate and Build
1. Click **Generate**
2. Click **Open Project** (opens Visual Studio)
3. In Visual Studio: Build > Build Solution (F7)

## Method 4: Using Visual Studio Directly

Visual Studio 2019/2022 has native CMake support:

### Step 1: Open Folder
1. Launch Visual Studio
2. File → Open → Folder
3. Select the R-Type project directory

### Step 2: Configure CMake (if using vcpkg)
1. Visual Studio will detect `CMakeLists.txt`
2. If you want to use vcpkg, edit `CMakeSettings.json`:
   ```json
   {
     "configurations": [
       {
         "name": "x64-Release",
         "generator": "Ninja",
         "configurationType": "Release",
         "buildRoot": "${projectDir}\\build\\${name}",
         "installRoot": "${projectDir}\\install\\${name}",
         "cmakeCommandArgs": "-DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake",
         "buildCommandArgs": "",
         "ctestCommandArgs": ""
       }
     ]
   }
   ```

### Step 3: Build
- Select configuration (Debug or Release) from dropdown
- Build → Build All (Ctrl+Shift+B)

## Build Options

You can customize the build with these CMake options:

```powershell
# Force using system-installed dependencies only (will fail if not found)
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..

# Disable building tests
cmake -DBUILD_TESTS=OFF ..

# Enable vcpkg hints
cmake -DUSE_VCPKG=ON ..

# Specify build type (if not using Visual Studio generator)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Troubleshooting

### Error: "CMake not found"
**Solution**: Add CMake to your PATH or reinstall CMake with "Add to PATH" option.

### Error: "Git not found"
**Solution**: Install Git for Windows and ensure it's in your PATH.

### Error: "No compiler found"
**Solution**: 
- Install Visual Studio with "Desktop development with C++" workload
- Or install MinGW-w64 and add to PATH

### Error: "Failed to download ASIO/raylib"
**Solution**:
- Check your internet connection
- Check if you're behind a proxy/firewall
- Try using vcpkg method instead

### Error: "OpenGL not found" (for raylib)
**Solution**:
- Update your graphics drivers
- raylib should work with system OpenGL on Windows

### Build is slow
**Solution**: Use parallel builds:
```powershell
cmake --build . --config Release -j 8
# Or let CMake decide:
cmake --build . --config Release --parallel
```

### Cannot find executables after build
**Solution**: 
- Visual Studio builds put executables in `build\Debug\` or `build\Release\`
- MinGW builds put them directly in `build\`

## Running the Server and Client

### Step 1: Open Command Prompt or PowerShell
```powershell
cd build\Release  # or build\Debug
```

### Step 2: Start Server
```powershell
.\r-type_server.exe 4242 4243
```

You should see:
```
R-Type Server started
TCP port: 4242
UDP port: 4243
[GameServer] ECS initialized
R-Type Game Server is running...
```

### Step 3: Start Client (in another terminal)
```powershell
cd build\Release
.\render_client.exe localhost 4242
```

Enter your player name when prompted.

### Step 4: (Optional) Test Client
```powershell
.\test_headless.exe localhost 4242
```

## Firewall Configuration

If clients can't connect:

### Option 1: Allow through Windows Firewall GUI
1. Windows Security → Firewall & network protection
2. Advanced settings
3. Inbound Rules → New Rule
4. Port → TCP → 4242 → Allow
5. Repeat for UDP → 4243

### Option 2: PowerShell (Admin)
```powershell
New-NetFirewallRule -DisplayName "R-Type Server TCP" -Direction Inbound -LocalPort 4242 -Protocol TCP -Action Allow
New-NetFirewallRule -DisplayName "R-Type Server UDP" -Direction Inbound -LocalPort 4243 -Protocol UDP -Action Allow
```

## Clean Build

To start fresh:
```powershell
# Remove build directory
Remove-Item -Recurse -Force build

# Rebuild
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Development Tips

### Debug Build
For debugging with Visual Studio:
```powershell
cmake --build . --config Debug
```
Then open the solution in Visual Studio and press F5 to debug.

### Rebuild Single Target
```powershell
cmake --build . --config Release --target r-type_server
```

### See Full Build Commands
```powershell
cmake --build . --config Release --verbose
```

## Next Steps

After successful build:
1. Read [ARCHITECTURE.md](ARCHITECTURE.md) for code structure
2. Read [PROTOCOL.md](PROTOCOL.md) for network protocol
3. Try modifying the code and rebuilding
4. Run tests with `test_headless.exe`

## Getting Help

If you encounter issues not covered here:
1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
2. Look at CMake configuration output for specific errors
3. Check the [main BUILD.md](BUILD.md) for more details
4. Ensure all prerequisites are properly installed
