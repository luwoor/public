cmake_minimum_required(VERSION 3.20)
project(NovelWriterWin32 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 单文件编译，链接Windows系统原生库
add_executable(NovelWriter WIN32 main.cpp)
target_link_libraries(NovelWriter PRIVATE user32 gdi32 comctl32 shlwapi)

# 打包成单文件
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")
