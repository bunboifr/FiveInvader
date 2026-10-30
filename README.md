# FiveInvader

FiveInvader is an open-source static recompilation runtime for Grand Theft Auto V
on Xbox 360, targeting Linux and Android. It is forked from the
[SanRecomp](https://github.com/OZORDI/SanRecomp) project and builds on the
cross-platform [MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)
framework. Android support is a bring-up target and is not yet a supported
build.

The project does not include game assets. You must provide files from your own
legally obtained copy of the game.

## Status

This project is in early development. PowerPC-to-C++ recompilation and the
desktop CMake structure are present; game-specific runtime, rendering, audio,
input, and Android integration remain active work.

## Build on Linux

Initialize the required submodules and install the Linux dependencies listed in
[`docs/BUILDING.md`](docs/BUILDING.md). Then configure and build:

```bash
git clone --recurse-submodules https://github.com/triadwoozie/FiveInvader.git
cd FiveInvader
export VCPKG_ROOT="$PWD/thirdparty/vcpkg"
cmake . --preset linux-release
cmake --build ./out/build/linux-release --target FiveInvader
```

The build requires the game's `default.xex` and `xbox360.rpf` in
`FiveInvaderLib/private/`; these files are not tracked by Git.

## Android

Android is a bring-up target, not a currently supported CMake preset. Android
toolchain, native lifecycle, and input integration remain to be implemented.

## Documentation

| Document | Description |
|---|---|
| [`docs/BUILDING.md`](docs/BUILDING.md) | Existing build configurations and dependencies |
| [`docs/DUMPING-en.md`](docs/DUMPING-en.md) | Game-file preparation information |
