# Main executables
add_executable(test_combine main.cc test.cc)

# Link libraries
target_link_libraries(test_combine PRIVATE ${PROJECT_NAME} gtest)
target_add_openmp( test_combine )
