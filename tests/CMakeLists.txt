find_package(Threads)

macro(add_test name file)
    add_executable(${name} ${file} common.h common.cpp pugixml.cpp )
    target_link_libraries(${name} std_ext_uni)
    #target_link_libraries(${name} gcov)
    #setup_target_for_coverage(NAME ${name}_coverage DEPENDENCIES ${name} EXECUTABLE ${name})
    target_include_directories(${name} PUBLIC . ${CMAKE_SOURCE_DIR}/third_party/catch2/include)
endmacro()

add_test(tst_prop_script prop_script.cpp)
target_compile_definitions(tst_prop_script PRIVATE -DUCDXML_FILE="${CMAKE_SOURCE_DIR}/ucd/12.0/ucd.nounihan.flat.xml")