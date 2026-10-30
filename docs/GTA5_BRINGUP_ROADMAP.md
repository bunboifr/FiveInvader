# FiveInvader: GTA V Xbox 360 Bringup Roadmap

This document serves as an operational engineering guide for the primary build host to execute static binary recompilation, resolve link-stage dependencies, and verify the CRT boot entrypoint for **Grand Theft Auto V** (Xbox 360 `default.xex`).

---

## 1. System Requirements & Setup on Primary Host

### Host Prerequisites
- **CPU**: Multi-core modern x86_64 (16+ threads strongly recommended for Clang/Ninja)
- **RAM**: Minimum 32 GB RAM (64 GB recommended for compiling 500+ generated PPC translation units)
- **Toolchain**:
  - Clang / LLVM 16+ (or MSVC Clang-CL on Windows)
  - LLD linker (`-fuse-ld=lld`)
  - CMake 3.20+
  - Ninja 1.11+
  - OpenSSL 3.0+ / vcpkg

### Step 1: Submodule Initialization
```bash
git pull origin main
git submodule update --init --recursive
```

### Step 2: Target Binary Verification
Place the legally acquired Xbox 360 GTA V disc 2 retail executable into `FiveInvaderLib/private/`:
- **Path**: `FiveInvaderLib/private/default.xex`
- **Media ID**: `79E465C5`
- **Title ID**: `545408A7`
- **Base Size**: ~26,628,096 bytes (decompressed / unencrypted)

> [!IMPORTANT]
> Verify that `FiveInvaderLib/private/default.xex` is never tracked by Git. Confirm with `git status` that it is ignored.

---

## 2. XenonRecomp Translation Pipeline

### Step 1: Build XenonRecomp Tool
From the repository root:
```bash
cmake -B tools/XenonRecomp/build -S tools/XenonRecomp -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C tools/XenonRecomp/build
```

### Step 2: Translate `default.xex`
Run the binary translator using the audited GTA V configuration:
```bash
./tools/XenonRecomp/build/XenonRecomp ./FiveInvaderLib/config/GTA5.toml ./FiveInvader/cpu/ppc_context.h
```

### Expected Output in `FiveInvaderLib/ppc/`:
- `ppc_recomp.0.cpp` through `ppc_recomp.545.cpp` (recompiled C++ translation units)
- `ppc_func_mapping.cpp` (virtual address to function pointer dispatch table)
- `ppc_recomp_shared.h` (function prototypes and register context macros)
- `ppc_config.h` & `ppc_context.h`

---

## 3. Priority Ranking of the 89 Undefined Linker Symbols

During initial compile-first link testing, LLD identified 89 missing symbols. They are prioritized below into four actionable tiers.

### Tier 1: Critical for CRT Boot & Memory Management (Priority: IMMEDIATE)
Without these kernel and memory primitives, the runtime will crash before `mainCRTStartup` finishes C runtime initialization.

| Symbol | Subsystem | Required Implementation Strategy |
|---|---|---|
| `__imp__NtYieldExecution` | Kernel Scheduler | Map directly to `std::this_thread::yield()`. Returns `0` (STATUS_SUCCESS). |
| `__imp__ExAllocatePool` | Memory Manager | Allocate memory block from guest heap or host virtual allocator (`malloc` aligned to 16 bytes). Return allocated pointer. |
| `__imp__MmMapIoSpace` | Hardware / GPU | Map physical hardware address range into guest virtual address space (return virtual address). |
| `__imp__KeSetCurrentProcessType` | Kernel Process | Set guest process type flag (e.g. Title Process `1`). Return `0`. |
| `__imp__VdSetDisplayModeOverride` | Video Driver | Acknowledge 720p display mode parameter (1280x720, 60Hz). Return `0`. |
| `__imp__NtWriteFileGather` | Kernel File I/O | Scatter-gather file write wrapper used by CRT stdout/stderr and trace logs. Implement synchronous write loop. |

---

### Tier 2: Host Framework, Engine Linkage & Subsystem Integration (Priority: HIGH)
These are linker failures from host libraries, missing source definitions, or unlinked submodules.

| Symbol | Root Cause | Fix / Resolution |
|---|---|---|
| `liberty::install::StfsParser::kBlockSize` | Missing static member definition | Add `constexpr size_t StfsParser::kBlockSize = 0x1000;` in `FiveInvader/install/stfs_parser.cpp`. |
| `plume::CreateVulkanInterface()` | Missing Vulkan backend linkage | Ensure `tools/nebula` or Vulkan render backend target is compiled into `FiveInvader`. |
| `Net::CreateSessionTracker()` | Missing source in CMake | Add `session_tracker.cpp` to `FiveInvader/CMakeLists.txt` or provide mock session tracker. |
| `Net::GameModeToString()` | Missing enum stringifier | Provide lookup table implementation in `FiveInvader/kernel/io/p2p_manager.cpp`. |
| `Net::MapAreaToString()` | Missing enum stringifier | Provide lookup table implementation in `FiveInvader/kernel/io/p2p_manager.cpp`. |
| `absl::...::LogMessage::operator<<` | Abseil logging link mismatch | Link `absl_log` and `absl_log_internal_message` targets in root `CMakeLists.txt`. |
| `__imp__KeInitializeDpc` | Kernel DPC | Initialize DPC struct (`routine`, `context`). Can be a basic struct copy. |
| `__imp__KeInsertQueueDpc` | Kernel DPC | Queue DPC for worker execution or execute inline immediately. |

---

### Tier 3: Secondary XAM / Network / Telemetry (Priority: MEDIUM - Safe No-Ops / Stubs)
These functions are called during online checks, telemetry, audio ducking, and Xbox Live matchmaking. They can safely return default success (`0`) or not found (`ERROR_NOT_FOUND`) during early bringup.

#### A. Network & HTTP Telemetry (19 symbols)
- `NetDll_XHttpStartup`, `NetDll_XHttpShutdown`
- `NetDll_XHttpOpen`, `NetDll_XHttpCloseHandle`, `NetDll_XHttpConnect`
- `NetDll_XHttpOpenRequest`, `NetDll_XHttpSendRequest`, `NetDll_XHttpReceiveResponse`
- `NetDll_XHttpQueryHeaders`, `NetDll_XHttpReadData`, `NetDll_XHttpWriteData`, `NetDll_XHttpDoWork`
- `NetDll_XHttpSetStatusCallback`
- `NetDll_XNetRegisterKey`, `NetDll_XNetUnregisterKey`
- `NetDll_XNetInAddrToXnAddr`, `NetDll_XNetInAddrToServer`
- `NetDll_XNetConnect`, `NetDll_XNetDnsLookup`, `NetDll_XNetDnsRelease`, `NetDll_XNetGetOpt`
- `NetDll_XnpLogonGetStatus`

*Strategy*: Return `0` (or `ERROR_INTERNET_NAME_NOT_RESOLVED` where appropriate). Zero return prevents blocking threads.

#### B. Title Authentication & Accounts (6 symbols)
- `XampXAuthStartup`, `XampXAuthShutdown`, `XampXAuthGetTitleBuffer`
- `XamGetToken`, `XamFreeToken`
- `XNetLogonGetTitleID`, `XNetLogonGetMachineID`

*Strategy*: Set token buffer to dummy test GUID; return `0` (`ERROR_SUCCESS`).

#### C. User Profile & Age Verification (5 symbols)
- `XamUserGetAge`: Write `25` (adult) to output pointer; return `0`.
- `XamUserGetAgeGroup`: Write `AGEGROUP_ADULT` (`3`) to output pointer; return `0`.
- `XamUserGetDeviceContext`: Return `0`.
- `XamUserGetOnlineCountryFromXUID`: Write `"USA"` or country code `103`; return `0`.
- `XamUserGetMembershipTierFromXUID`: Write `XUSER_MEMBERSHIP_GOLD` (`6`); return `0`.

#### D. Shell UI & Guide Dialogs (6 symbols)
- `XamShowKeyboardUI`
- `XamShowMarketplaceUI`
- `XamShowMessageComposeUI`
- `XamShowFriendRequestUI`
- `XamShowMarketplaceDownloadItemsUI`
- `XamMarketplaceAcquireFreeContent`

*Strategy*: Return `ERROR_CANCELLED` (`0x800704C7`) or `0`. Prevents UI modal freeze.

#### E. Audio Ducking System (6 symbols)
- `XAudioGetDuckerThreshold`, `XAudioGetDuckerHoldTime`, `XAudioGetDuckerAttackTime`
- `XAudioGetDuckerReleaseTime`, `XAudioGetDuckerLevel`, `XAudioEnableDucker`

*Strategy*: Write default floating-point ducking factors (`1.0f`) to output registers; return `0`.

#### F. Background Downloads & Disc Management (6 symbols)
- `XamBackgroundDownloadSetMode`, `XamBackgroundDownloadGetMode`
- `XamBackgroundDownloadItemGetStatus`, `XamBackgroundDownloadItemGetHistoryStatus`
- `XamContentResolve`
- `XamSwapDisc`, `XamSwapCancel`
- `XexLoadImage`, `XexUnloadImage`

*Strategy*: Return no pending downloads / disc 1 loaded.

---

### Tier 4: Legacy GTA IV Address Hooks (Priority: LOW - Already Stubbed)
These 37 legacy hooks from GTA IV are already wrapped in `#if 0` with TODO comments and do **not** block compilation or linking:
- **Player Limit Loops (21 symbols)**: `sub_8267DB90` through `sub_8238F700`
- **Audio Hooks (2 symbols)**: `sub_822EEDB8`, `sub_821AB5F8`
- **Save System Hooks (6 symbols)**: `sub_829A1C38`, `sub_829A1CA0`, `sub_829A1CB8`, `sub_8297A930`, `sub_82122CA0`, `sub_821200D0`
- **Engine / Video Subsystems (8 symbols)**: `sub_8218C600`, `sub_82120EE8`, `sub_821250B0`, `sub_82124080`, `sub_82120FB8`, `sub_82318F60`, `sub_829D1758`, `sub_829D8860`

---

## 4. Entrypoint Verification (`mainCRTStartup` Tracking)

### Step 1: Discover GTA V Entrypoint
Inspect the headers of `default.xex`:
```bash
# Using xextool (if available) or readelf / python script:
python3 -c "
with open('FiveInvaderLib/private/default.xex', 'rb') as f:
    data = f.read(0x1000)
    # Read XEX header entrypoint field at offset 0x18
    import struct
    magic = data[:4]
    print(f'Magic: {magic}')
"
```
In XenonRecomp's generated `ppc_func_mapping.cpp`, find the entry point symbol matching the XEX header entrypoint address.

### Step 2: Signature Scan for Register Helper Routines
PowerPC MSVC/GCC binaries use standardized helper functions to save and restore non-volatile registers before function epilogues.
Scan `default.xex` for:
- `savegprlr_14`: `91 C1 ...` (`stw r14..r31` followed by `mflr r0; blr`)
- `restgprlr_14`: `81 C1 ...` (`lwz r14..r31` followed by `mtlr r0; blr`)
- `savefpr_14` / `restfpr_14` (floating point register store/load)
- `savevmx_14` / `restvmx_14` (AltiVec / VMX128 vector store/load)

Update the addresses in `FiveInvaderLib/config/GTA5.toml`.

### Step 3: Boot Sequence Verification
1. Launch `FiveInvader`:
   ```bash
   ./out/build/linux-release/FiveInvader/FiveInvader
   ```
2. Check standard output for CRT initialization:
   ```text
   [FiveInvader:CPU] Initializing guest PowerPC context...
   [FiveInvader:Kernel] Mapping 512MB guest physical memory (0x80000000 - 0xA0000000)...
   [FiveInvader:Kernel] Jumping to entrypoint (mainCRTStartup @ 0x82XXXXXX)...
   [FiveInvader:Native:INFO] Registered core GTA V script natives
   ```
3. Monitor for unhandled native calls or unresolved guest memory accesses.
