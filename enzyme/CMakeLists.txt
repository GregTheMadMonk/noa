add_library( enzyme_mod STATIC )
set_source_files_properties( enzyme.xx PROPERTIES LANGUAGE CXX )
target_sources(
    enzyme_mod PUBLIC
    FILE_SET enzyme_mod_xx
    TYPE CXX_MODULES
    FILES enzyme.xx
)
target_compile_features( enzyme_mod PUBLIC cxx_std_23 )
target_include_directories(
    enzyme_mod PUBLIC "${Enzyme_SOURCE_DIR}/enzyme/include"
)

add_library( clang_enzyme INTERFACE )
target_link_libraries( clang_enzyme INTERFACE enzyme_mod ClangEnzymeFlags )

add_library( lld_enzyme INTERFACE )
target_link_libraries( lld_enzyme INTERFACE enzyme_mod LLDEnzymeFlags )
