cmake_minimum_required(VERSION 3.14)

project(DEVS)
SET(PROJECT_VERSION_MAJOR 0)
SET(PROJECT_VERSION_MINOR 1)

set (CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_COMPILER "g++")
SET(CMAKE_CXX_COMPILER "g++")

SET(GXX_STDLIB_FLAGS "-lstdc++")
#ADD_COMPILE_OPTIONS(-lstdc++)
#add_definitions(${GXX_STDLIB_FLAGS})
#SET (CMAKE_CXX_FLAGS "${GXX_STDLIB_FLAGS}")

SET(KERNEL_HEADERS kernel/include/Atomic.hpp kernel/include/Coupled.hpp kernel/include/DEVSDEF.h kernel/include/Digraph.hpp kernel/include/Entity.hpp kernel/include/Entstr.hpp kernel/include/List.hpp kernel/include/Log.hpp kernel/include/MList.hpp kernel/include/Message.hpp kernel/include/Model.hpp kernel/include/Port.hpp)
SET(KERNEL_SOURCES kernel/src/Atomic.cpp kernel/src/Coupled.cpp kernel/src/Digraph.cpp kernel/src/Entity.cpp kernel/src/Entstr.cpp kernel/src/List.cpp kernel/src/Log.cpp kernel/src/MList.cpp kernel/src/Message.cpp kernel/src/Model.cpp kernel/src/Port.cpp)
SET(HEADER_FILES include/Generator.hpp include/Process.hpp include/Transducer.hpp include/Utility.hpp)
SET(SOURCE_FILES src/Generator.cpp src/Process.cpp src/Transducer.cpp src/Utility.cpp)

ADD_EXECUTABLE( main.out src/main.cpp ${KERNEL_HEADERS} ${KERNEL_SOURCES} ${HEADER_FILES} ${SOURCE_FILES} )
