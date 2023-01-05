# Philipp Neufeld, 2023

cmake_minimum_required (VERSION 3.17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# don't allow build in the source directory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
   message(FATAL_ERROR "You don't want to configure in the source directory! Please use cmake [<options>] -S <path-to-source> -B <path-to-build>")
endif()

# define project
project ("PooledCalculationLibrary" VERSION 1.0)

# include directories
set(CP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}")

# add static library
set(CP_LIB_TARGET "${CMAKE_PROJECT_NAME}")
set(CP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/CalcPool")
set(CP_SOURCES
    "${CP_SOURCE_DIR}/Execution/Progress.cpp"
    "${CP_SOURCE_DIR}/Execution/ThreadPool.cpp"
    "${CP_SOURCE_DIR}/Execution/ServerPool.cpp"
    "${CP_SOURCE_DIR}/Networking/TCPIP.cpp"
    "${CP_SOURCE_DIR}/Networking/PackageServer.cpp"
    "${CP_SOURCE_DIR}/Util/Cpu.cpp"
    "${CP_SOURCE_DIR}/Util/UUID.cpp"
    "${CP_SOURCE_DIR}/Util/Argparse.cpp"
    "${CP_SOURCE_DIR}/Util/ProgressBar.cpp"
    )
add_library("${CP_LIB_TARGET}" STATIC "${CP_SOURCES}")

# require C++17 for library and all children using it
target_compile_features("${CP_LIB_TARGET}" PUBLIC cxx_std_17)

# set include directory as interface
target_include_directories("${CP_LIB_TARGET}" INTERFACE "${CP_INCLUDE_DIR}")

# CpuDetect checks for SIMD support and exports SIMD_MACRO_DEFINITIONS and SIMD_COMPILER_FLAGS
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include (CpuDetect)
target_compile_definitions("${CP_LIB_TARGET}" PUBLIC "${SIMD_MACRO_DEFINITIONS}")
target_compile_options("${CP_LIB_TARGET}" PUBLIC "${SIMD_COMPILER_FLAGS}")

# threading
find_package(Threads REQUIRED)
if (Threads_FOUND)
    target_link_libraries("${CP_LIB_TARGET}" PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif()

# GNUC on an ARM system (disable ABI warning)
if (CMAKE_COMPILER_IS_GNUCC AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    message("ARM system and GNUC compiler detected. Disabling ABI warnings (-Wno-psabi)")
    target_compile_options("${CP_LIB_TARGET}" PUBLIC "-Wno-psabi")
endif()

# put the built library file into the .../lib/ folder
set(BUILD_LIB_DIR "${CMAKE_BINARY_DIR}/lib")
set_target_properties("${CP_LIB_TARGET}" 
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${BUILD_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BUILD_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${BUILD_LIB_DIR}"
)

set(CP_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}${CP_LIB_TARGET}${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(CP_LIB_PATH "${BUILD_LIB_DIR}/${CP_LIB_NAME}")

# function that adds a target for a qsim app
function(qsim_add_app APP_TARGET)
    add_subdirectory("apps/${APP_TARGET}")
    target_link_libraries("${APP_TARGET}" PUBLIC "${CP_LIB_TARGET}")
endfunction()

# add test app
add_executable("Test" "main.cpp" )
target_link_libraries("Test" PUBLIC "${CP_LIB_TARGET}")
