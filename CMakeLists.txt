project(cedilla)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(PythonInterp 3 REQUIRED)

foreach(UCD_VERSION 12.0 11.0 10.0 9.0 8.0 7.0)
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
      file(DOWNLOAD "https://www.unicode.org/Public/emoji/${UCD_VERSION}/emoji-data.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/emoji-data.txt)
    endif()
endforeach()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/binary_props.txt)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ucd/binary_props.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ucd/)
endif()


if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/NormalizationTest.txt)
  message(STATUS "Fetching unicode tests")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/10.0.0/ucd/NormalizationTest.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/NormalizationTest.txt)
endif()
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/PropertyValueAliases.txt)
  message(STATUS "Fetching PropertyValueAliases.txt")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/12.0.0/ucd/PropertyValueAliases.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/PropertyValueAliases.txt)
endif()





set(CMAKE_CXX_STANDARD 17)

add_custom_command(
        COMMENT "Generating properties file"
        OUTPUT ${PROJECT_BINARY_DIR}/props.h
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/gen.py ${PROJECT_BINARY_DIR}/props.h ${PROJECT_BINARY_DIR}/ucd/
        COMMAND clang-format ${PROJECT_BINARY_DIR}/props.h -i
        DEPENDS
            ${PROJECT_SOURCE_DIR}/gen.py
        )

add_custom_command(
    COMMENT "Generating Single Header"
    OUTPUT ${PROJECT_BINARY_DIR}/singleheader/ext/unicode.hpp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/singleheader/ext/
    COMMAND cat ${PROJECT_SOURCE_DIR}/single_header.h ${PROJECT_SOURCE_DIR}/base.h ${PROJECT_BINARY_DIR}/props.h ${PROJECT_SOURCE_DIR}/unicode.h > ${PROJECT_BINARY_DIR}/singleheader/ext/unicode.hpp
    DEPENDS
        ${PROJECT_BINARY_DIR}/props.h
        ${PROJECT_SOURCE_DIR}/base.h
        ${PROJECT_SOURCE_DIR}/unicode.h
        ${PROJECT_SOURCE_DIR}/single_header.h
    )

add_library(std_ext_uni STATIC
    gen.py
    props.h
    unicode.h
    base.h
    unicode.cpp
    ${PROJECT_BINARY_DIR}/singleheader/ext/unicode.hpp
)

add_custom_target("single-header")

add_custom_command(TARGET "single-header" COMMAND cp ${PROJECT_BINARY_DIR}/singleheader/ext/unicode.hpp ${PROJECT_SOURCE_DIR}/singleheader/ext/unicode.hpp)

target_compile_options(std_ext_uni PUBLIC -std=c++17)

target_include_directories(std_ext_uni PUBLIC
    ${PROJECT_BINARY_DIR}/singleheader
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(std_ext_uni)

add_subdirectory(tests)
