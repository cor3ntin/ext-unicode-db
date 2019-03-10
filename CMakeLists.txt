project(cedilla)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(PythonInterp 3 REQUIRED)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ucd/ucd.nounihan.flat.xml)
  message("Fetching unicode data")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/12.0.0/ucdxml/ucd.nounihan.flat.zip" ${CMAKE_CURRENT_SOURCE_DIR}/ucd/ucd.nounihan.flat.zip)
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ucd.nounihan.flat.zip WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ucd)
endif()
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ucd/NormalizationTest.txt)
  message("Fetching unicode tests")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/10.0.0/ucd/NormalizationTest.txt" ${CMAKE_CURRENT_SOURCE_DIR}/ucd/NormalizationTest.txt)
endif()
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ucd/PropertyValueAliases.txt)
  message("Fetching PropertyValueAliases.txt")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/12.0.0/ucd/PropertyValueAliases.txt" ${CMAKE_CURRENT_SOURCE_DIR}/ucd/PropertyValueAliases.txt)
endif()





set(CMAKE_CXX_STANDARD 17)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/props.h
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/gen.py ${PROJECT_SOURCE_DIR}/props.h
        COMMAND /opt/clangsvn/bin/clang-format ${PROJECT_SOURCE_DIR}/props.h -i
        DEPENDS
            ${PROJECT_SOURCE_DIR}/gen.py
        )
#APPEND_COVERAGE_COMPILER_FLAGS()

add_library(std_ext_uni STATIC
    gen.py
    props.h
    unicode.h
    utils.h
    props.cpp
)

target_include_directories(std_ext_uni PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(std_ext_uni)

add_subdirectory(tests)
