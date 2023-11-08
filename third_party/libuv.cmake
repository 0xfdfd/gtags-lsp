include (ExternalProject)

set(LIBUV_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libuv-1.46.0)
set(LIBUV_INSTALL_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_party/libuv)

if (WIN32)
    set(LIBUV_LIB_PATH ${LIBUV_INSTALL_PATH}/lib/libuv.lib)
	set(LIBUV_DEP_LIBS ws2_32 dbghelp iphlpapi userenv)
else ()
    set(LIBUV_LIB_PATH ${LIBUV_INSTALL_PATH}/lib/libuv.a)
	set(LIBUV_DEP_LIBS rt)
endif ()

ExternalProject_Add(3rd_libuv
    SOURCE_DIR
        ${LIBUV_SOURCE_PATH}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=${LIBUV_INSTALL_PATH}
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_TESTING=OFF
        -DLIBUV_BUILD_SHARED=OFF
    INSTALL_COMMAND
        ${CMAKE_COMMAND}
        --build .
        --target install --config ${CMAKE_BUILD_TYPE}
    BUILD_BYPRODUCTS
        ${LIBUV_LIB_PATH})

# INTERFACE_INCLUDE_DIRECTORIES requires include path exists.
file(MAKE_DIRECTORY ${LIBUV_INSTALL_PATH}/include)

add_library(libuv STATIC IMPORTED GLOBAL)
set_target_properties(libuv PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${LIBUV_INSTALL_PATH}/include
    IMPORTED_LOCATION ${LIBUV_LIB_PATH})
target_link_libraries(libuv INTERFACE ${LIBUV_DEP_LIBS})

add_dependencies(libuv 3rd_libuv)
