# CMake 最低版本号要求
cmake_minimum_required(VERSION 2.8)

# 项目信息
project(FtpClient)

set(INCROOT ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRCROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 指定头文件
include_directories(${INCROOT})

set(FILES_HEADER
        ${INCROOT}/funcpp.h
)

set(FILES_SRC
        ${SRCROOT}/encrypt.cpp
        ${SRCROOT}/transFile.cpp
        ${SRCROOT}/client.cpp
    )

# 指定生成目标
add_executable(FtpClient ${FILES_HEADER} ${FILES_SRC})

# 目标文件与库文件进行链接
target_link_libraries(FtpClient pthread crypt crypto)