cmake_minimum_required(VERSION 3.10)
project(GameHub_Server)

#set(CMAKE_CXX_STANDARD 14)

include_directories(src)
include_directories(include)
include_directories(lib)

add_executable(Server
    src/Server.cpp
    src/Lobby.cpp
    src/Player.cpp)
