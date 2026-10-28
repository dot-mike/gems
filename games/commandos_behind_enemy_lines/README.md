# Instructions

Fixes windowed mode in the Steam release of Commandos: Behind Enemy Lines (`Comandos.exe`).

1. Copy the released `ddraw.dll` into `G:\SteamLibrary\steamapps\common\Commandos Behind Enemy Lines`, next to `Comandos.exe`
2. Set the Windows desktop to 32-bit colour (the game refuses windowed mode otherwise)
3. Start the game and press Ctrl+Enter to switch between fullscreen and windowed

The log is in `%TEMP%/log.txt`. It says `present patched` when the fix is active.

The fix only works with the Steam `Comandos.exe` (SHA-256 `8f21a8e7...2649ce`). With any other exe it logs `present patch skipped` and changes nothing.
