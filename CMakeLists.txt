cmake_minimum_required(VERSION 3.26)
project(dvbp)

set(CMAKE_CXX_STANDARD 20)
include_directories(src)
include_directories(request)
include_directories(scheduler)
include_directories(storage)

add_executable(dvbp
        src/request/request.cpp
        src/request/request.h

        src/scheduler/scheduler.h
        src/scheduler/scheduler.cpp
        src/scheduler/schedulerClairvoyant.cpp
        src/scheduler/schedulerClairvoyant.h
        src/scheduler/HA.cpp
        src/scheduler/LA.cpp
        src/scheduler/RCP.cpp

        src/storage/storage.cpp
        src/storage/storage.h
        src/storage/vmType.cpp 
        src/storage/vmType.h
        
        src/main.cpp
        src/util.cpp
        src/util.h
)

target_compile_options(dvbp PRIVATE --std=c++20)