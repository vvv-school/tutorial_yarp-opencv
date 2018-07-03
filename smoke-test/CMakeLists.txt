# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 3.5)

# set the project name
project(TestTutorialYarpOpencv)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
set(CMAKE_SHARED_MODULE_PREFIX "")

# add the required cmake packages
find_package(RTF)
find_package(RTF 1.3 COMPONENTS DLL)
find_package(YARP 3.0.0 REQUIRED COMPONENTS OS sig dev math rtf)

# add include directories
include_directories(${CMAKE_SOURCE_DIR}
                    ${RTF_INCLUDE_DIRS})

# add the source codes to build the plugin library
add_library(${PROJECT_NAME} MODULE test.cpp)

# add required libraries
target_link_libraries(${PROJECT_NAME} ${RTF_LIBRARIES} ${YARP_LIBRARIES})

