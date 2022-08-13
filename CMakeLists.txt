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

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(generator)


SET(HEADERS_DIR ${PROJECT_BINARY_DIR}/include/cedilla/)
#set(CMAKE_CXX_FLAGS "-fsanitize=address")


add_custom_command(
    COMMENT "Generating binary properties header"
    OUTPUT
        ${HEADERS_DIR}/details/generated_props.hpp
    WORKING_DIRECTORY
        ${PROJECT_BINARY_DIR}
    COMMAND
        mkdir -p ${HEADERS_DIR}/details
    COMMAND unicode-properties-data-generator
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_BINARY_DIR}/ucd/PropertyValueAliases.txt
        ${HEADERS_DIR}/details/generated_props.hpp
    COMMAND
        clang-format ${HEADERS_DIR}/details/generated_props.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_BINARY_DIR}/ucd/PropertyValueAliases.txt
        unicode-properties-data-generator
)

add_custom_command(
    COMMENT "Generating normalization header"
    OUTPUT
        ${HEADERS_DIR}/details/generated_normalization.hpp
    WORKING_DIRECTORY
        ${PROJECT_BINARY_DIR}
    COMMAND
        mkdir -p ${HEADERS_DIR}/details
    COMMAND unicode-normalization-data-generator
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${HEADERS_DIR}/details/generated_normalization.hpp
    COMMAND
        clang-format ${HEADERS_DIR}/details/generated_normalization.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_BINARY_DIR}/ucd/PropertyValueAliases.txt
        unicode-normalization-data-generator
)


#add_custom_command(
#    COMMENT "Generating name_to_cp.hpp"
#    OUTPUT ${HEADERS_DIR}/name_to_cp.hpp
#    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
#    COMMAND mkdir -p ${HEADERS_DIR}
#    COMMAND unicode-name-to-cp-data-generator
#      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml > ${HEADERS_DIR}/name_to_cp.hpp
#    COMMAND cat ${PROJECT_SOURCE_DIR}/src/name_to_cp.hpp >> ${HEADERS_DIR}/name_to_cp.hpp
#    COMMAND clang-format ${HEADERS_DIR}/name_to_cp.hpp -i
#    DEPENDS
#        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
#        ${PROJECT_SOURCE_DIR}/src/name_to_cp.hpp
#        unicode-name-to-cp-data-generator
#)

#add_custom_command(
#    COMMENT "Generating cp_to_name.hpp"
#    OUTPUT ${HEADERS_DIR}/cp_to_name.hpp
#    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
#    COMMAND mkdir -p ${HEADERS_DIR}
#    COMMAND unicode-cp-to-name-data-generator
#      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml ${HEADERS_DIR}/cp_to_name.hpp
#    COMMAND cat ${PROJECT_SOURCE_DIR}/src/names.hpp >> ${HEADERS_DIR}/cp_to_name.hpp
#    COMMAND clang-format ${HEADERS_DIR}/cp_to_name.hpp -i
#    DEPENDS
#        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
#        ${PROJECT_SOURCE_DIR}/src/names.hpp
#        unicode-cp-to-name-data-generator
#)

file(GLOB_RECURSE GENERATED_HEADERS LIST_DIRECTORIES false RELATIVE ${HEADERS_DIR} ".hpp")
message("${GENERATED_HEADERS}")

add_library(cedilla STATIC
#  ${HEADERS_DIR}/cp_to_name.hpp
#  ${HEADERS_DIR}/name_to_cp.hpp
  ${HEADERS_DIR}details/generated_props.hpp
  ${HEADERS_DIR}/details/generated_normalization.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/details/hangul.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/details/skiplist.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/details/sets.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/details/scripts.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/details/to.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/utf.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/properties.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/normalization.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/casing.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/grapheme_cluster.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/word_break.hpp
  ${PROJECT_SOURCE_DIR}/src/unicode.cpp
)

target_include_directories(cedilla PUBLIC ${PROJECT_BINARY_DIR}/include src)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(cedilla PUBLIC fmt::fmt)

#add_custom_target("single-header")
#add_custom_command(TARGET "single-header"
#    COMMAND
#        cp ${HEADERS_DIR}/* ${PROJECT_SOURCE_DIR}/generated_includes/cedilla/
#    DEPENDS
#    include/cedilla/name_to_cp.hpp include/cedilla/cp_to_name.hpp
#    include/cedilla/properties.hpp
#    include/cedilla/casing.hpp
#)

target_compile_options(cedilla PUBLIC -std=c++20 -march=native)

if(MSVC)
  target_compile_options(cedilla INTERFACE /W4 /WX)
else()
  target_compile_options(cedilla INTERFACE -Wall -Wextra -Werror)
endif()

target_include_directories(cedilla PUBLIC ${PROJECT_BINARY_DIR}/include/)

add_subdirectory(tests)
