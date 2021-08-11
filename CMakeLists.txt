cmake_minimum_required(VERSION 3.16)
project(mimic_grasping_plugin_generator
        VERSION 11.08.2021
        DESCRIPTION
        "Package to build pure c++ plugins to mimic_grasping_api")

set(CMAKE_C_COMPILER "gcc-10")
set(CMAKE_CXX_COMPILER "/usr/bin/g++-10")
set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

get_filename_component(CURRENT_DIR ./ ABSOLUTE)

set(PLUGINS_DIR_FOLDER ${CURRENT_DIR}/plugins)
set(SUBDIR_LIBS_PATH libs)

#External packages
find_package(Eigen3 REQUIRED) # To matrix operations
#find_package(Threads REQUIRED) # Multithreading
find_package(Boost REQUIRED COMPONENTS system thread timer) # Multithreading

add_library(simple_network SHARED IMPORTED GLOBAL)
set_property(TARGET simple_network PROPERTY IMPORTED_LOCATION "/usr/local/lib/simple_network/libsimple_network.so")

add_library(plugin_system_management SHARED IMPORTED GLOBAL)
set_property(TARGET plugin_system_management PROPERTY IMPORTED_LOCATION "/usr/local/lib/plugin_system_management/libplugin_system_management.so")

add_library(transform_manipulation SHARED IMPORTED GLOBAL)
set_property(TARGET transform_manipulation PROPERTY IMPORTED_LOCATION "/usr/local/lib/transform_manipulation/libtransform_manipulation.so")

include_directories(
        include/6d_mimic_interface
)

## ----------------------------------------------------------------------------------------------------------------------------------
## -----------------------------------------------------------  6DMimic interface LIB- ----------------------------------------------
## ----------------------------------------------------------------------------------------------------------------------------------

add_library(6d_mimic_interface SHARED #Shared is to export it latter
        src/6d_mimic_interface/6d_mimic_interface.cpp
        include/6d_mimic_interface/6d_mimic_interface.h)

target_link_libraries(6d_mimic_interface
        simple_network
        plugin_system_management
        transform_manipulation
        #${BOOST_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        #${EIGEN_LIBRARIES}
        Eigen3::Eigen
        Boost::thread
        Boost::timer
        jsoncpp
        )

target_include_directories(6d_mimic_interface PRIVATE .)

set_target_properties(6d_mimic_interface PROPERTIES
        #VERSION ${PROJECT_VERSION}
        PUBLIC_HEADER "include/6d_mimic_interface/6d_mimic_interface.h"
        )

configure_file(install_configurations/6dmimic_interface.pc.in 6d_mimic_interface.pc @ONLY)

install(TARGETS 6d_mimic_interface
        LIBRARY DESTINATION ${PLUGINS_DIR_FOLDER}
        PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/public_headers)

install(FILES ${CMAKE_BINARY_DIR}/6d_mimic_interface.pc
        DESTINATION ${CMAKE_BINARY_DIR}/pkgconfig)


## ----------------------------------------------------------------------------------------------------------------------------------
## --------------------------------------------------------  Executables ------------------------------------------------------------
## ----------------------------------------------------------------------------------------------------------------------------------
## Define an executable
add_executable(6dmimic_interface_node
        src/examples/6dmimic_interface_node.cpp)

## Define the executable dependencies
target_link_libraries(6dmimic_interface_node
        6d_mimic_interface
        )

