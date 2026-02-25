cmake_minimum_required(VERSION 3.10)
project(carista_task)

set(CMAKE_CXX_STANDARD 20)

file(GLOB APP_SOURCES "src/can_parser.cpp" "src/main.cpp")

add_executable(carista_task ${APP_SOURCES})
target_compile_options(carista_task PRIVATE -O3 -Wall -Werror)

add_executable(test EXCLUDE_FROM_ALL src/test.cpp src/can_parser.cpp)
target_compile_options(test PRIVATE -O0 -g3 -Wall -Werror)