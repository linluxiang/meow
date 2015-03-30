cmake_minimum_required(VERSION 3.0)
project(EchoClient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")

set(SOURCE_FILES main.cpp)
add_executable(EchoClient ${SOURCE_FILES} EchoClient.cpp EchoClient.h opcode.h)