# HookX - Simplified Automatic Address Configuration

## How It Works

After injection, the DLL automatically reads the pointer value from `D:\temp\hookx_addresses.txt` without requiring user input.

## Setup Process

1. **Run HookX.exe** - It will inject the DLL and check for existing address file
2. **Address File** - The system automatically looks for `D:\temp\hookx_addresses.txt`
3. **Automatic Loading** - The DLL reads the address every 500ms and updates automatically

## Address File Format

Create or edit `D:\temp\hookx_addresses.txt` with the following format:

```
g_cameraForward=7FF123456780
```

Where `7FF123456780` is your actual memory address in hexadecimal (without 0x prefix).

## What Happens Automatically

1. **Injection**: HookX.exe injects hook_dll.dll into the target process
2. **File Check**: DLL checks for address file every 500ms
3. **Address Loading**: When a valid address is found, it's automatically loaded
4. **Logging**: posZ value is logged every 1 second to `D:\temp\hookx_log.txt`
5. **Real-time Updates**: You can update the address file anytime, and the DLL will pick up changes

## Log Output Examples

```
[ADDRESS UPDATE] g_cameraForward auto-loaded from file: 0x7FF123456780
[POSZ LOG] posZ value: -25.850000
[POSZ LOG] posZ value: -26.120000
```

## No User Interaction Required

- No manual address input needed
- No prompts during injection
- Just run HookX.exe and it handles everything automatically
- Edit the address file if you need to change the memory address

## File Locations

- Address file: `D:\temp\hookx_addresses.txt`
- Log file: `D:\temp\hookx_log.txt`
- Sample file included: `sample_addresses.txt`
