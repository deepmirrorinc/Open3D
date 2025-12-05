## Open3D ARM64 Cross-Compilation Guide

**Author:** DeepMirror

This document is based on the cross-compilation scripts and CMake modifications in the current repository. It summarizes the dependencies, configurations, and common issue resolutions required to build Open3D for Linux ARM64 (aarch64) targets on an x86_64 Ubuntu host.

### Scope
- **Host**: Ubuntu 20.04/22.04 x86_64 (sudo privileges required)
- **Target**: Linux aarch64 root filesystem based on glibc
- **Open3D Version**: `master` (includes the modifications described below)

---

### 1. Host Dependencies (x86_64)
1. Install general build tools and fetch project dependencies:
   ```bash
   sudo apt-get update
   sudo apt-get install git build-essential ninja-build pkg-config ccache
   bash util/install_deps_ubuntu.sh assume-yes
   ```
2. Ensure **CMake ≥ 3.20** is available. Ubuntu 20.04 ships with 3.16, which can be installed from the [Kitware APT repository](https://apt.kitware.com/) or using official binaries.
3. Since Shader encoding tools need to run on the host, a complete host compilation environment (gcc/g++) is required.

---

### 2. Installing Cross-Toolchain and ARM64 Dependencies
1. **Enable ARM64 architecture and configure ports repository**
   ```bash
   sudo dpkg --add-architecture arm64
   sudo tee /etc/apt/sources.list.d/ubuntu-ports.list >/dev/null <<'EOF'
   deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal main restricted universe multiverse
   deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-updates main restricted universe multiverse
   deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-security main restricted universe multiverse
   EOF
   sudo apt-get update
   ```
2. **Install cross-compilation toolchain**
   ```bash
   sudo apt-get install crossbuild-essential-arm64 \
        gfortran-aarch64-linux-gnu binutils-aarch64-linux-gnu \
        pkg-config-aarch64-linux-gnu
   ```
3. **Install ARM64 target dependencies** (headers and libraries required for Open3D GUI/rendering/IO):
   ```bash
   sudo apt-get install \
     libx11-dev:arm64 libxrandr-dev:arm64 libxinerama-dev:arm64 \
     libxcursor-dev:arm64 libxi-dev:arm64 libglu1-mesa-dev:arm64 \
     libosmesa6-dev:arm64 libudev-dev:arm64 libtbb-dev:arm64 \
     zlib1g-dev:arm64 libpng-dev:arm64 libturbojpeg0-dev:arm64
   ```
   Additional target libraries such as `libcurl4-openssl-dev:arm64`, `librealsense2-dev:arm64` can be installed as needed.

---

### 3. Cross-Compilation Toolchain File
File: `cmake/aarch64.toolchain.cmake`

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(LINUX_AARCH64 TRUE)
set(PROCESSOR_ARCH aarch64)

set(CMAKE_SYSROOT /)
set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
set(CMAKE_Fortran_COMPILER /usr/bin/aarch64-linux-gnu-gfortran)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Key points:
- Explicitly declare `LINUX_AARCH64` so that top-level CMake and third-party modules can consistently identify the target architecture.
- Specify `gfortran` so that OpenBLAS/LAPACK can find the correct Fortran compiler when building from source.

---

### 4. Build Script `build.sh`

This script implements a three-stage build process: **host tools**, **ARM64 target code**, and **x86_64 target code**:

1. **Build host tools**: Compile `ShaderEncoder` and `ShaderLinker` on the host and add the output directory to `PATH`, resolving the issue of "runtime tools not executable" during cross-compilation.

2. **Build ARM64 version**: Run CMake in the `build-aarch64` directory using the above toolchain:
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=/Open3D/cmake/aarch64.toolchain.cmake \
         -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
         -DCMAKE_INSTALL_INCLUDEDIR=include \
         -DCMAKE_INSTALL_LIBDIR=lib \
         -DBUILD_EXAMPLES=OFF \
         -DBUILD_PYTHON_MODULE=OFF \
         -DBUILD_GUI=OFF \
         -DUSE_BLAS=ON \
         -DBUILD_ISPC_MODULE=OFF \
         -DWITH_IPPICV=OFF \
         -DBUILD_VTK_FROM_SOURCE=ON \
         -DBUILD_CURL_FROM_SOURCE=ON \
         -DUSE_SYSTEM_EMBREE=OFF \
         -DBUILD_SHARED_LIBS=ON \
         -DCMAKE_Fortran_COMPILER=/usr/bin/aarch64-linux-gnu-gfortran \
         ..
   make -j4
   make install
   ```
   After the build completes, ARM64 library files will be moved to the `${INSTALL_PREFIX}/lib/aarch64/` directory.

3. **Build x86_64 version**: Build the native version in the `build-x86_64` directory:
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
         -DCMAKE_INSTALL_INCLUDEDIR=include \
         -DCMAKE_INSTALL_LIBDIR=lib \
         -DBUILD_PYTHON_MODULE=OFF \
         -DBUILD_GUI=OFF \
         -DUSE_BLAS=OFF \
         -DUSE_SYSTEM_BLAS=ON \
         -DBUILD_SHARED_LIBS=ON \
         -DBUILD_EXAMPLES=OFF \
         ..
   make -j
   make install
   ```
   After the build completes, x86_64 library files will be linked to the `${INSTALL_PREFIX}/lib/x86_64/` directory.

**Notes:**
- `WITH_IPPICV=OFF`: IPP only supports x86.
- `USE_SYSTEM_EMBREE=OFF`: Embree will be rebuilt with ARM configuration. If not needed, it can be further skipped in CMake.
- `BUILD_VTK_FROM_SOURCE=ON` / `BUILD_CURL_FROM_SOURCE=ON`: Avoid dependency on x86 precompiled binaries.
- Header files (include) are installed in the shared directory `${INSTALL_PREFIX}/include`, while library files are separated by architecture to `${INSTALL_PREFIX}/lib/aarch64/` and `${INSTALL_PREFIX}/lib/x86_64/`.

**Installation directory structure:**
```
${INSTALL_PREFIX}/
├── include/          # Shared header files (shared by both architectures)
└── lib/
    ├── aarch64/
    │   └── libOpen3D.so
    └── x86_64/
        └── libOpen3D.so -> ../libOpen3D.so
```

Run with:
```bash
bash build.sh
```

---

### 5. Key Modifications to Third-Party Dependencies

| File | Purpose | Description |
|------|---------|-------------|
| `3rdparty/find_dependencies.cmake` | Pass `CMAKE_SYSTEM_NAME/PROCESSOR/SYSROOT` to all `ExternalProject` | Ensures sub-projects like embree, VTK, curl maintain the same architecture settings as the main project, avoiding default fallback to host x86 |
| `3rdparty/openblas/openblas.cmake` | Fix `TARGET=ARMV8` and pass Fortran compiler | Resolves `-march=native` and "gfortran not found" issues during cross-compilation |
| `3rdparty/mkl/tbb.cmake` | Pass cross-compiler information | Ensures TBB generates ARM static libraries, avoiding default detection of host architecture |
| `build.sh` | Automatically build host tools, unify CMake options | Avoids repetitive manual configuration |

---

### 6. Quick Command Summary

```bash
# Prepare environment
sudo dpkg --add-architecture arm64
# (Add ports repository)
sudo apt-get update
sudo apt-get install crossbuild-essential-arm64 gfortran-aarch64-linux-gnu \
     libxrandr-dev:arm64 libxinerama-dev:arm64 libxcursor-dev:arm64 libxi-dev:arm64 \
     libglu1-mesa-dev:arm64 libosmesa6-dev:arm64 libudev-dev:arm64 libtbb-dev:arm64

# Clone and build
git clone https://github.com/isl-org/Open3D.git
cd Open3D
bash util/install_deps_ubuntu.sh assume-yes   # Optional (host dependencies)
bash build.sh
```

After the build completes, installation files will be located in the `${INSTALL_PREFIX}` directory (default: `/Open3D/install`):
- Header files: `${INSTALL_PREFIX}/include/`
- ARM64 library files: `${INSTALL_PREFIX}/lib/aarch64/libOpen3D.so`
- x86_64 library files: `${INSTALL_PREFIX}/lib/x86_64/libOpen3D.so`
