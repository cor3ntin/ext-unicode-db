find_package(Threads)
find_package(Catch2)
macro(add_test name file)
    add_executable(${name} ${file} common.h common.cpp pugixml.cpp )
    target_link_libraries(${name} std_ext_uni Catch2::Catch2)
    target_compile_options(${name} PRIVATE -std=c++17 -O3)
    target_compile_definitions(${name} PRIVATE -DUCDXML_FILE="${PROJECT_BINARY_DIR}/ucd/13.0/ucd.nounihan.flat.xml")
endmacro()

add_test(tst_prop_script prop_script.cpp)

add_test(tst_name tst_names.cpp)
target_link_libraries(tst_name fmt::fmt)
