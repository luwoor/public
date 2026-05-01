cmake_minimum_required(VERSION 3.20)
project(NovelWin32 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(NovelWriter WIN32 main.cpp)
target_link_libraries(NovelWriter PRIVATE user32 gdi32 comctl32 shlwapi)
