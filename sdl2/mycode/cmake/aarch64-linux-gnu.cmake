# =====================================================================
# aarch64-linux-gnu.cmake - RK3568 交叉编译工具链文件
#
# 用法:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake
#   cmake --build build
#
# 前提: 已安装交叉编译链 (Ubuntu: sudo apt install gcc-aarch64-linux-gnu)
# =====================================================================
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_PREFIX aarch64-linux-gnu)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

# 只在工具链 sysroot 里找库/头文件, 避免混入主机 x86 的库
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
