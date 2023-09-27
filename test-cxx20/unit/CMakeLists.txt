####################################################################################################
############################              NOA unit testing             #############################
####################################################################################################


# GTest
include(fetch-gtest)

# Test data
include(noa-test-data)

# Find ZLIB
find_package(ZLIB)

# Source files
set(NOA_CXX20_TEST_SRC
        noa-cxx20-test-suite.cc
        test-combine.cc
        test-domain.cc
        test-mhfem.cc
        ${NOA_ROOT_DIR}/test/kernels.cc)

# Main test executable
add_executable(noa_cxx20_test_suite ${NOA_CXX20_TEST_SRC})
add_dependencies(noa_cxx20_test_suite test_data)

# Link libraries
target_link_libraries(noa_cxx20_test_suite PRIVATE gtest ${PROJECT_NAME}_CXX20 ZLIB::ZLIB)
target_compile_options(noa_cxx20_test_suite
        PRIVATE -O3 -DHAVE_ZLIB
        $<$<COMPILE_LANGUAGE:CXX>: ${W_FLAGS}>
        $<$<COMPILE_LANGUAGE:CUDA>:${MCXX_CUDA}>)

target_add_openmp( noa_cxx20_test_suite )


# Set up test
add_test(NAME TEST_${PROJECT_NAME}_CXX20
        COMMAND noa_cxx20_test_suite
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
