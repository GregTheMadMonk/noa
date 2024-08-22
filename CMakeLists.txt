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
add_subdirectory( enzyme )

add_subdirectory( example_module )
add_subdirectory( example_consumer )
add_subdirectory( local_vol )
add_subdirectory( local_vol_consumer )

execute_process(
    COMMAND python3-config --extension-suffix
    OUTPUT_VARIABLE PYTHON_EXTENSION_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_subdirectory( torch )

# Add more subprojects
add_subdirectory( python_bindings )
