# FiveInvader Agent Handover & Architectural Documentation

## Executive Summary

This document provides a comprehensive handover summary for the **FiveInvader** project (a GTA V Xbox 360 binary translation and static recompilation runtime derived from MarathonRecomp and SanRecomp).

Static code repairs, logger enum regressions, patch file renaming alignment, and legacy GTA IV hook stubbing have been completed. All legacy GTA IV function hooks and jump tables that previously caused LLD link failures have been audited and stubbed with clear markers.

---

## 1. Repository Layout & Branch

### Branch & Remote Setup
- **Branch**: `main`
- **Remote `origin`**: `https://github.com/bunboifr/FiveInvader.git` (Active Push Target / Personal Fork)
- **Remote `upstream`**: `https://github.com/triadwoozie/FiveInvader.git` (Upstream Primary Repository)
- **Author Identity**: Configured locally inside repository root (`triadwoozie <109454483+triadwoozie@users.noreply.github.com>`).

### Directory Structure
```
FiveInvader/
├── .github/                      # CI workflows (disabled during bringup in workflows_disabled)
├── CMakeLists.txt                # Root CMake project configuration
├── CMakePresets.json             # Cross-platform presets (Linux, Windows, macOS)
├── docs/                         # Project documentation and architecture specs
│   ├── AGENT_HANDOVER.md         # This handover document
│   ├── BUILDING.md               # Host build instructions
│   ├── INSTALLATION_ARCHITECTURE.md
│   └── ONLINE_MULTIPLAYER.md
├── FiveInvader/                  # Main runtime executable source
│   ├── api/                      # RAGE, Liberty, Havok, stdx interfaces
│   ├── apu/                      # Audio processing unit (XMA decoder, SDL2 driver)
│   ├── cpu/                      # Guest thread execution & PPC context definitions
│   ├── gpu/                      # Video renderer, shaders, ImGui snapshot
│   ├── hid/                      # Human interface device input drivers
│   ├── install/                  # Title update & XEX patching tools (RPF extractor with OpenSSL EVP)
│   ├── kernel/                   # Xbox 360 kernel simulation (memory, VFS, I/O, XAM)
│   │   └── io/                   # P2P networking, session trackers, sockets
│   ├── os/                       # OS abstraction layer (Linux, macOS, Win32)
│   └── patches/                  # Function hooks, patch tables, runtime overrides (GTA V aligned)
├── FiveInvaderLib/               # Recompilation library & configs
│   ├── CMakeLists.txt
│   ├── config/                   # XenonRecomp config files (GTA5.toml, etc.)
│   ├── ppc/                      # Output directory for recompiled PPC C++ code
│   └── private/                  # Target storage for original GTA V default.xex / xbox360.rpf
└── thirdparty/                   # Git submodules for external libraries
```

### Submodule Status
Submodules configured in `.gitmodules` (must be updated via `git submodule update --init --recursive` on the build machine):
- `tools/XenonRecomp`
- `tools/XenosRecomp`
- `tools/xenia`
- `tools/nebula/src`
- `thirdparty/GameNetworkingSockets`
- `thirdparty/SDL`, `thirdparty/SDL_mixer`
- `thirdparty/imgui`, `thirdparty/implot`
- `thirdparty/magic_enum`, `thirdparty/json`, `thirdparty/stb`, `thirdparty/concurrentqueue`, `thirdparty/unordered_dense`, `thirdparty/nativefiledialog-extended`, `thirdparty/ddspp`, `thirdparty/msdf-atlas-gen`, `thirdparty/ffmpeg-core`, `thirdparty/MoltenVK`

---

## 2. Code Fixes Applied

### A. Logger & Enum Regressions (`FiveInvader/os/logger.h`)
- **Root Cause**: Earlier global regex replacements stripped enum qualifiers and left `ELogType` without an `Info` variant, while macros like `LOG_INFO`, `LOGF_INFO`, `LOGN_INFO`, `LOGFN_INFO` passed raw integers or caused namespace collisions. Furthermore, `<string_view>` and `<fmt/format.h>` were omitted from `logger.h`.
- **Fixes**:
  - Added `#include <string_view>` and `#include <fmt/format.h>` to `FiveInvader/os/logger.h`.
  - Added `Info` variant to `os::logger::ELogType` (`None`, `Info`, `Utility`, `Warning`, `Error`).
  - Correctly mapped `LOG_INFO(...)`, `LOGF_INFO(...)`, `LOGN_INFO(...)`, and `LOGFN_INFO(...)` to `os::logger::ELogType::Info`.
  - Updated OS-specific implementations (`logger_linux.cpp`, `logger_macos.cpp`, `logger_win32.cpp`) to explicitly qualify `os::logger::ELogType` in method signatures and added `case ELogType::Info:` handling in `logger_win32.cpp`.
  - Audited `FiveInvader/kernel/io/p2p_manager.cpp` and related networking headers to ensure logger namespaces and format strings cleanly match `os::logger` declarations.

### B. Patch File Renaming & CMake Alignment
- **Root Cause**: Source files in `FiveInvader/patches/` were named `gta4_*` from legacy SanRecomp/Liberty development, whereas `CMakeLists.txt` and internal header includes expected `gta5_*`.
- **Files Renamed**:
  - `FiveInvader/patches/gta4_patches.cpp` → `FiveInvader/patches/gta5_patches.cpp`
  - `FiveInvader/patches/gta4_patches.h` → `FiveInvader/patches/gta5_patches.h`
  - `FiveInvader/patches/gta4_aspect_ratio_patches.cpp` → `FiveInvader/patches/gta5_aspect_ratio_patches.cpp`
  - `FiveInvader/patches/gta4_input_patches.cpp` → `FiveInvader/patches/gta5_input_patches.cpp`
  - `FiveInvader/patches/gta4_input_patches.h` → `FiveInvader/patches/gta5_input_patches.h`
- Added `#include "gta5_input_patches.h"` to `gta5_input_patches.cpp`.
- Updated `.gitignore` to whitelist `!docs/AGENT_HANDOVER.md`.

---

## 3. Stubbed Hooks Inventory

All legacy GTA IV memory addresses, function hooks, and jump table references have been audited and wrapped with `#if 0 ... #endif` or converted to clean no-op stubs. Every stubbed block is marked with:
`// TODO: Replace GTA IV virtual address with GTA V offset`

| File | Target / Symbol | Original Virtual Address / Context | Stubbing Method | Purpose in GTA IV / Note |
|---|---|---|---|---|
| `FiveInvader/patches/player_limit_patches.h` | `PLAYER_LOOPS_44[]` | `0x8267DB90` - `0x82689998` (15 addrs) | `#if 0` + empty array fallback | Proximity/bounds/player iteration loops in GTA IV |
| `FiveInvader/patches/player_limit_patches.h` | `PLAYER_LOOPS_46[]` | `0x826AE960` - `0x826B0830` (5 addrs) | `#if 0` + empty array fallback | Vtable & player iteration loops in GTA IV |
| `FiveInvader/patches/player_limit_patches.h` | `PLAYER_LOOPS_OTHER[]` | `0x827FC738`, `0x8238F700` (2 addrs) | `#if 0` + empty array fallback | Distance checks and vtable dispatch loops |
| `FiveInvader/patches/player_limit_patches.cpp` | `sub_826A7858` | `0x826A7858` (accessor), `0x826A78AC`, `0x826A78BC` | `#if 0 ... #endif` | Primary gatekeeper player accessor function |
| `FiveInvader/patches/player_limit_patches.cpp` | Loop hooks (22 functions) | `sub_8267DB90` through `sub_8238F700` | `#if 0 ... #endif` | Individual loop hook overrides that previously caused unresolved LLD symbols |
| `FiveInvader/patches/audio_patches.cpp` | `sub_822EEDB8` | `0x822EEDB8` | `#if 0 ... #endif` | GTA IV RAGE `audEngine` initialization |
| `FiveInvader/patches/audio_patches.cpp` | `sub_821AB5F8` | `0x821AB5F8` | `#if 0 ... #endif` | GTA IV radio station initialization |
| `FiveInvader/patches/memcpy_patches.cpp` | `sub_82990830` | `0x82990830` | `#if 0 ... #endif` | GTA IV CRT optimized memcpy |
| `FiveInvader/patches/memcpy_patches.cpp` | `sub_82990880` | `0x82990880` | `#if 0 ... #endif` | GTA IV CRT aligned memcpy continuation |
| `FiveInvader/kernel/save_hooks.cpp` | `sub_829A1C38` | `0x829A1C38` | `#if 0 ... #endif` | Content creation wrapper (`XamContentCreateEx`) |
| `FiveInvader/kernel/save_hooks.cpp` | `sub_829A1CA0` | `0x829A1CA0` | `#if 0 ... #endif` | Content close wrapper (`XamContentClose`) |
| `FiveInvader/kernel/save_hooks.cpp` | `sub_829A1CB8` | `0x829A1CB8` | `#if 0 ... #endif` | Content enumeration wrapper |
| `FiveInvader/kernel/save_hooks.cpp` | `sub_8297A930` | `0x8297A930` | `#if 0 ... #endif` | Save manager operation orchestrator |
| `FiveInvader/kernel/save_hooks.cpp` | `sub_82122CA0` | `0x82122CA0` (`0x81209104` vtables) | `#if 0 ... #endif` | Save slot context initialization |
| `FiveInvader/kernel/save_hooks.cpp` | `sub_821200D0` | `0x821200D0` | `#if 0 ... #endif` | Post-init save & profile loading |
| `FiveInvader/kernel/game_init.h` | `GameInitGlobals` | `0x82126194`, `0x8305C1B0`, `0x8312579A`, `0x83084044`... | Marker added | Hardcoded GTA IV engine/global address table |
| `FiveInvader/kernel/game_init.cpp` | `__imp__sub_8218C600`... | 6 GTA IV entry imports | `#if 0 ... #endif` | Core engine, game manager, memory pool imports |
| `FiveInvader/kernel/game_init.cpp` | `GameInit::*` | `InitCoreEngine`, `InitGameManager`, `Initialize` | Stubbed with no-op / success | Resolves link-time missing symbol errors |
| `FiveInvader/kernel/imports.cpp` | `g_knownSpinWaitFunctions` | `0x82672E50` - `0x823005E0` (6 addrs) | `#if 0` + empty set fallback | GTA IV wait-loop yield registry |
| `FiveInvader/kernel/imports.cpp` | `sub_82192578` | `0x82192578` | `#if 0 ... #endif` | GTA IV thread initialization hook |
| `FiveInvader/kernel/imports.cpp` | `sub_825383D8` | `0x825383D8` (`XapiInitProcess`) | `#if 0 ... #endif` | Guest function hook for Xapi init |
| `FiveInvader/patches/gta5_patches.cpp` | `sub_82XXXXXX`, `sub_82YYYYYY` | Placeholders | Marker added | Future `CGame::Initialize` and `Process` hooks |

---

## 4. The Core Architecture Mismatch

### GTA IV vs. GTA V Xbox 360 Binary Translation Differences

1. **Virtual Address Space & Binary Scale**:
   - **GTA IV (SanRecomp/Liberty)**: Base binary size ~12-16 MB. The `.text` code section spans `0x82000000` through approximately `0x82A13D5C`. CRT and helper routines (`savegprlr`, `restgprlr`, `memcpy`) sit in the high `0x82980000` - `0x829A0000` range.
   - **GTA V (FiveInvader `default.xex`)**: GTA V is vastly larger (~26-32 MB decompressed `.text`). Function offsets, entry points, and global variables do not align with GTA IV. The base load address on Xbox 360 remains in the `0x82000000` region, but code density, branch targets, and section layouts are fundamentally shifted.

2. **RAGE Engine Generation Shift**:
   - GTA IV uses early RAGE with distinct subsystem lists (63 subsystems initialized in `sub_82120FB8`).
   - GTA V uses an overhauled generation of RAGE featuring multi-threaded script virtual machines, Euphoria 3 physics pipelines, hierarchical audio streaming (`.awc` audio containers instead of legacy structures), and modified memory pooling.
   - Direct calls to `sub_8218C600` or `sub_82120000` are completely invalid in GTA V and cause missing symbol errors or runtime faults if unresolved.

3. **Key Address Ranges to Locate in GTA V Next**:
   - **XEX Entry Point**: Find GTA V's startup entry point from `default.xex` header.
   - **Register Save/Restore Helper Functions**: Locate `savegprlr_14` through `savegprlr_31`, `restgprlr_14` through `restgprlr_31`, `savefpr`, `restfpr`, `savevmx`, `restvmx` using signature scanning (pattern matching).
   - **Memory & CRT Primitives**: Locate `memcpy`, `memmove`, `memset`, and `longjmp`/`setjmp`.
   - **RAGE Audio Subsystem**: Identify the entry point for `audEngine` and speech/radio bank mounting routines.
   - **Save System & Content API Wrappers**: Locate GTA V's calls into `XamContentCreateEx` and save game deserialization.
   - **Player & Network Tables**: Identify GTA V's network player manager structure and max player constant checks.

---

## 5. Immediate Next Steps for the Powerful Host

Once this commit is pulled onto the primary build machine with adequate CPU and disk resources:

### Step 1: Pull and Initialize Submodules
```bash
git pull origin main
git submodule update --init --recursive
```

### Step 2: Place Original GTA V Assets
Copy the legally dumped Xbox 360 GTA V files into `./FiveInvaderLib/private/`:
- `FiveInvaderLib/private/default.xex`
- `FiveInvaderLib/private/xbox360.rpf`

*(Ensure no game binaries or archives are staged or committed to Git).*

### Step 3: Configure Recompiler Config (`FiveInvaderLib/config/GTA5.toml`)
Update `GTA5.toml` to point to the actual register save/restore addresses discovered via pattern scanning on GTA V's `default.xex`.

### Step 4: Run Static Binary Recompilation
Using XenonRecomp:
```bash
./tools/XenonRecomp/build/XenonRecomp ./FiveInvaderLib/config/GTA5.toml
```

### Step 5: Configure and Build with CMake / Ninja

#### Linux (x64 / ARM64):
```bash
# Configure preset
cmake --preset linux-release

# Compile FiveInvader binary
cmake --build ./out/build/linux-release --target FiveInvader -j$(nproc)
```

#### Windows (Clang-CL / Ninja):
```powershell
# In VS 2022 Developer Command Prompt:
cmake --preset x64-Clang-Release
cmake --build .\out\build\x64-Clang-Release --target FiveInvader -j%NUMBER_OF_PROCESSORS%
```

#### macOS (Apple Silicon / Intel):
```bash
cmake --preset macos-release
cmake --build ./out/build/macos-release --target FiveInvader -j$(sysctl -n hw.ncpu)
```
