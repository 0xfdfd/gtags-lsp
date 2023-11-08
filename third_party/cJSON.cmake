include (ExternalProject)

set(CJSON_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON-1.7.16)
set(CJSON_INSTALL_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_party/cJSON)

if (WIN32)
    set(CJSON_LIB_PATH ${CJSON_INSTALL_PATH}/lib/cjson.lib)
else ()
    set(CJSON_LIB_PATH ${CJSON_INSTALL_PATH}/lib/libcjson.a)
endif ()

ExternalProject_Add(3rd_cJSON
    SOURCE_DIR
        ${CJSON_SOURCE_PATH}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=${CJSON_INSTALL_PATH}
        -DENABLE_CJSON_TEST=OFF
        -DBUILD_SHARED_LIBS=OFF
    INSTALL_COMMAND
        ${CMAKE_COMMAND}
        --build .
        --target install --config ${CMAKE_BUILD_TYPE}
    BUILD_BYPRODUCTS
        ${CJSON_LIB_PATH})

# INTERFACE_INCLUDE_DIRECTORIES requires include path exists.
file(MAKE_DIRECTORY ${CJSON_INSTALL_PATH}/include)

add_library(cJSON STATIC IMPORTED GLOBAL)
set_target_properties(cJSON PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CJSON_INSTALL_PATH}/include
    IMPORTED_LOCATION ${CJSON_LIB_PATH})

add_dependencies(cJSON 3rd_cJSON)
