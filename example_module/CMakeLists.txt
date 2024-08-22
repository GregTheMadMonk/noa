file( GLOB EXAMPLE_MOD_XX *.xx )
set_source_files_properties( ${EXAMPLE_MOD_XX} PROPERTIES LANGUAGE CXX )
add_library( example_module STATIC )
target_sources(
    example_module PUBLIC
    FILE_SET example_module_xx
    TYPE CXX_MODULES
    FILES ${EXAMPLE_MOD_XX}
)
target_compile_features( example_module PUBLIC cxx_std_23 )
target_include_directories(
    example_module PRIVATE "${Enzyme_SOURCE_DIR}/enzyme/include"
)

# Public because also exports differentiable templates!
target_link_libraries( example_module PUBLIC clang_enzyme )
