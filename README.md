# HookX - XInput Controller Hook and Game Injection System

## Overview

HookX is a Windows-based system that hooks into XInput controller functions to provide automated controller input simulation and game memory reading capabilities. The system consists of two main components:

1. **HookX.exe** - The injector that attaches the hook DLL to target processes
2. **hook_dll.dll** - The hook library that intercepts XInput calls and provides input simulation

## Features

- **Automatic Controller Simulation**: Generates circular stick movements and enhanced input patterns
- **Y Button Toggle**: Press Y button to toggle between different input modes
- **Memory Reading**: Reads game memory for camera and position data
- **Real-time Logging**: Comprehensive logging of all operations
- **Hot-swappable Configuration**: Update memory addresses without restarting
- **Automatic Address Configuration**: No manual input required after injection

## System Architecture

### HookX.exe (Injector)

The main executable that:
- Finds target processes (specifically "eldenring.exe")
- Injects the hook DLL into the target process
- Automatically creates configuration files for memory addresses
- Provides user feedback and error handling

### hook_dll.dll (Hook Library)

The injected DLL that:
- Hooks `XInputGetState` function calls
- Generates automated controller input
- Reads game memory for camera and position data
- Provides toggle functionality via Y button
- Logs all operations for debugging

## Installation & Setup

### Prerequisites

- Visual Studio with C++ tools
- Windows 10/11
- Administrator privileges for injection

### Build Process

1. Open the project in Visual Studio
2. Build in Release configuration:
   ```bash
   cmake --build build --config Release
   ```

### Required Directories

```bash
mkdir D:\temp
```

## Configuration

### Automatic Address Configuration

After injection, the DLL automatically reads the pointer value from `D:\temp\hookx_addresses.txt` without requiring user input.

#### Address File Format

Create or edit `D:\temp\hookx_addresses.txt` with the following format:

```
g_cameraForward=7FF123456780
```

Where `7FF123456780` is your actual memory address in hexadecimal (without 0x prefix).

#### What Happens Automatically

1. **Injection**: HookX.exe injects hook_dll.dll into the target process
2. **File Check**: DLL checks for address file every 500ms
3. **Address Loading**: When a valid address is found, it's automatically loaded
4. **Position Calculation**: `g_position` is automatically set as `g_cameraForward + 16` bytes
5. **Logging**: posZ value is logged every 1 second to `D:\temp\hookx_log.txt`
6. **Real-time Updates**: You can update the address file anytime, and the DLL will pick up changes

## Usage Workflow

### Simple Setup Process

1. **Run HookX.exe** - Injects DLL automatically
2. **Edit Address File** - Update `D:\temp\hookx_addresses.txt` with real address
3. **Automatic Operation** - DLL picks up the address and starts logging posZ values

### Example Usage

```bash
# 1. Run the injector
HookX.exe

# 2. Edit the address file (while hook is running)
echo g_cameraForward=7FF123456780 > D:\temp\hookx_addresses.txt

# 3. Check the logs
tail -f D:\temp\hookx_log.txt
```

### No User Interaction Required

- No manual address input needed
- No prompts during injection
- Just run HookX.exe and it handles everything automatically
- Edit the address file if you need to change the memory address

## Input Simulation Modes

### Default Mode (g_rotatingCamera = false)
- Generates circular movement patterns
- Uses predefined stick values with rotational enhancement
- Smooth, continuous circular motion

### Rotating Camera Mode (g_rotatingCamera = true)
- Activated by pressing Y button on controller
- Sends continuous upward left stick input (`leftY = 1.0`)
- Calculates camera rotation based on position relative to center point
- Adjusts right stick based on forward vector calculations

### Y Button Toggle

After the game is injected, when the user clicks the Y button on the controller:
- **First press**: Enables continuous `leftY` input (full upward movement)
- **Second press**: Stops the continuous input and returns to normal behavior
- Toggle state is logged for debugging

## Memory Layout

The system expects the following memory layout at `g_cameraForward`:

| Offset | Type  | Description |
|--------|-------|-------------|
| 0x00   | float | Forward X   |
| 0x04   | float | Forward Y   |
| 0x08   | float | Forward Z   |
| 0x0C   | float | (unused)    |
| 0x10   | float | Position X  |
| 0x14   | float | Position Y  |
| 0x18   | float | Position Z  |

### Memory Address Configuration

- Only `g_cameraForward` address is required
- `g_position` is automatically calculated as `g_cameraForward + 16` bytes
- Addresses should be in hexadecimal format without "0x" prefix
- File is monitored every 500ms for real-time updates

## Logging System

All operations are logged to: `D:\temp\hookx_log.txt`

### Log Categories

- **[HOOK]**: DLL attachment/detachment events
- **[ADDRESS CHECK]**: File monitoring and address validation
- **[ADDRESS UPDATE]**: Memory address changes
- **[CAMERA LOG]**: Camera forward vector values
- **[POSITION LOG]**: Position coordinate values
- **[POSZ LOG]**: Z-coordinate position values (logged every 1 second)

### Log Examples

```
[HOOK] Hook DLL attached to process - HookX started
[ADDRESS UPDATE] g_cameraForward auto-loaded from file: 0x7FF123456780
[ADDRESS UPDATE] g_position auto-set to: 0x7FF123456799 (g_cameraForward + 16)
[POSZ LOG] posZ value: -25.850000
[CAMERA LOG] Camera Forward Vector: X=0.750000, Y=0.000000, Z=-0.661000
[POSITION LOG] g_position: 0x7FF123456799 -> Position: X=100.5, Y=50.2, Z=-25.8
Y button pressed! Continuous leftY input: ENABLED
```

## Controller Input Mapping

| Input | Function |
|-------|----------|
| Y Button | Toggle between rotation modes |
| Left Stick | Automated circular movement or continuous upward |
| Right Stick | Camera rotation adjustment (when in rotating mode) |

## Technical Details

### Memory Safety

All memory access is protected with exception handling:

```cpp
__try {
    float* cameraPtr = static_cast<float*>(g_cameraForward);
    float value = *cameraPtr;  // Safe dereferencing
    // Use the value...
} __except (EXCEPTION_EXECUTE_HANDLER) {
    // Handle access violations gracefully
}
```

### Timing and Threading

- **Address Check Interval**: 500ms
- **Logging Interval**: 1000ms (1 second)
- **Frame Counter**: Increments with each XInput call
- **Safe Memory Access**: Uses `__try/__except` for crash prevention

### Process Injection

- Uses Microsoft Detours library for function hooking
- Targets `XInputGetState` function specifically
- Maintains original function pointer for fallback
- Clean attachment and detachment process

### Getting Float Values from Pointers

```cpp
// Basic dereferencing
float* ptr = static_cast<float*>(somePointer);
float value = *ptr;  // Gets the float value

// Array access for multiple floats
float value1 = ptr[0];  // First float
float value2 = ptr[1];  // Second float
float value3 = ptr[2];  // Third float

// Safe dereferencing (recommended)
__try {
    float value = *ptr;
} __except (EXCEPTION_EXECUTE_HANDLER) {
    // Handle access violation
}
```

## File Locations

- **Address file**: `D:\temp\hookx_addresses.txt`
- **Log file**: `D:\temp\hookx_log.txt`
- **Executable**: `build\Release\HookX.exe`
- **Hook DLL**: `build\Release\hook_dll.dll`

## Troubleshooting

### Common Issues

**DLL Not Injecting**:
- Run as Administrator
- Ensure target process is running
- Check Windows Defender/Antivirus settings

**No Memory Values Logged**:
- Verify address file exists: `D:\temp\hookx_addresses.txt`
- Check address format (hex without 0x prefix)
- Ensure address points to valid memory

**Controller Not Responding**:
- Verify XInput is being used by target application
- Check if other controller software is interfering
- Ensure DLL injection was successful

**Address File Issues**:
- File shows "g_cameraForward is nullptr" - update the address file with a real address
- "Cannot read value - Access Violation" - the address is invalid or the game has updated

### Debug Information

Check the log file for detailed information about:
- Address loading status
- Memory access attempts
- Controller input states
- Y button toggle events

## Benefits of This System

- **Zero User Interaction** - No prompts or manual input required
- **Real-time Updates** - Change addresses without restarting
- **Clear Feedback** - Comprehensive logging of what's happening
- **Error Handling** - Graceful handling of missing/invalid files
- **Hot-swappable** - Edit the address file while the hook is running
- **Safe Memory Access** - Protected memory operations prevent crashes

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Disclaimer

This tool is for educational and research purposes. Users are responsible for ensuring they comply with the terms of service of any software they use this with.