file( GLOB LOCAL_VOL_MOD *.xx )
set_source_files_properties( ${LOCAL_VOL_MOD} PROPERTIES LANGUAGE CXX )
add_library( local_vol STATIC local_vol.cc )
target_sources(
    local_vol PUBLIC
    FILE_SET local_vol_modules
    TYPE CXX_MODULES
    FILES ${LOCAL_VOL_MOD}
)
target_compile_features( local_vol PUBLIC cxx_std_23 )
target_link_libraries( local_vol PRIVATE ClangEnzymeFlags )
# Doesn't automatically inject includes for modules for some reason
target_include_directories(
    local_vol PRIVATE
    "${Enzyme_SOURCE_DIR}/enzyme/include"
)
