cmake_minimum_required( VERSION 3.30 FATAL_ERROR )

# Back up C++ flags - we'll need them to build Enzyme
# Then we'll temporarily enable libc++ to enable `import std` support
# Then we'll restore the backup and include Enzyme
# After that we will add libc++ again
set( CMAKE_CXX_FLAGS_BCK "${CMAKE_CXX_FLAGS}" )

# `import std` only works with libc++ + experimental feature support enabled
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
set( CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508" )

project(
    enzyme_examples
    DESCRIPTION "Temproary project for Enzyme examples until it's in NOA"
    LANGUAGES C CXX
)

# Finally enable `import std` support for the project
set( CMAKE_CXX_MODULE_STD ON )

# Set all .xx files as C++ sources
file( GLOB_RECURSE XX_FILES *.xx )

# Include CPM.cmake
include( CPM.cmake/cmake/CPM.cmake )

# Import dependencies
# Enzyme
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BCK}" )
CPMAddPackage(
    GITHUB_REPOSITORY "EnzymeAD/Enzyme"
    GIT_TAG "main"
    SOURCE_SUBDIR "enzyme"
)

# Restore libc++ in flags
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )

# Add subprojects
add_subdirectory( local_vol )
add_subdirectory( local_vol_consumer )

# Add Python and Torch before project that need those
find_package( Python REQUIRED COMPONENTS Development )
find_package( Torch REQUIRED )

if (NOT TORCH_PYTHON_LIBRARY)
    find_library(TORCH_PYTHON_LIBRARY torch_python REQUIRED PATH "${TORCH_INSTALL_PREFIX}/lib")
endif()

message( STATUS "Torch install prefix: ${TORCH_INSTALL_PREFIX}" )
message( STATUS "Torch libraries: ${TORCH_LIBRARIES} ${TORCH_PYTHON_LIBRARY}" )

add_library( PyTorch INTERFACE )
target_include_directories( PyTorch INTERFACE ${TORCH_INCLUDE_DIRS} )
target_link_libraries(
    PyTorch INTERFACE
    ${TORCH_LIBRARIES} ${TORCH_PYTHON_LIBRARY} Python::Python
)
target_compile_options( PyTorch INTERFACE ${TORCH_CXX_FLAGS} )

execute_process(
    COMMAND python3-config --extension-suffix
    OUTPUT_VARIABLE PYTHON_EXTENSION_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add more subprojects
add_subdirectory( example_module )
