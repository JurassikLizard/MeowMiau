# MeowMiau

A simple C project built with raylib for creating graphics-based applications.

## Project Overview

MeowMiau is a lightweight C application that demonstrates basic raylib usage. It provides a foundation for developing graphics applications with support for multiple platforms and build environments.

## Prerequisites

### General Requirements
- **Git** - For cloning the repository
- **CMake** - Version 3.16 or higher

### Platform-Specific Requirements

#### Windows (MSYS2)
- **MSYS2** - Unix-like environment for Windows
  - Install from: https://www.msys2.org/
  - Once installed, run the MSYS2 terminal and install build tools:
    ```bash
    pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make
    ```

#### Docker
- **Docker** - For containerized builds
  - Install from: https://www.docker.com/

## Building the Project

### Option 1: Windows with MSYS2

1. Clone the repository:
   ```bash
   git clone --recurse-submodules <repository-url>
   cd MeowMiau
   ```

2. Run the build script:
   ```bash
   ./scripts/build_win_msys.sh
   ```

   Or manually:
   ```bash
   cmake -S . -B build-win -DPLATFORM=Desktop -G "MSYS Makefiles"
   cmake --build build-win
   ```

3. Run the executable:
   ```bash
   ./build-win/meowmiau.exe
   ```

### Option 2: DRM Mode (no windows, overrides whole screen) for Raspberry Pi w/ Alpine

1. Clone the repository:
   ```bash
   git clone --recurse-submodules <repository-url>
   cd MeowMiau
   ```
2. Run the build script
   ```bash
   ./scripts/build_win_msys.sh
   ```

---OR---

2. Build the Docker image:
   ```bash
   docker build -f docker/Dockerfile -t meowmiau .
   ```

3. Run the container with build:
   ```bash
   docker run --rm -v $(pwd):/app meowmiau /bin/bash -c "cmake -S . -B build-docker -DPLATFORM=Desktop && cmake --build build-docker"
   ```

### Option 3: Linux with System Compiler

1. Install dependencies (Ubuntu/Debian):
   ```bash
   sudo apt-get install build-essential cmake git
   ```

2. Clone and build:
   ```bash
   git clone --recurse-submodules <repository-url>
   cd MeowMiau
   cmake -S . -B build-linux -DPLATFORM=Desktop
   cmake --build build-linux
   ```

3. Run the executable:
   ```bash
   ./build-linux/meowmiau
   ```

## Project Structure

```
MeowMiau/
├── src/              # Source code
│   └── main.c        # Main application file
├── external/         # External dependencies
│   └── raylib/       # Raylib graphics library
├── cmake/            # CMake custom modules and toolchains
│   └── toolchains/   # Cross-compilation toolchains
├── scripts/          # Build and utility scripts
├── docker/           # Docker configurations
└── CMakeLists.txt    # CMake build configuration
```

## Dependencies

- **Raylib** - A simple and easy-to-use library for video games and multimedia (included)
  - Documentation: https://www.raylib.com/
  - GitHub: https://github.com/raysan5/raylib

## Contributing

I welcome contributions! Please submit PRs.
- Keep functions focused and well-documented
- Test on both Windows (MSYS2) and RPi-DRM before submitting

## Support

For issues, questions, or suggestions, please open an issue on the repository.
