cmake_minimum_required( VERSION 3.20 )

project(
    enzyme-consumer
    LANGUAGES C CXX
)

include( FetchContent )
FetchContent_Declare(
    Enzyme
    URL "https://github.com/EnzymeAD/Enzyme/archive/refs/heads/main.zip"
    SOURCE_SUBDIR "enzyme"
)
FetchContent_MakeAvailable( Enzyme )

find_package( BLAS REQUIRED )

message( STATUS "Enzyme location: ${Enzyme_SOURCE_DIR}" )

add_library( functions SHARED functions.cc )
target_link_libraries( functions PRIVATE ClangEnzymeFlags )
target_include_directories( functions PRIVATE ${Enzyme_SOURCE_DIR} BLAS::BLAS )
target_sources(
    functions PUBLIC
    FILE_SET functions_h
    TYPE HEADERS
    FILES functions.hh
)
