cmake_minimum_required(VERSION 3.3)
project(Peer_Sync)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(SOURCE_FILES main.c main.h client.c list.c list.h server.c stringUtils.c stringUtils.h socketUtils.c socketUtils.h server.h client.h)
add_executable(Peer_Sync ${SOURCE_FILES})
