foreach(UCD_VERSION 14.0)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION})
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION})
    endif()
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/ucd.nounihan.flat.xml)
      message(STATUS "Fetching unicode ${UCD_VERSION} data")
      file(DOWNLOAD "https://unicode.org/Public/${UCD_VERSION}.0/ucdxml/ucd.nounihan.flat.zip"
          ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/ucd.nounihan.flat.zip)
      execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ucd.nounihan.flat.zip WORKING_DIRECTORY
          ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION})
    endif()
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/emoji-data.txt)
    message(STATUS "Fetching emoji-data.txt")
      file(DOWNLOAD "https://unicode.org/Public/${UCD_VERSION}.0/ucd/emoji/emoji-data.txt"
          ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/emoji-data.txt)
    endif()
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/UnicodeData.txt)
    message(STATUS "Fetching UnicodeData.txt")
      file(DOWNLOAD "https://unicode.org/Public/${UCD_VERSION}.0/ucd/UnicodeData.txt"
          ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/UnicodeData.txt)
    endif()
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/NameAliases.txt)
    message(STATUS "Fetching NameAliases.txt")
      file(DOWNLOAD "https://unicode.org/Public/${UCD_VERSION}.0/ucd/NameAliases.txt"
          ${CMAKE_CURRENT_BINARY_DIR}/ucd/${UCD_VERSION}/NameAliases.txt)
    endif()
endforeach()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/binary_props.txt)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ucd/binary_props.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ucd/)
endif()


if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/NormalizationTest.txt)
  message(STATUS "Fetching unicode tests")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/14.0.0/ucd/NormalizationTest.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/NormalizationTest.txt)
endif()
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ucd/PropertyValueAliases.txt)
  message(STATUS "Fetching PropertyValueAliases.txt")
  file(DOWNLOAD "ftp://ftp.unicode.org/Public/14.0.0/ucd/PropertyValueAliases.txt" ${CMAKE_CURRENT_BINARY_DIR}/ucd/PropertyValueAliases.txt)
endif()
