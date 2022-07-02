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
        ${HEADERS_DIR}/details/generated_props.hpp
    COMMAND
        clang-format ${HEADERS_DIR}/details/generated_props.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        unicode-properties-data-generator
)



#add_custom_command(
#    COMMENT "Generating data files for properties"
#    OUTPUT ${PROJECT_BINARY_DIR}/cedilla/generated_props.hpp
#    BYPRODUCTS
#      ${PROJECT_BINARY_DIR}/cedilla/generated_props_extra.hpp
#      ${PROJECT_BINARY_DIR}/cedilla/generated_props_casing.hpp
#    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
#    COMMAND mkdir -p ${HEADERS_DIR}
#    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/cedilla
#    COMMAND
#      ${Python3_EXECUTABLE}
#      ${PROJECT_SOURCE_DIR}/tools/gen.py
#      ${PROJECT_BINARY_DIR}/cedilla/generated_props.hpp
#      ${PROJECT_BINARY_DIR}/cedilla/generated_props_extra.hpp
#      ${PROJECT_BINARY_DIR}/cedilla/generated_props_casing.hpp
#      ${PROJECT_BINARY_DIR}/ucd/
#    COMMAND
#      clang-format ${PROJECT_BINARY_DIR}/cedilla/generated_props.hpp -i
#    COMMAND
#      clang-format ${PROJECT_BINARY_DIR}/cedilla/generated_props_extra.hpp -i
#    COMMAND
#      clang-format ${PROJECT_BINARY_DIR}/cedilla/generated_props_casing.hpp -i
#    DEPENDS
#      ${PROJECT_SOURCE_DIR}/tools/gen.py
#      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
#      ${PROJECT_BINARY_DIR}/ucd/PropertyValueAliases.txt
#      ${PROJECT_BINARY_DIR}/ucd/14.0/emoji-data.txt
#)

#add_custom_command(
#    COMMENT "Generating amalgated headers"
#    OUTPUT ${HEADERS_DIR}/properties.hpp
#    BYPRODUCTS
#           ${HEADERS_DIR}/casing.hpp
#    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}

#    COMMAND ${Python3_EXECUTABLE} -m quom
#                ${PROJECT_SOURCE_DIR}/src/cedilla/properties.hpp
#                -I ${PROJECT_BINARY_DIR}
#                -I ${PROJECT_SOURCE_DIR}/src/
#                ${HEADERS_DIR}/properties.hpp

#    COMMAND ${Python3_EXECUTABLE} -m quom
#                ${PROJECT_SOURCE_DIR}/src/cedilla/casing.hpp
#                -I ${PROJECT_BINARY_DIR}
#                -I ${PROJECT_SOURCE_DIR}/src/
#                ${HEADERS_DIR}/casing.hpp

#    COMMAND clang-format ${HEADERS_DIR}/properties.hpp -i
#    COMMAND clang-format ${HEADERS_DIR}/casing.hpp -i

#    DEPENDS
#        ${PROJECT_BINARY_DIR}/cedilla/generated_props.hpp
#        ${PROJECT_BINARY_DIR}/cedilla/generated_props_extra.hpp
#        ${PROJECT_BINARY_DIR}/cedilla/generated_props_casing.hpp
#        ${PROJECT_SOURCE_DIR}/src/cedilla/properties.hpp
#        ${PROJECT_SOURCE_DIR}/src/cedilla/casing.hpp
#        ${PROJECT_SOURCE_DIR}/src/cedilla/synopsis.hpp
#        ${PROJECT_SOURCE_DIR}/src/cedilla/base.h
#        ${PROJECT_SOURCE_DIR}/src/cedilla/properties_impl.hpp
#        ${PROJECT_SOURCE_DIR}/src/cedilla/casing_impl.hpp
#        ${PROJECT_SOURCE_DIR}/src/cedilla/regex.h
#    )

add_custom_command(
    COMMENT "Generating name_to_cp.hpp"
    OUTPUT ${HEADERS_DIR}/name_to_cp.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND unicode-name-to-cp-data-generator
      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml > ${HEADERS_DIR}/name_to_cp.hpp
    COMMAND cat ${PROJECT_SOURCE_DIR}/src/name_to_cp.hpp >> ${HEADERS_DIR}/name_to_cp.hpp
    COMMAND clang-format ${HEADERS_DIR}/name_to_cp.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_SOURCE_DIR}/src/name_to_cp.hpp
        unicode-name-to-cp-data-generator
)

add_custom_command(
    COMMENT "Generating cp_to_name.hpp"
    OUTPUT ${HEADERS_DIR}/cp_to_name.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND unicode-cp-to-name-data-generator
      ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND cat ${PROJECT_SOURCE_DIR}/src/names.hpp >> ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND clang-format ${HEADERS_DIR}/cp_to_name.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/14.0/ucd.nounihan.flat.xml
        ${PROJECT_SOURCE_DIR}/src/names.hpp
        unicode-cp-to-name-data-generator
)

file(GLOB_RECURSE GENERATED_HEADERS LIST_DIRECTORIES false RELATIVE ${HEADERS_DIR} ".hpp")
message("${GENERATED_HEADERS}")

add_library(cedilla STATIC
  ${HEADERS_DIR}/cp_to_name.hpp
  ${HEADERS_DIR}/name_to_cp.hpp
  ${HEADERS_DIR}details/generated_props.hpp
  ${PROJECT_SOURCE_DIR}/src/cedilla/details/bitset.hpp
  src/unicode.cpp
)

target_include_directories(cedilla PUBLIC ${PROJECT_BINARY_DIR}/include src)

#add_custom_target("single-header")
#add_custom_command(TARGET "single-header"
#    COMMAND
#        cp ${HEADERS_DIR}/* ${PROJECT_SOURCE_DIR}/generated_includes/cedilla/
#    DEPENDS
#    include/cedilla/name_to_cp.hpp include/cedilla/cp_to_name.hpp
#    include/cedilla/properties.hpp
#    include/cedilla/casing.hpp
#)

target_compile_options(cedilla INTERFACE -std=c++20)

if(MSVC)
  target_compile_options(cedilla INTERFACE /W4 /WX)
else()
  target_compile_options(cedilla INTERFACE -Wall -Wextra -Werror)
endif()

target_include_directories(cedilla PUBLIC ${PROJECT_BINARY_DIR}/include/)

add_subdirectory(tests)
