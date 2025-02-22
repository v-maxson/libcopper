file (GLOB_RECURSE TEMPLATE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h.in)

set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)

foreach (OLDFILE ${TEMPLATE_HEADERS})
    get_filename_component(NEWFILE ${OLDFILE} NAME_WLE)
    configure_file (${OLDFILE} ${GENERATED_HEADERS_DIR}/${NEWFILE})
endforeach ()

include_directories(${GENERATED_HEADERS_DIR})