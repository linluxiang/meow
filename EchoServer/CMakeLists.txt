cmake_minimum_required(VERSION 3.0)
project(SimpleServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(SimpleServer ${SOURCE_FILES} Server.cpp Server.h Channel.cpp Channel.h EchoServer.cpp EchoServer.h opcode.h EchoHandler.cpp EchoHandler.h)