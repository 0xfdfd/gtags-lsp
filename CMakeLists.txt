cmake_minimum_required(VERSION 3.12)
project(tags-lsp)

###############################################################################
# Functions
###############################################################################

# Enable all warning for target
function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

###############################################################################
# Setup executable
###############################################################################

add_executable(${PROJECT_NAME}
    "src/method/__init__.c"
    "src/method/cancelrequest.c"
    "src/method/exit.c"
    "src/method/initialize.c"
    "src/method/set_trace.c"
    "src/method/shutdown.c"
    "src/method/textdocument_didchange.c"
    "src/method/textdocument_didclose.c"
    "src/method/textdocument_didopen.c"
    "src/method/textdocument_didsave.c"
    "src/method/workspace_didchangeworkspacefolders.c"
    "src/task/__init__.c"
    "src/task/update_tags.c"
    "src/utils/alloc.c"
    "src/utils/execute.c"
    "src/utils/io.c"
    "src/utils/list.c"
    "src/utils/log.c"
    "src/utils/lsp_error.c"
    "src/utils/lsp_msg.c"
    "src/utils/lsp_parser.c"
    "src/utils/lsp_work.c"
    "src/main.c"
    "src/runtime.c"
    "src/version.c")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -DTAGS_LSP_PROG_NAME="${PROJECT_NAME}")

setup_target_wall(${PROJECT_NAME})

###############################################################################
# Dependency
###############################################################################

# libuv
include(third_party/libuv.cmake)
target_link_libraries(${PROJECT_NAME} PRIVATE libuv)

# cJSON
include(third_party/cJSON.cmake)
target_link_libraries(${PROJECT_NAME} PRIVATE cJSON)

###############################################################################
# Test
###############################################################################
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(test)
endif()
