# =====================================================================
# 车机业务代码 mycode - CMake 构建 (CLion 打开本目录)
#
# 本目录只放业务代码 (src/ include/ sim/ server/ third_party/cjson)。
# LVGL 源码 / lv_drivers / lv_conf.h / lv_drv_conf.h 一律用 mycode 文件夹
# 外面的 (上层 sdl2/ 工程里那套), 不在这里重复携带, 上层 lv_conf.h 升级
# 或改配置时模拟器自动跟随。
#
# PC 模拟器 (WSL, 弹出 1024x600 SDL 窗口):
#   前置: sudo apt install libsdl2-dev
#   cmake -B build-sim -DBUILD_CLIENT=OFF -DBUILD_SERVER=OFF
#   cmake --build build-sim
#   运行: build-sim/bin/carpad_sim   (登录/注册/天气为模拟数据)
#
# 聊天/游戏服务器 (PC 运行, 配合模拟器联调):
#   cmake -B build-server -DBUILD_CLIENT=OFF -DBUILD_SDL_SIM=OFF
#   产物: build-server/bin/chat_server  game_server
#
# 上板 (RK3568): 用上层 LVGL/ 工程编译 (其 CMakeLists/Makefile 已集成
#   mycode)。本目录 BUILD_CLIENT 仅在自带板级移植层时用:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake \
#         -DBUILD_CLIENT=ON -DBUILD_SERVER=OFF -DBUILD_SDL_SIM=OFF
# =====================================================================
cmake_minimum_required(VERSION 3.10)
project(mycode C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -O1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------- 外部依赖 (mycode 文件夹外的东西, 均可命令行覆盖) ----------
set(LVGL_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/../lvgl      CACHE PATH "LVGL 源码 (外部 sdl2/)")
set(LV_DRIVERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lv_drivers CACHE PATH "lv_drivers 源码 (外部 sdl2/)")
set(CONF_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/..           CACHE PATH "lv_conf.h / lv_drv_conf.h 所在目录 (外部)")
set(PORT_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/../lv_port   CACHE PATH "板级移植层 (可选, 提供 lv_port_init)")

option(BUILD_CLIENT  "编译车机客户端 (板端, 需板级移植层)" OFF)
option(BUILD_SERVER  "编译聊天/游戏服务器 (PC 运行)"       ON)
option(BUILD_SDL_SIM "编译 PC SDL2 模拟器 (WSL/Windows)"    ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cjson)

# ---------- cJSON (mycode 独有, 外部工程没有) ----------
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cjson/cJSON.c)
    message(FATAL_ERROR "缺少 third_party/cjson/cJSON.c")
endif()
set(CJSON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cjson/cJSON.c)

# ---------- 外部 LVGL / lv_drivers ----------
if(NOT EXISTS ${LVGL_DIR}/lvgl.h)
    message(WARNING "LVGL_DIR (${LVGL_DIR}) 下没找到 lvgl.h, 请检查上层 sdl2/ 工程")
endif()
if(NOT EXISTS ${CONF_DIR}/lv_conf.h)
    message(WARNING "CONF_DIR (${CONF_DIR}) 下没找到 lv_conf.h, 请检查上层 sdl2/ 工程")
endif()
file(GLOB_RECURSE LVGL_SRCS  ${LVGL_DIR}/src/*.c)
file(GLOB         LVSDL_SRCS ${LV_DRIVERS_DIR}/sdl/*.c)

# ---------- 车机客户端 (板端) ----------
if(BUILD_CLIENT)
    file(GLOB_RECURSE APP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
    file(GLOB         PORT_SRCS ${PORT_DIR}/*.c)

    add_executable(carpad ${APP_SRCS} ${PORT_SRCS} ${LVGL_SRCS} ${CJSON_SRC})
    target_include_directories(carpad PRIVATE
        ${CONF_DIR} ${LVGL_DIR} ${LVGL_DIR}/src ${PORT_DIR})
    find_package(Threads REQUIRED)
    target_link_libraries(carpad PRIVATE Threads::Threads m)
endif()

# ---------- 服务器 ----------
if(BUILD_SERVER)
    add_executable(chat_server server/chat_server.c src/core/protocol.c)
    find_package(Threads REQUIRED)
    target_link_libraries(chat_server PRIVATE Threads::Threads)

    add_executable(game_server server/game_server.c)
    target_link_libraries(game_server PRIVATE Threads::Threads)
endif()

# ---------- PC SDL2 模拟器 ----------
if(BUILD_SDL_SIM)
    set(SDL2_LIBS "")
    set(SDL2_INC_DIRS "")
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        set(SDL2_LIBS SDL2::SDL2)
    else()
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2_PC QUIET sdl2)
        endif()
        if(SDL2_PC_FOUND)
            set(SDL2_INC_DIRS ${SDL2_PC_INCLUDE_DIRS})
            set(SDL2_LIBS ${SDL2_PC_LIBRARIES})
        endif()
    endif()

    if(NOT SDL2_LIBS)
        message(STATUS "未找到 SDL2, 跳过 carpad_sim (WSL 里: sudo apt install libsdl2-dev)")
    else()
        message(STATUS "SDL2 OK -> 启用 carpad_sim (LVGL: ${LVGL_DIR})")
        file(GLOB SIM_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sim/*.c)
        file(GLOB UI_SRCS  ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.c)

        # 模拟器 = UI + core + 桩入口, 不编 net/task/main (桩在 sim/main_sdl.c)
        add_executable(carpad_sim
            ${SIM_SRCS}
            ${UI_SRCS}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/msg_queue.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/config.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/app_manager.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/protocol.c
            ${LVSDL_SRCS}
            ${LVGL_SRCS}
            ${CJSON_SRC})

        target_include_directories(carpad_sim PRIVATE
            ${CONF_DIR}                        # 外部 lv_conf.h / lv_drv_conf.h
            ${LVGL_DIR}
            ${LVGL_DIR}/src
            ${LV_DRIVERS_DIR}                  # 外部 lv_drivers (sdl/sdl.h)
            ${SDL2_INC_DIRS})
        target_compile_definitions(carpad_sim PRIVATE
            LV_CONF_INCLUDE_SIMPLE=1
            LV_LVGL_H_INCLUDE_SIMPLE=1
            CARPAD_PC_SIM=1)
        find_package(Threads REQUIRED)
        target_link_libraries(carpad_sim PRIVATE ${SDL2_LIBS} Threads::Threads)
    endif()
endif()
