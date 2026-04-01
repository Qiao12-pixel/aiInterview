cmake_minimum_required(VERSION 3.24)
project(aiInterview)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)

# ==============================
# 【关键】加载 Conan 工具链
# ==============================
include(${CMAKE_BINARY_DIR}/build/Debug/generators/conan_toolchain.cmake)
# message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
# ==============================
# 1. 查找 brew 安装的 Qt
# ==============================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# ==============================
# 2. 查找 brew 安装的 podofo
# ==============================
set(PODOFO_PREFIX /opt/homebrew/Cellar/podofo/1.0.3) # 实测路径

find_path(PODOFO_INCLUDE_DIRS NAMES podofo/podofo.h PATHS ${PODOFO_PREFIX}/include NO_DEFAULT_PATH)
find_library(PODOFO_LIBRARIES NAMES podofo PATHS ${PODOFO_PREFIX}/lib NO_DEFAULT_PATH)

if(NOT PODOFO_INCLUDE_DIRS OR NOT PODOFO_LIBRARIES)
    message(FATAL_ERROR "podofo not found in ${PODOFO_PREFIX}")
endif()


# ==============================
# 3. 查找 Conan 安装的其他库
# ==============================
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(PortAudio REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)

# ==============================
# 你的源文件
# ==============================
add_executable(aiInterview
        main.cpp
        testModule/webSocket.cpp
        testModule/spdlog.cpp
        testModule/test.cpp
)

# ==============================
# 链接所有库
# ==============================
target_link_libraries(aiInterview
        PRIVATE
        # Qt (brew)
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Network

        # podofo (brew)
        ${PODOFO_LIBRARIES}

        # Conan 安装的库
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
        portaudio
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        CURL::libcurl
        ZLIB::ZLIB
)

# ==============================
# podofo 头文件路径（brew）
# ==============================
target_include_directories(aiInterview
        PRIVATE
        ${PODOFO_INCLUDE_DIRS}
)
