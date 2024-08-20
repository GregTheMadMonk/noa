add_library( example_module SHARED module.cc )
target_link_libraries( example_module PRIVATE ClangEnzymeFlags PyTorch )
target_compile_features( example_module PRIVATE cxx_std_23 )
target_include_directories(
    example_module PRIVATE "${Enzyme_SOURCE_DIR}/enzyme/include"
)
set_target_properties(
    example_module PROPERTIES
    PREFIX ""
    SUFFIX "${PYTHON_EXTENSION_SUFFIX}"
    OUTPUT_NAME "example_module"
)
