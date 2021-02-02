project(UDP)
cmake_minimum_required(VERSION 2.4.6)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(PkgConfig REQUIRED)
pkg_search_module (PC_JSON-C REQUIRED json-c json)
find_library(MATRIX_CREATOR_HAL matrix_creator_hal)

include_directories("${PROJECT_SOURCE_DIR}/include")

set (SOURCES src/udp_rec.c src/json_parsing.c)
set(CMAKE_BUILD_TYPE "Debug")

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/lib")

#Add base directory for includes (global)

find_path (JSON_C_INCLUDE_DIR json.h PATH_SUFFIXES json-c json)
find_library (JSON_C_LIBRARIES NAMES json-c libjson-c)
include_directories (${JSON_C_INCLUDE_DIR} ${PROJECT_SOURCE_DIR/include})


add_executable(udp_exe
    ${SOURCES}
)

target_link_libraries(udp_exe
    ${JSON_C_LIBRARIES}
    ${MATRIX_CREATOR_HAL}
    libm.so
)

install(TARGETS udp_exe DESTINATION bin)
