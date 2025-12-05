## Open3D ARM64 交叉编译指引

**Author:** DeepMirror

本文档基于当前仓库中的交叉编译脚本和 CMake 修改，总结了在 x86_64 Ubuntu 主机上为 Linux ARM64（aarch64）目标构建 Open3D 所需的依赖、配置和常见问题处理方式。

### 适用范围
- **Host**：Ubuntu 20.04/22.04 x86_64（需要 sudo 权限）
- **Target**：基于 glibc 的 Linux aarch64 根文件系统
- **Open3D 版本**：`master`（包含下述修改）

---

### 1. 主机依赖（x86_64）
1. 安装通用构建工具并拉取项目依赖：
   ```bash
   sudo apt-get update
   sudo apt-get install git build-essential ninja-build pkg-config ccache
   bash util/install_deps_ubuntu.sh assume-yes
   ```
2. 确保拥有 **CMake ≥ 3.20**。Ubuntu 20.04 自带 3.16，可从 [Kitware APT 仓库](https://apt.kitware.com/) 安装或使用官方二进制。
3. 由于 Shader 编码工具需要在宿主上运行，必须具备完整的宿主编译环境（gcc/g++）。

---

### 2. 安装交叉工具链与 ARM64 依赖
1. **启用 ARM64 架构并配置 ports 源**
   ```bash
   sudo dpkg --add-architecture arm64
   sudo tee /etc/apt/sources.list.d/ubuntu-ports.list >/dev/null <<'EOF'
   deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal main restricted universe multiverse
   deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-updates main restricted universe multiverse
   deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-security main restricted universe multiverse
   EOF
   sudo apt-get update
   ```
2. **安装交叉编译链**
   ```bash
   sudo apt-get install crossbuild-essential-arm64 \
        gfortran-aarch64-linux-gnu binutils-aarch64-linux-gnu \
        pkg-config-aarch64-linux-gnu
   ```
3. **安装 ARM64 目标依赖**（对应 Open3D GUI/渲染/IO 需要的头文件与库）：
   ```bash
   sudo apt-get install \
     libx11-dev:arm64 libxrandr-dev:arm64 libxinerama-dev:arm64 \
     libxcursor-dev:arm64 libxi-dev:arm64 libglu1-mesa-dev:arm64 \
     libosmesa6-dev:arm64 libudev-dev:arm64 libtbb-dev:arm64 \
     zlib1g-dev:arm64 libpng-dev:arm64 libturbojpeg0-dev:arm64
   ```
   根据业务需要，可额外安装 `libcurl4-openssl-dev:arm64`、`librealsense2-dev:arm64` 等目标库。

---

### 3. 交叉编译工具链文件
文件：`cmake/aarch64.toolchain.cmake`

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

要点：
- 显式声明 `LINUX_AARCH64`，使顶层 CMake 与第三方模块能一致识别目标架构。
- 指定 `gfortran`，以便 OpenBLAS/LAPACK 从源码构建时能够找到正确的 Fortran 编译器。

---

### 4. 构建脚本 `build.sh`

该脚本实现了**主机工具**、**ARM64 目标代码**和**x86_64 目标代码**的三段式构建：

1. **构建主机工具**：在宿主上编译 `ShaderEncoder` 与 `ShaderLinker`，并把生成目录加入 `PATH`，解决交叉编译时"运行时工具不可执行"的问题。

2. **构建 ARM64 版本**：在 `build-aarch64` 目录中使用上述 toolchain 运行 CMake：
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
   构建完成后，ARM64 库文件会被移动到 `${INSTALL_PREFIX}/lib/aarch64/` 目录。

3. **构建 x86_64 版本**：在 `build-x86_64` 目录中构建原生版本：
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
   构建完成后，x86_64 库文件会被链接到 `${INSTALL_PREFIX}/lib/x86_64/` 目录。

**说明：**
- `WITH_IPPICV=OFF`：IPP 仅支持 x86。
- `USE_SYSTEM_EMBREE=OFF`：Embree 会按 ARM 配置重新构建，如无需可进一步在 CMake 中跳过。
- `BUILD_VTK_FROM_SOURCE=ON` / `BUILD_CURL_FROM_SOURCE=ON`：避免依赖 x86 预编译二进制。
- 头文件（include）安装在共享目录 `${INSTALL_PREFIX}/include`，库文件按架构分离到 `${INSTALL_PREFIX}/lib/aarch64/` 和 `${INSTALL_PREFIX}/lib/x86_64/`。

**安装目录结构：**
```
${INSTALL_PREFIX}/
├── include/          # 共享头文件（两个架构共用）
└── lib/
    ├── aarch64/
    │   └── libOpen3D.so
    └── x86_64/
        └── libOpen3D.so -> ../libOpen3D.so
```

运行方式：
```bash
bash build.sh
```

---

### 5. 第三方依赖的关键修改

| 文件 | 目的 | 说明 |
|------|------|------|
| `3rdparty/find_dependencies.cmake` | 向所有 `ExternalProject` 传递 `CMAKE_SYSTEM_NAME/PROCESSOR/SYSROOT` | 保证 embree、VTK、curl 等子项目与主项目保持相同架构设定，避免默认回退到主机 x86 |
| `3rdparty/openblas/openblas.cmake` | 固定 `TARGET=ARMV8` 并传入 Fortran 编译器 | 解决交叉编译过程中 `-march=native` 与 “未找到 gfortran” 的问题 |
| `3rdparty/mkl/tbb.cmake` | 传递交叉编译器信息 | 确保 TBB 生成 ARM 静态库，避免默认检测宿主架构 |
| `build.sh` | 自动构建宿主工具、统一 CMake 选项 | 避免重复手动配置 |

---

### 6. 快速命令汇总

```bash
# 准备环境
sudo dpkg --add-architecture arm64
# （添加 ports 源）
sudo apt-get update
sudo apt-get install crossbuild-essential-arm64 gfortran-aarch64-linux-gnu \
     libxrandr-dev:arm64 libxinerama-dev:arm64 libxcursor-dev:arm64 libxi-dev:arm64 \
     libglu1-mesa-dev:arm64 libosmesa6-dev:arm64 libudev-dev:arm64 libtbb-dev:arm64

# 克隆并构建
git clone https://github.com/isl-org/Open3D.git
cd Open3D
bash util/install_deps_ubuntu.sh assume-yes   # 可选（宿主依赖）
bash build.sh
```

构建完成后，安装文件将位于 `${INSTALL_PREFIX}` 目录（默认为 `/Open3D/install`）：
- 头文件：`${INSTALL_PREFIX}/include/`
- ARM64 库文件：`${INSTALL_PREFIX}/lib/aarch64/libOpen3D.so`
- x86_64 库文件：`${INSTALL_PREFIX}/lib/x86_64/libOpen3D.so`

