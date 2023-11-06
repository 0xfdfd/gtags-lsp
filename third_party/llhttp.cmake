include (ExternalProject)

set(LLHTTP_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llhttp-release-v9.1.3)
set(LLHTTP_INSTALL_PATH ${CMAKE_CURRENT_BINARY_DIR}/third_party/llhttp)

if (WIN32)
    set(LLHTTP_LIB_PATH ${LLHTTP_INSTALL_PATH}/lib/llhttp.lib)
else ()
    set(LLHTTP_LIB_PATH ${LLHTTP_INSTALL_PATH}/lib/libllhttp.a)
endif ()

ExternalProject_Add(3rd_llhttp
    SOURCE_DIR
        ${LLHTTP_SOURCE_PATH}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX:PATH=${LLHTTP_INSTALL_PATH}
        -DBUILD_SHARED_LIBS=OFF
		-DBUILD_STATIC_LIBS=ON
        -DBUILD_TESTING=OFF
    INSTALL_COMMAND
        ${CMAKE_COMMAND}
        --build .
        --target install --config Release
    BUILD_BYPRODUCTS
        ${LLHTTP_LIB_PATH})

# INTERFACE_INCLUDE_DIRECTORIES requires include path exists.
file(MAKE_DIRECTORY ${LLHTTP_INSTALL_PATH}/include)

add_library(llhttp STATIC IMPORTED GLOBAL)
set_target_properties(llhttp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${LLHTTP_INSTALL_PATH}/include
    IMPORTED_LOCATION ${LLHTTP_LIB_PATH})

add_dependencies(llhttp 3rd_llhttp)
