project(cedilla)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(PythonInterp 3 REQUIRED)

foreach(UCD_VERSION 13.0)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION})
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION})
    endif()
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/ucd.nounihan.flat.xml)
      message(STATUS "Fetching unicode ${UCD_VERSION} data")
      file(DOWNLOAD "ftp://ftp.unicode.org/Public/${UCD_VERSION}.0/ucdxml/ucd.nounihan.flat.zip" ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/ucd.nounihan.flat.zip)
      execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ucd.nounihan.flat.zip WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION})
    endif()
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/emoji-data.txt)
    message(STATUS "Fetching emoji-data.txt")
      file(DOWNLOAD "ftp://ftp.unicode.org/Public/${UCD_VERSION}.0/ucd/emoji/emoji-data.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/emoji-data.txt)
    endif()
endforeach()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/binary_props.txt)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ucd/binary_props.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ucd/)
endif()


if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/NormalizationTest.txt)
  message(STATUS "Fetching unicode tests")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/13.0.0/ucd/NormalizationTest.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/NormalizationTest.txt)
endif()
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/PropertyValueAliases.txt)
  message(STATUS "Fetching PropertyValueAliases.txt")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/13.0.0/ucd/PropertyValueAliases.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/PropertyValueAliases.txt)
endif()

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(third_party/fmt)
add_subdirectory(third_party/pstl2019_20191006oss)
add_executable(namesgen
    tools/names.cpp
    tests/pugixml.cpp
)
target_link_libraries(namesgen PRIVATE fmt::fmt ParallelSTL)
target_compile_options(namesgen PRIVATE "-fopenmp-simd")
target_include_directories(namesgen PUBLIC third_party/range-v3/include third_party/fmt/include)

SET(HEADERS_DIR ${PROJECT_BINARY_DIR}/include/cedilla/)

add_executable(namesreversegen
    tools/namesreverse.cpp
    tests/pugixml.cpp
)
target_link_libraries(namesreversegen PRIVATE fmt::fmt ParallelSTL)
target_compile_options(namesreversegen PRIVATE "-fopenmp-simd" "-march=native" "-g3")
target_include_directories(namesreversegen PUBLIC third_party/range-v3/include third_party/fmt/include)

add_custom_command(
    COMMENT "Generating Properties Header"
    OUTPUT ${HEADERS_DIR}/properties.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND pypy3 ${PROJECT_SOURCE_DIR}/tools/gen.py ${PROJECT_BINARY_DIR}/props.h ${PROJECT_BINARY_DIR}/extra.h ${PROJECT_BINARY_DIR}/ucd/
    COMMAND clang-format ${PROJECT_BINARY_DIR}/props.h -i
    COMMAND clang-format ${PROJECT_BINARY_DIR}/extra.h -i
    COMMAND cat ${PROJECT_SOURCE_DIR}/single_header.h
                ${PROJECT_SOURCE_DIR}/synopsys.h
                ${PROJECT_SOURCE_DIR}/base.h
                ${PROJECT_BINARY_DIR}/props.h
                ${PROJECT_SOURCE_DIR}/unicode.h
                ${PROJECT_BINARY_DIR}/extra.h
                ${PROJECT_SOURCE_DIR}/regex.h
                > ${HEADERS_DIR}/properties.hpp
    DEPENDS
        ${PROJECT_SOURCE_DIR}/synopsys.h
        ${PROJECT_SOURCE_DIR}/base.h
        ${PROJECT_SOURCE_DIR}/unicode.h
        ${PROJECT_SOURCE_DIR}/regex.h
        ${PROJECT_SOURCE_DIR}/single_header.h
        ${PROJECT_SOURCE_DIR}/tools/gen.py
        ${PROJECT_BINARY_DIR}/ucd/13.0/ucd.nounihan.flat.xml
    )

add_custom_command(
    COMMENT "Generating name_to_cp.hpp"
    OUTPUT ${HEADERS_DIR}/name_to_cp.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND namesreversegen
      ${PROJECT_BINARY_DIR}/ucd/13.0/ucd.nounihan.flat.xml > ${HEADERS_DIR}/name_to_cp.hpp
    COMMAND cat ${PROJECT_SOURCE_DIR}/name_to_cp.hpp >> ${HEADERS_DIR}/name_to_cp.hpp
    COMMAND clang-format ${HEADERS_DIR}/name_to_cp.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/13.0/ucd.nounihan.flat.xml
        ${PROJECT_SOURCE_DIR}/name_to_cp.hpp
)

add_custom_command(
    COMMENT "Generating cp_to_name.hpp"
    OUTPUT ${HEADERS_DIR}/cp_to_name.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${HEADERS_DIR}
    COMMAND namesgen
      ${PROJECT_BINARY_DIR}/ucd/13.0/ucd.nounihan.flat.xml ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND cat ${PROJECT_SOURCE_DIR}/names.hpp >> ${HEADERS_DIR}/cp_to_name.hpp
    COMMAND clang-format ${HEADERS_DIR}/cp_to_name.hpp -i
    DEPENDS
        ${PROJECT_BINARY_DIR}/ucd/13.0/ucd.nounihan.flat.xml
        ${PROJECT_SOURCE_DIR}/names.hpp
        namesgen
)

add_library(std_ext_uni STATIC
  ${HEADERS_DIR}/cp_to_name.hpp
  ${HEADERS_DIR}/name_to_cp.hpp
  ${HEADERS_DIR}/properties.hpp
  empty.cpp
  unicode.cpp
)

add_custom_target("single-header")
add_custom_command(TARGET "single-header"
    COMMAND cp ${HEADERS_DIR}* ${PROJECT_SOURCE_DIR}/generated_includes/cedilla/
    DEPENDS include/cedilla/name_to_cp.hpp include/cedilla/cp_to_name.hpp include/cedilla/properties.hpp
)

target_compile_options(std_ext_uni INTERFACE -std=c++17)

if(MSVC)
  target_compile_options(std_ext_uni INTERFACE /W4 /WX)
else()
  # target_compile_options(std_ext_uni INTERFACE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories(std_ext_uni PUBLIC ${PROJECT_BINARY_DIR}/include/)

add_subdirectory(tests)
