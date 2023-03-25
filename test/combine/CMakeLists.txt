####################################################################################################
##################                  'Combine' test manager test                  ###################
####################################################################################################

# Gflags
include(fetch-gflags)

# Main executables
add_executable(test_combine test.cc)

# Link libraries
target_link_libraries(test_combine PRIVATE ${PROJECT_NAME} gflags)
