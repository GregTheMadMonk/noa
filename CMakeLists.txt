####################################################################################################
#################                      NOA: main cmake file                       ##################
####################################################################################################

# Minimum cmake version
cmake_minimum_required(VERSION 3.25.2 FATAL_ERROR) # 3.25.2 adds NVCC C++20 support

# Project
project(NOA
        VERSION 0.0.1
        DESCRIPTION
        "Differentiable Programming Algorithms in Modern C++"
        HOMEPAGE_URL https://github.com/grinisrit/noa
        LANGUAGES C CXX)

# User options
option(INSTALL_NOA "Install NOA to GNU locations" ON)
option(BUILD_NOA_TESTS "Build and run tests " ON)
option(BUILD_NOA_BENCHMARKS "Build and run benchmarks " OFF)
option(BUILD_NOA_CUDA "Build CUDA support" OFF)
option(BUILD_JNOA "Build JNI bindings" OFF)
option(BUILD_DOCS "Build documentation with doxygen" OFF)
option(NO_OMP "Disables OpenMP libraries from being used in some parts the build. This is a temporal thing, as it appears that using OMP affects stability of some solvers" OFF)

# Add OpenMP to target
function(target_add_openmp target_name)
	if( NO_OMP )
		message( STATUS "NO_OMP is defined, skipping adding OpenMP for target ${target_name}" )
		return()
	endif()
	find_package(OpenMP QUIET)
	target_include_directories( ${target_name} AFTER PRIVATE ${OpenMP_CXX_INCLUDE_DIRS} )
	target_link_libraries( ${target_name} PRIVATE ${OpenMP_CXX_LIBRARIES} )
	target_compile_options( ${target_name} BEFORE PRIVATE -DHAVE_OPENMP $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}> )
endfunction()

# Require C++17 for most of the NOA
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warning flags
set(W_FLAGS "") #"-Wall;-Wextra;-Wpedantic")

# Native optimisations
set(NA_OPT_FLAGS "-march=native;-mtune=native")

# CUDA modern C++ extensions
set(MCXX_CUDA "--expt-relaxed-constexpr;--expt-extended-lambda")

# Cache path to root directory
set(NOA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Add custom modules
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Make sure LibTorch is installed 
include(check-torch)

# Set up CUDA if available
if (BUILD_NOA_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif ()

# NOA library  
add_library(${PROJECT_NAME} INTERFACE)
# Alias for super-build pattern
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Header-only library
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# C++20 NOA headers
add_library(${PROJECT_NAME}_CXX20 INTERFACE)
# Alias
add_library(
    ${PROJECT_NAME}::${PROJECT_NAME}_CXX20 ALIAS ${PROJECT_NAME}_CXX20
) # <-- add_library

# Interface LibTorch
target_link_libraries(${PROJECT_NAME} INTERFACE torch)
set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_LINK_LIBRARIES torch)

# C++20 NOA uses C++17 headers too
target_link_libraries(${PROJECT_NAME}_CXX20 INTERFACE ${PROJECT_NAME})
set_target_properties(
    ${PROJECT_NAME}_CXX20
    PROPERTIES INTERFACE_LINK_LIBRARIES ${PROJECT_NAME}
) # <-- set_target_properties
# C++20 NOA requires at least C++20 (naturally)
target_compile_features(${PROJECT_NAME}_CXX20 INTERFACE cxx_std_20)
# Library headers
target_include_directories(
    ${PROJECT_NAME}_CXX20
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src-cxx20>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
) # <-- target_include_directories

if (BUILD_NOA_TESTS)
    enable_testing()
    add_subdirectory(test)
    add_subdirectory(test-cxx20)
endif ()

if (BUILD_NOA_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()

if (BUILD_JNOA)
    add_subdirectory(jnoa)
endif ()

if (BUILD_DOCS)
    include(doxygen-docs)
endif ()


if (INSTALL_NOA)
    # locations are provided by GNUInstallDirs
    install(TARGETS
            ${PROJECT_NAME}
            EXPORT
            ${PROJECT_NAME}_Targets)

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            ${PROJECT_NAME}ConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion)

    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
            ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake INSTALL_DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(EXPORT ${PROJECT_NAME}_Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
            ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
            ${PROJECT_SOURCE_DIR}/cmake/pumas-materials.cmake
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/noa
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/src-cxx20/noa-cxx20
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

endif ()
