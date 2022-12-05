cmake_minimum_required(VERSION 3.24)
project(lldb_test)

set(CMAKE_CXX_STANDARD 20)

add_executable(lldb_test main.cpp)

if(DEFINED ENV{LLDB_PATH})
    set(LLDB_PATH $ENV{LLDB_PATH})
endif()

if(NOT LLDB_PATH)
    set(LLVM_VERSION 14.0.0 CACHE STRING "Version of LLVM to use")
    if(DEFINED ENV{LLVM_INSTALL_DIR})
        set(LLDB_PATH $ENV{LLVM_INSTALL_DIR}/${LLVM_VERSION})
    elseif(WIN32)
        set(LLDB_PATH $ENV{HOMEDRIVE}$ENV{HOMEPATH}/libclang/${LLVM_VERSION})
    else()
        set(LLDB_PATH $ENV{HOME}/libclang/${LLVM_VERSION})
    endif()
endif()
message(STATUS "lldb: using install at ${LLDB_PATH}")

find_library(lib_lldb NAMES lldb PATHS ${LLDB_PATH}/lib REQUIRED)
target_link_libraries(lldb_test ${lib_lldb})
target_include_directories(lldb_test PRIVATE ${LLDB_PATH}/include)
set_property(TARGET lldb_test APPEND PROPERTY INSTALL_RPATH "\$ORIGIN/lldb/lib")
set_target_properties(lldb_test PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE)
file(
        COPY ${LLDB_PATH}/bin/lldb
        DESTINATION ${PROJECT_BINARY_DIR}/lldb/bin
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
file(
        COPY ${LLDB_PATH}/bin/lldb-server
        DESTINATION ${PROJECT_BINARY_DIR}/lldb/bin
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
file(
        COPY ${LLDB_PATH}/bin/lldb-argdumper
        DESTINATION ${PROJECT_BINARY_DIR}/lldb/bin
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
file(
        COPY ${LLDB_PATH}/lib/liblldb.so.14
        DESTINATION ${PROJECT_BINARY_DIR}/lldb/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)