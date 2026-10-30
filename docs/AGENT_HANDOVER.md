# Agent Handover: GTA V Compile-First Build

Date: 2026-10-30

## Current State

- CMake configuration succeeded in `build/`; Ninja target `FiveInvader` compiles
  its C++ translation units and reaches the final executable link.
- The last uncapped linker run did **not** link: LLD reported 89 undefined
  symbols. Stop here for this handover; no further build or link attempts were
  made after the request to halt compilation.
- The configured compiler is `clang++` 22.1.8. The configured Ninja version is
  1.13.2, with `x64-linux` as the vcpkg triplet.
- The configured linker flags are
  `-fuse-ld=lld -Wl,--error-limit=0`. LLD is necessary because the fmt archive
  is built from LLVM ThinLTO bitcode; the earlier default GNU `ld.bfd` link
  rejected that archive as an unrecognized file format.
- vcpkg is checked out at `../vcpkg` relative to the project, on commit
  `d186940b2f`; OpenSSL 3.4.0 is installed. CMake's cache has this vcpkg root
  and the OpenSSL target used by the Linux build.
- Re-run, only when authorized, from the project directory:

  ```sh
  VCPKG_ROOT=/home/triadwoozie/Documents/FiveInvader/vcpkg \
    ninja -C build FiveInvader
  ```

## Binary and Asset Status

- The candidate Xbox 360 executable is
  `../rom_data/extracted_disc2/default.xex`, outside this Git repository. It is
  26,628,096 bytes; `file` identifies it as an Xbox 360 executable with media
  ID `79E465C5`, all regions. Do not copy or stage it.
- `FiveInvaderResources/` is present in the working tree and occupies about
  112 MB. These resources are pre-existing, untracked game/application assets;
  they were not staged or committed.
- Build outputs are under the ignored `build/` directory. Temporary logs and
  tool-output dumps created or used during this session were removed. The
  remaining untracked `.bin` files are the existing resource files, not
  temporary compiler outputs. No untracked `.o`, `.a`, `.xex`, or scratch
  build-log files were found in the repository status.

## Inventory of Focused Code Changes

The handover commit contains only the focused code files listed below plus
this document. Other dirty GTA V port work and untracked resource/tool trees
were left unstaged.

- `FiveInvader/os/logger.h`
  - Corrected the logging macro expansion so the `ELogType` namespace/type is
    not prefixed twice.
  - The no-category macros pass the enum's zero value directly instead of
    using the token `None`. This also avoids collision with X11's `None` macro.
  - The affected categories are the regular, formatted, utility, and
    non-function-specific logging macros.
- `FiveInvader/install/rpf_extractor.cpp` and
  `FiveInvader/CMakeLists.txt`
  - Restricted CommonCrypto to Apple platforms.
  - Added an OpenSSL EVP AES-256-ECB implementation for other non-Windows
    platforms and linked `OpenSSL::Crypto` for Linux.
  - This is a portability fix, not a bypass of archive decryption.
- `FiveInvader/kernel/imports.cpp`
  - Added zero-return compile-first stubs for these 20 guest API entrypoints:

    ```text
    __imp__XamUserGetOnlineCountryFromXUID
    __imp__XamGetLanguage
    __imp__KeInsertQueueDpc
    __imp__XamUserGetMembershipTierFromXUID
    __imp__XAudioGetDuckerLevel
    __imp__XNetLogonGetTitleID
    __imp__KeInitializeDpc
    __imp__XamVoiceIsActiveProcess
    __imp__XNetLogonGetMachineID
    __imp__NetDll_XHttpStartup
    __imp__NetDll_XHttpShutdown
    __imp__NetDll_XHttpOpen
    __imp__NetDll_XHttpCloseHandle
    __imp__NetDll_XHttpConnect
    __imp__NetDll_XHttpSetStatusCallback
    __imp__NetDll_XHttpOpenRequest
    __imp__NetDll_XHttpSendRequest
    __imp__NetDll_XHttpReceiveResponse
    __imp__NetDll_XHttpQueryHeaders
    __imp__NetDll_XHttpReadData
    ```

  - These handlers set `r3` to zero and do not provide real API behavior.
    Treat them as temporary compile/link placeholders, not as finished runtime
    implementations.
  - The file also has big-endian `be<uint32_t>` values in critical-section
    code; formatting now uses `.get()` on those wrappers.
- `FiveInvader/patches/player_limit_patches.cpp`
  - The existing `#if 0` block around the original player-subsystem lookup
    (`sub_826A6CC8` / `sub_826AE738`, including the old LR values
    `0x826A78AC` and `0x826A78BC`) remains disabled. The direct array fallback
    in `sub_826A7858` remains active.
  - This `#if 0` block was already in the dirty worktree before this session;
    it is included because it is directly relevant to the compile-first
    handover. The loop-forwarding hooks in this file are **not** stubbed and
    remain unresolved at link time.
- No other game-function hooks were newly disabled in this session. In
  particular, do not interpret the 20 API import stubs as stubs for the
  unresolved game-function wrappers below.

## Linker Failure Breakdown

The captured uncapped LLD run reported **89 undefined symbols**. The first
group of 20 import stubs above was present for this run; the remaining guest
imports listed here are distinct missing implementations. The output also
contains unresolved wrappers for inherited game addresses, platform/library
symbols, and a missing session-tracker implementation.

### Remaining guest/XDK API imports (45)

```text
__imp__NetDll_XNetRegisterKey
__imp__NetDll_XNetUnregisterKey
__imp__NetDll_XNetInAddrToXnAddr
__imp__NetDll_XNetInAddrToServer
__imp__NetDll_XNetConnect
__imp__NetDll_XNetDnsLookup
__imp__NetDll_XNetDnsRelease
__imp__NetDll_XNetGetOpt
__imp__NetDll_XHttpWriteData
__imp__NetDll_XHttpDoWork
__imp__XamFreeToken
__imp__XamGetToken
__imp__XampXAuthStartup
__imp__XampXAuthShutdown
__imp__XampXAuthGetTitleBuffer
__imp__NtYieldExecution
__imp__XamUserGetDeviceContext
__imp__MmMapIoSpace
__imp__XamUserGetAge
__imp__XamUserGetAgeGroup
__imp__NetDll_XnpLogonGetStatus
__imp__XamShowMarketplaceDownloadItemsUI
__imp__XAudioGetDuckerThreshold
__imp__VdSetDisplayModeOverride
__imp__XAudioGetDuckerHoldTime
__imp__XAudioGetDuckerAttackTime
__imp__XAudioGetDuckerReleaseTime
__imp__XAudioEnableDucker
__imp__KeSetCurrentProcessType
__imp__NtWriteFileGather
__imp__XamBackgroundDownloadSetMode
__imp__XamBackgroundDownloadGetMode
__imp__XamContentResolve
__imp__XamShowKeyboardUI
__imp__XamShowMarketplaceUI
__imp__XamShowMessageComposeUI
__imp__XamShowFriendRequestUI
__imp__XamBackgroundDownloadItemGetStatus
__imp__XamBackgroundDownloadItemGetHistoryStatus
__imp__ExAllocatePool
__imp__XexUnloadImage
__imp__XexLoadImage
__imp__XamSwapDisc
__imp__XamSwapCancel
__imp__XamMarketplaceAcquireFreeContent
```

### Inherited game-function wrappers and patch hooks

- Six save wrappers in `FiveInvader/kernel/save_hooks.cpp` call missing
  generated originals:

  ```text
  __imp__sub_829A1C38
  __imp__sub_829A1CA0
  __imp__sub_829A1CB8
  __imp__sub_8297A930
  __imp__sub_82122CA0
  __imp__sub_821200D0
  ```

- 21 original functions forwarded by the legacy player-loop hooks in
  `FiveInvader/patches/player_limit_patches.cpp` are unresolved:

  ```text
  __imp__sub_8267DB90  __imp__sub_8267D798  __imp__sub_8267D910
  __imp__sub_8267DE28  __imp__sub_8267E198  __imp__sub_8267EBA8
  __imp__sub_8267ECF0  __imp__sub_8267F318  __imp__sub_8267F838
  __imp__sub_82683998  __imp__sub_82685D20  __imp__sub_82685EB0
  __imp__sub_826861D0  __imp__sub_82689828  __imp__sub_82689998
  __imp__sub_826AE960  __imp__sub_826AF5B8  __imp__sub_826B0720
  __imp__sub_826B0830  __imp__sub_827FC738  __imp__sub_8238F700
  ```

- Two audio hook originals in `FiveInvader/patches/audio_patches.cpp` are
  unresolved:

  ```text
  __imp__sub_822EEDB8
  __imp__sub_821AB5F8
  ```

- Other inherited PPC wrapper targets reported by LLD:

  ```text
  __imp__sub_8218C600
  __imp__sub_82120EE8
  __imp__sub_821250B0
  __imp__sub_82124080
  __imp__sub_82120FB8
  __imp__sub_82318F60
  __imp__sub_829D1758
  __imp__sub_829D8860
  ```

  The `sub_829D1758` and `sub_829D8860` hooks are in the active
  `FiveInvader/gpu/video.cpp` code. The old
  `FiveInvader/patches/video_patches.cpp` source is excluded from the current
  executable source list; it is not the source of those two references.

### Other unresolved symbols

```text
absl::lts_20240722::log_internal::LogMessage::operator<<<unsigned long, 0>(unsigned long const&)
absl::lts_20240722::log_internal::LogMessage::operator<<<int, 0>(int const&)
liberty::install::StfsParser::kBlockSize
plume::CreateVulkanInterface()
Net::CreateSessionTracker()
Net::GameModeToString(Net::GameMode)
Net::MapAreaToString(Net::MapArea)
```

The session-tracker references originate from the P2P manager path; check
whether the required implementation source is included in the active CMake
target before writing replacement code. The Abseil and `kBlockSize` failures
are separate library/definition issues and should not be papered over as GTA V
address stubs.

## Root Architecture Problem

FiveInvader is being compiled as a static recompilation runtime for GTA V, but
the active recompiled PPC library and game-specific patch sources still
contain inherited fixed virtual addresses and wrapper names from an earlier
game configuration. A function name such as `sub_8267DB90` is not a portable
symbol: its existence and semantics depend on the exact executable analyzed
to generate the PPC source. The generated `ppc_func_mapping.cpp`,
`ppc_recomp.*.cpp`, patch wrappers, save hooks, and hand-entered player-loop
table must all describe the same GTA V executable.

The local GTA V `default.xex` is available, but its presence alone does not
prove that any inherited entrypoint or table entry matches it. Do not infer
function identity from an address range or reuse GTA IV-era offsets. The
runtime should not execute these hooks until each target has been mapped to
the selected GTA V executable and generated source.

## Handover Checklist

1. Keep compilation halted until a separate task authorizes a build. Preserve
   the current `default.xex` and resource paths without adding game data to
   Git.
2. Verify the executable identity and module/load address assumptions for the
   exact `default.xex`; record its hash and title/media metadata in local
   research notes, not by committing the binary.
3. Configure the recompiler against that executable and regenerate the PPC
   output, function mapping, and switch/function tables as one consistent
   set. Check the generated output for the inherited `sub_82...` names before
   compiling.
4. For each unresolved `sub_...` hook, locate its generated GTA V function
   and confirm its entry address, call signature, and original-function
   symbol. Update the hook and mapping only after that verification. If there
   is no supported GTA V equivalent, remove the hook registration or gate it
   behind an explicit disabled option; do not retain a forwarding call to a
   nonexistent `__imp__` function.
5. Review the player-limit address table and `sub_826A7858` path as a single
   feature. Either map each loop/accessor against GTA V and validate its
   structure offsets, or disable the feature as a whole; the current constants
   and zero-return memory allocation placeholders are not validated runtime
   behavior.
6. Resolve remaining XDK imports by ABI and actual call-site use. Implement
   only required APIs; where intentionally unsupported, use an explicit
   documented no-op/failure contract appropriate to the API rather than a
   blanket zero return.
7. Resolve non-game link errors separately: check CMake source registration
   for the session tracker and Vulkan implementation, the Abseil library
   configuration, and the `StfsParser::kBlockSize` definition.
8. Once authorized, run the focused target with the configured LLD flags,
   record the new unresolved-symbol count, and repeat until the target links.
   Then run the applicable tests and verify no game assets or build outputs
   have entered the staged changes.
