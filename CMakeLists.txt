cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

project(MCU_ISP)

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(mcu_isp src/main.cpp src/io_stream.cpp src/serial_port.cpp src/mcu_isp.cpp)
target_link_libraries(mcu_isp ${Boost_LIBRARIES})

