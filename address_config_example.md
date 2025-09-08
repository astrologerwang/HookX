# HookX Address Configuration Test

This demonstrates the simplified address configuration where only g_cameraForward needs to be provided.

## Example usage:

1. Run HookX.exe
2. When prompted, enter: 0x7FF123456789
3. The system will automatically set:
   - g_cameraForward = 0x7FF123456789
   - g_position = 0x7FF123456799 (g_cameraForward + 16)

## Expected log output every 1 second:

```
[ADDRESS UPDATE] g_cameraForward set to: 0x7FF123456789
[ADDRESS UPDATE] g_position auto-set to: 0x7FF123456799 (g_cameraForward + 16)
[CAMERA LOG] Camera Forward Vector: X=0.750000, Y=0.000000, Z=-0.661000
[POSITION LOG] g_position: 0x7FF123456799 -> Position: X=100.5, Y=50.2, Z=-25.8
```

## Communication file format:

D:\temp\hookx_addresses.txt will contain:
```
g_cameraForward=7FF123456789
```

The DLL automatically calculates g_position from this single value.
