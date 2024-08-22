add_executable( example_consumer main.cc )
target_link_libraries( example_consumer PRIVATE example_module )
target_compile_features( example_consumer PRIVATE cxx_std_23 )
