add_library( python_bindings SHARED bindings.cc )
target_link_libraries( python_bindings PRIVATE torchext example_module )
target_compile_features( python_bindings PRIVATE cxx_std_23 )
set_target_properties(
    python_bindings PROPERTIES
    PREFIX ""
    SUFFIX "${PYTHON_EXTENSION_SUFFIX}"
    OUTPUT_NAME "python_bindings"
)
