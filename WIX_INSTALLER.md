# WiX Installer Configuration for SeaBattle

## Prerequisites

1. **WiX Toolset**: Install WiX Toolset v3.x or v4.x from https://wixtoolset.org/
2. **CMake**: Version 3.16 or higher
3. **Qt6**: With required components (Widgets, Core)

## Building the Installer

### Step 1: Configure the project
```bash
cmake --preset x64-release
```

### Step 2: Build the project
```bash
cmake --build out/build/x64-release --config Release
```

### Step 3: Create the installer package
```bash
cd out/build/x64-release
cpack -G WIX -C Release
```

Or use the package preset:
```bash
cmake --build --preset default
cpack --preset default
```

The installer (.msi file) will be created in `out/package/default/` directory.

## Customization

### 1. Product GUID
The `CPACK_WIX_UPGRADE_GUID` in `CMakeLists.txt` should be unique for your product:
```cmake
set(CPACK_WIX_UPGRADE_GUID "12345678-1234-1234-1234-123456789ABC")
```
**Important**: Generate a new GUID using `uuidgen` or an online GUID generator and keep it consistent across versions.

### 2. Product Information
Update version and vendor information in `CMakeLists.txt`:
```cmake
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VENDOR "Your Company Name")
```

### 3. License Agreement
Edit `License.rtf` to customize your license terms.

### 4. Optional Resources

Create a `resources` directory with:
- **icon.ico** (48x48 or 256x256): Product icon
- **banner.bmp** (493x58 pixels): Top banner in installer
- **dialog.bmp** (493x312 pixels): Background image in installer

### 5. Shortcuts
The installer creates:
- Start Menu shortcut in "SeaBattle" folder
- Desktop shortcut

Modify `cmake/WixFragment.wxs` to customize shortcut behavior.

## Installer Features

- ✓ Desktop shortcut
- ✓ Start Menu entry
- ✓ Automatic uninstaller
- ✓ Upgrade support (keeps GUID same for updates)
- ✓ Qt dependencies bundled automatically
- ✓ Add/Remove Programs integration

## Troubleshooting

### WiX not found
Ensure WiX is installed and added to PATH. Check with:
```bash
candle.exe -?
```

### Missing Qt DLLs
The `qt_generate_deploy_app_script` automatically handles Qt dependencies.

### Custom Components
Edit `cmake/WixFragment.wxs` to add custom components, registry entries, or file associations.

## File Structure
```
SeaBattle/
├── CMakeLists.txt              # Main CMake with CPack/WiX config
├── CMakePresets.json           # Build presets
├── License.rtf                 # License agreement
├── cmake/
│   └── WixFragment.wxs        # WiX shortcuts configuration
├── resources/                  # Optional installer resources
│   ├── icon.ico
│   ├── banner.bmp
│   └── dialog.bmp
└── src/
    └── CMakeLists.txt         # Application target
```
