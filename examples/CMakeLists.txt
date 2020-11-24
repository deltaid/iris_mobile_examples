cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(examples)

set(ARCH "Win32" CACHE STRING "The architecture to build for.")
set(INCDIR "${CMAKE_CURRENT_LIST_DIR}/../include" CACHE STRING "Path to includes.")
set(LIBDIR "${CMAKE_CURRENT_LIST_DIR}/../lib/" CACHE STRING "Path to lib folders which contains the architecture subfolders.")

message("Build for architecture ${ARCH}.")
message("Include path is ${INCDIR}.")
message("Libraries path is ${LIBDIR}.")

find_library(IRM2_LIB NAMES iris_mobile_v2 PATHS ${LIBDIR}/${ARCH})
add_library(iris_mobile_v2 UNKNOWN IMPORTED)
set_target_properties(
    iris_mobile_v2
    PROPERTIES 
        IMPORTED_LOCATION
            ${IRM2_LIB}    
        INTERFACE_INCLUDE_DIRECTORIES
            ${INCDIR}    
    )

find_library(IIR_LIB NAMES iris_image_record PATHS ${LIBDIR}/${ARCH})
add_library(iris_image_record UNKNOWN IMPORTED)
set_target_properties(
    iris_image_record
    PROPERTIES 
        IMPORTED_LOCATION
            ${IIR_LIB}
        INTERFACE_INCLUDE_DIRECTORIES
            ${INCDIR}    
    )

find_library(IRE_LIB NAMES iris_engine_v3 PATHS ${LIBDIR}/${ARCH})
add_library(iris_engine_v3 UNKNOWN IMPORTED)
set_target_properties(
    iris_image_record
    PROPERTIES 
        IMPORTED_LOCATION
            ${IRE_LIB}
        INTERFACE_INCLUDE_DIRECTORIES
            ${INCDIR}    
    )

add_subdirectory("irm2")    
add_subdirectory("ire3")
