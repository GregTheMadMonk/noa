set_source_files_properties( rapidcsv.xx PROPERTIES LANGUAGE CXX )
add_executable( local_vol_consumer main.cc )
target_sources(
    local_vol_consumer PRIVATE
    FILE_SET local_vol_consumer_rapidcsv
    TYPE CXX_MODULES
    FILES rapidcsv.xx
)

target_link_libraries( local_vol_consumer PRIVATE local_vol )
target_compile_features( local_vol_consumer PRIVATE cxx_std_23 )

file( GLOB CSV_DATA *.csv )
foreach( CSV_FILE ${CSV_DATA} )
    get_filename_component( CSV_NAME "${CSV_FILE}" NAME )
    execute_process(
        COMMAND
        ${CMAKE_COMMAND} -E create_symlink
        "${CSV_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/${CSV_NAME}"
    )
endforeach()
