# Add Python and Torch before project that need those
find_package( Python REQUIRED COMPONENTS Development )
find_package( Torch REQUIRED )

if (NOT TORCH_PYTHON_LIBRARY)
    find_library(TORCH_PYTHON_LIBRARY torch_python REQUIRED PATH "${TORCH_INSTALL_PREFIX}/lib")
endif()

message( STATUS "Torch install prefix: ${TORCH_INSTALL_PREFIX}" )
message( STATUS "Torch libraries: ${TORCH_LIBRARIES} ${TORCH_PYTHON_LIBRARY}" )

add_library( PyTorch INTERFACE )
target_include_directories( PyTorch INTERFACE ${TORCH_INCLUDE_DIRS} )
target_link_libraries(
    PyTorch INTERFACE
    ${TORCH_LIBRARIES} ${TORCH_PYTHON_LIBRARY} Python::Python
)
target_compile_options( PyTorch INTERFACE ${TORCH_CXX_FLAGS} )

set_source_files_properties( torchext.xx base.xx PROPERTIES LANGUAGE CXX )
add_library( torchext STATIC )
target_sources(
    torchext PUBLIC
    FILE_SET torchext_xx
    TYPE CXX_MODULES
    FILES torchext.xx base.xx
)
target_compile_features( torchext PUBLIC cxx_std_23 )
target_link_libraries( torchext PUBLIC PyTorch )
