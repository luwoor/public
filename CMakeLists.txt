cmake_minimum_required(VERSION 3.20)
project(NovelWriter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

qt_add_executable(NovelWriter WIN32 main.cpp)
target_link_libraries(NovelWriter PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)