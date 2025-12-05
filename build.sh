#!/bin/bash
pushd /Open3D

# 编译 ShaderEncoder 和 ShaderLinker
mkdir -p build-shader
pushd build-shader
cmake .. -DBUILD_GUI=OFF -DBUILD_PYTHON_MODULE=OFF
make ShaderEncoder ShaderLinker
popd
popd
export PATH=/Open3D/build-shader/bin:$PATH
set -e

INSTALL_PREFIX=/Open3D/install

# Build and install aarch64
mkdir -p build-aarch64
pushd build-aarch64

cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
  -DCMAKE_INSTALL_INCLUDEDIR=include \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DCMAKE_TOOLCHAIN_FILE=/Open3D/cmake/aarch64.toolchain.cmake \
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
mkdir -p ${INSTALL_PREFIX}/lib/aarch64
mv ${INSTALL_PREFIX}/lib/libOpen3D.so ${INSTALL_PREFIX}/lib/aarch64/libOpen3D.so
popd
rm -rf build-aarch64

# Build and install x86_64
mkdir -p build-x86_64
pushd build-x86_64
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
mkdir -p ${INSTALL_PREFIX}/lib/x86_64
pushd ${INSTALL_PREFIX}/lib/x86_64
ln -s ../libOpen3D.so libOpen3D.so
popd
popd
rm -rf build-x86_64
