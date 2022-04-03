cmake_minimum_required(VERSION 3.18)
include(cmake/vcpkg.cmake)
project(cedilla VERSION 1.0)
enable_testing()

set(PY_VERSION 3.7)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
set(Python_FIND_IMPLEMENTATIONS PyPy)
find_package(Python3 COMPONENTS Interpreter)
message("-- Found Python ${Python3_EXECUTABLE}")

include(cmake/unicode_data.cmake)

set(CMAKE_CXX_STANDARD 17)

add_executable(namesgen tools/names.cpp)

find_package(Threads REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

target_link_libraries(namesgen PRIVATE
  Threads::Threads
  pugixml
  fmt::fmt
  TBB::tbb
  range-v3-concepts
)

SET(HEADERS_DIR ${PROJECT_BINARY_DIR}/include/cedilla/)

add_executable(namesreversegen
    tools/namesreverse.cpp
)

target_link_libraries(namesreversegen PRIVATE
  pugixml
  fmt::fmt
  TBB::tbb
  range-v3-concepts
  Threads::Threads
)
target_compile_options(namesreversegen PRIVATE "-fopenmp-simd" "-march=native")

add_custom_command(
    COMMENT "Generating Properties Header"
    OUTPUT ${HEADERS_DIR}/properties.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/cedilla
    COMMAND ${Python3_EXECUTABLE}
      ${PROJECT_SOURCE_DIR}/tools/gen.py
      ${PROJECT_BINARY_DIR}/cedilla/generated_props.hpp
      ${PROJECT_BINARY_DIR}/cedilla/generated_props_extra.hpp
      ${PROJECT_BINARY_DIR}/ucd/
    COMMAND ${Python3_EXECUTABLE} -m quom
                ${PROJECT_SOURCE_DIR}/src/all.hpp
                -I ${PROJECT_BINARY_DIR}
                -I ${PROJECT_SOURCE_DIR}/src/
                ${HEADERS_DIR}/properties.hpp

    COMMAND clang-format ${HEADERS_DIR}/properties.hpp -i

    DEPENDS
        ${PROJECT_SOURCE_DIR}/src/all.hpp
        ${PROJECT_SOURCE_DIR}/src/cedilla/synopsys.h
        ${PROJECT_SOURCE_DIR}/src/cedilla/base.h
        ${PROJECT_SOURCE_DIR}/src/cedilla/unicode.h
        ${PROJECT_SOURCE_DIR}/src/cedilla/regex.h
        ${PROJECT_SOURCE_DIR}/tools/gen.py
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_BINARY_DIR}/ucd/PropertyValueAliases.txt
        ${PROJECT_BINARY_DIR}/ucd/14.0/emoji-data.txt
    )

add_custom_command(
    COMMENT "Generating name_to_cp.hpp"
    OUTPUT ${HEADERS_DIR}/name_to_cp.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND namesreversegen
      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml > ${HEADERS_DIR}/name_to_cp.hpp
    COMMAND cat ${PROJECT_SOURCE_DIR}/src/name_to_cp.hpp >> ${HEADERS_DIR}/name_to_cp.hpp
    COMMAND clang-format ${HEADERS_DIR}/name_to_cp.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_SOURCE_DIR}/src/name_to_cp.hpp
)

add_custom_command(
    COMMENT "Generating cp_to_name.hpp"
    OUTPUT ${HEADERS_DIR}/cp_to_name.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND namesgen
      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND cat ${PROJECT_SOURCE_DIR}/src/names.hpp >> ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND clang-format ${HEADERS_DIR}/cp_to_name.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_SOURCE_DIR}/src/names.hpp
        namesgen
)

add_library(std_ext_uni STATIC
  ${HEADERS_DIR}/cp_to_name.hpp
  ${HEADERS_DIR}/name_to_cp.hpp
  ${HEADERS_DIR}/properties.hpp
  src/unicode.cpp
)

add_custom_target("single-header")
add_custom_command(TARGET "single-header"
    COMMAND cp ${HEADERS_DIR}* ${PROJECT_SOURCE_DIR}/generated_includes/cedilla/
    DEPENDS
    include/cedilla/name_to_cp.hpp include/cedilla/cp_to_name.hpp
    include/cedilla/properties.hpp
)

target_compile_options(std_ext_uni INTERFACE -std=c++17)

if(MSVC)
  target_compile_options(std_ext_uni INTERFACE /W4 /WX)
else()
  target_compile_options(std_ext_uni INTERFACE -Wall -Wextra -Werror)
endif()

target_include_directories(std_ext_uni PUBLIC ${PROJECT_BINARY_DIR}/include/)

add_subdirectory(tests)
