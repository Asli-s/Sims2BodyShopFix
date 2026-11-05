# Sims 2 Body Shop crash fix

Fixes Body Shop (The Sims 2 Ultimate Collection) crashing at start on modern PCs.

[![Download BodyShopFix.exe](https://img.shields.io/badge/Download-BodyShopFix.exe-2ea44f?style=for-the-badge&logo=windows)](https://github.com/Asli-s/Sims2BodyShopFix/releases/latest/download/BodyShopFix.exe)

## How to use

1. Close Body Shop.
2. Download `BodyShopFix.exe` with the button above.
3. Double-click it and say **Yes** when Windows asks for permission.
4. Click **Yes** to apply the fix. Done!

To undo it, run the patcher again and click **Yes** when it asks to revert.
A backup of the original is also saved next to Body Shop as `TS2BodyShop.exe.backup`.

If the patcher can't find Body Shop, it asks you to pick `TS2BodyShop.exe` yourself.

The patcher isn't signed, so Windows may say "Windows protected your PC".
Click **More info → Run anyway**.

## What it works with

Only the **Ultimate Collection** Body Shop (version 1.17.0.66) for now. The patcher checks the
file and changes nothing if it's a different version.

## Why it crashes

When Body Shop starts, it asks the graphics card how much texture memory it has and then
fills 110% of that with test textures. It was made for the cards of 2006, which only had a
few hundred MB.
Modern cards have huge amounts of memory and report about 4 GB, so Body Shop keeps making
textures until its own memory runs out, and you get the famous "Application has crashed" error.
That's also why the 4 GB patch doesn't help.

The fix makes Body Shop use 256 MB for this test, like a card from its time. It changes
6 bytes in `TS2BodyShop.exe`:

| File offset | Original | Fixed |
|---|---|---|
| `0x1FA7CF` | `8B 06 56 FF 50 10` | `B8 00 00 00 10 90` |

Tested on Windows 11 with an NVIDIA card. If it works (or doesn't) on your PC, please let me
know in [Issues](../../issues).

## Building it yourself

Open a command prompt in this folder and run `build.bat` (needs Visual Studio 2022 with C++).
The patcher is one small file: [`src/bodyshop_fix.cpp`](src/bodyshop_fix.cpp).

## License

The patcher is [MIT licensed](LICENSE). The Sims 2 and Body Shop belong to Electronic Arts.
This is a fan fix and is not made or supported by EA.
